#include <cstring>
#include <cmath>
#include <sys/ioctl.h>
#include <vector>
#include <chrono>
#include <thread>
#include <cassert>
#include <unistd.h>
#include <atomic>
#include <mutex>

using namespace std::chrono_literals;

constexpr double char_ratio = 0.5;
constexpr uint32_t sim_size = 100;
constexpr double G = 0.01;
constexpr double size = 0.3;
constexpr double dt = 0.01;
constexpr double stiffness = .01;
constexpr double initial_velocity_multiplier = 0.02;
constexpr size_t num_particles = 10000;
constexpr size_t max_particles_per_cell = 64;
constexpr double wall_collision_velocity = 0.5;
constexpr double collision_factor = -0.3;
constexpr int fps = 50;

uint32_t screen_width,
	screen_height;
double ratio;
double sim_width, sim_height;
std::vector<char> screen;
std::atomic_bool present = false;
std::condition_variable cv;
std::mutex m;

struct vec2
{
	double x = 0, y = 0;
};

vec2 operator+(const vec2 &a, const vec2 &b)
{
	return {a.x + b.x, a.y + b.y};
}

vec2 operator-(const vec2 &a, const vec2 &b)
{
	return {a.x - b.x, a.y - b.y};
}

double operator*(vec2 a, vec2 b)
{
	return a.x * b.x + a.y * b.y;
}

vec2 operator*(vec2 a, double v)
{
	return {a.x * v, a.y * v};
}

vec2 operator/(vec2 a, double v)
{
	return {a.x / v, a.y / v};
}

struct particle
{
	vec2 pos;
	vec2 v;
	vec2 a;
};

struct rect
{
	vec2 pos;
	vec2 size;

	bool is_inside(const vec2 p) const
	{
		assert(size.x > 0);
		assert(size.y > 0);

		return p.x > pos.x &&
			   p.y > pos.y &&
			   p.x <= pos.x + size.x &&
			   p.y <= pos.y + size.y;
	}
};

struct cell
{
	static const size_t m_max_particles = max_particles_per_cell;
	static const uint8_t m_num_children = 4;

	std::vector<particle> m_particles;
	cell *const m_parent = nullptr;
	std::vector<cell> m_children;
	const rect m_rect;
	size_t m_num_particles = 0;
	vec2 m_center_of_mass;
	vec2 m_a;

	cell(cell *const parent, const rect r) : m_parent(parent), m_rect(r)
	{
		m_particles.reserve(m_max_particles + 1);
		m_children.reserve(m_num_children);
	}

	void subdivide()
	{
		static int i = 0;
		i++;
		assert(m_children.empty());
		const vec2 cell_size = m_rect.size * 0.5;
		const vec2 middle_point = m_rect.pos + cell_size;

		m_children.emplace_back(this, rect{m_rect.pos, cell_size});
		m_children.emplace_back(this, rect{{m_rect.pos.x + cell_size.x, m_rect.pos.y}, cell_size});
		m_children.emplace_back(this, rect{{m_rect.pos.x, m_rect.pos.y + cell_size.y}, cell_size});
		m_children.emplace_back(this, rect{{m_rect.pos.x + cell_size.x, m_rect.pos.y + cell_size.y}, cell_size});

		m_num_particles = 0;
		for (const particle &p : m_particles)
		{
			add(p);
		}

		m_particles.clear();
	}

	void unsubdivide()
	{
		for (cell &child : m_children)
		{
			if (!child.m_children.empty())
			{
				child.unsubdivide();
			}
			m_particles.insert(m_particles.cend(), child.m_particles.cbegin(), child.m_particles.cend());
		}

		m_children.clear();
	}

	void add(const particle &p)
	{
		++m_num_particles;

		if (m_children.empty())
		{
			m_particles.push_back(p);
			if (m_particles.size() > m_max_particles)
			{
				subdivide();
			}
		}
		else
		{
			const vec2 cell_size = m_rect.size * 0.5;
			const vec2 middle_point = m_rect.pos + cell_size;

			if (p.pos.y <= middle_point.y)
			{
				if (p.pos.x <= middle_point.x)
				{
					m_children[0].add(p);
				}
				else
				{
					m_children[1].add(p);
				}
			}
			else
			{
				if (p.pos.x <= middle_point.x)
				{
					m_children[2].add(p);
				}
				else
				{
					m_children[3].add(p);
				}
			}
		}
	}

	void propagate_particles_up()
	{
		if (!m_children.empty())
		{
			for (cell &child : m_children)
			{
				child.propagate_particles_up();
			}
		}

		if (m_parent != nullptr)
		{
			std::vector<particle> new_particles;
			new_particles.reserve(m_max_particles + 1);
			size_t num_removed = 0;

			for (particle &p : m_particles)
			{
				if (m_rect.is_inside(p.pos))
				{
					new_particles.push_back(p);
				}
				else
				{
					m_parent->m_particles.push_back(p);
					++num_removed;
				}
			}

			m_particles = std::move(new_particles);
			m_num_particles -= num_removed;
		}
	}

	void propagate_particles_down()
	{
		if (!m_children.empty())
		{
			if (m_num_particles <= m_max_particles)
			{
				unsubdivide();
			}
			else
			{
				m_num_particles -= m_particles.size();
				for (particle &p : m_particles)
				{
					add(p);
				}
				m_particles.clear();

				for (cell &child : m_children)
				{
					child.propagate_particles_down();
				}
			}
		}
	}

	void progress()
	{
		calculate_physics();
		propagate_particles_up();
		propagate_particles_down();
	}

	static void cell_pair_interaction(cell &a, cell &b)
	{
		const vec2 ab = b.m_center_of_mass - a.m_center_of_mass;
		const double distance = sqrt(ab * ab);
		if (!isnormal(distance))
		{
			return;
		}

		const vec2 unit_vec = ab / distance;

		double g;
		if (distance < size)
		{
			g = 0;
		}
		else
		{
			g = G / (distance * distance);
		}

		a.m_a = a.m_a + unit_vec * (g * b.m_num_particles);
		b.m_a = b.m_a - unit_vec * (g * a.m_num_particles);
	}

	static void particle_pair_interaction(particle &a, particle &b)
	{
		const vec2 ab = b.pos - a.pos;
		const double distance = sqrt(ab * ab);
		if (!isnormal(distance))
		{
			return;
		}

		const vec2 unit_vec = ab / distance;

		double f;

		/* Collision */
		if (distance < size)
		{
			f = -stiffness;
			const double relative_v = (b.v - a.v) * unit_vec;
			if (relative_v > 0)
			{
				f *= collision_factor;
			}
		}
		else
		{
			f = G / (distance * distance);
		}

		/* Assume that mass is equal to 1 */
		a.a = a.a + unit_vec * f;
		b.a = b.a - unit_vec * f;
	}

	static void simple_wall(particle &p, vec2 wall_pos, vec2 wall_normal)
	{
		const vec2 r_vec = p.pos - wall_pos;
		const double distance = r_vec * wall_normal;
		if (distance < size * 0.5)
		{
			if (distance < 0)
			{
				p.pos = p.pos - wall_normal * distance;
			}

			const double projected_v = p.v * wall_normal;
			if (projected_v < 0)
			{
				p.v = p.v - wall_normal * projected_v * (1.0 + wall_collision_velocity);
			}
		}
	}

	void find_leafs(std::vector<cell *> &cells)
	{
		if (!m_children.empty())
		{
			for (cell &child : m_children)
			{
				child.find_leafs(cells);
			}
		}
		else
		{
			if (m_num_particles > 0)
			{
				cells.push_back(this);
			}
		}
	}

	void calculate_physics()
	{
		std::vector<cell *> leafs;
		leafs.reserve(m_num_particles / m_max_particles * 2);
		find_leafs(leafs);

		/* Center of mass */
		for (cell *leaf : leafs)
		{
			cell &l = *leaf;

			l.m_center_of_mass = vec2();
			for (const particle &p : l.m_particles)
			{
				l.m_center_of_mass = l.m_center_of_mass + p.pos;
			};
			l.m_center_of_mass = l.m_center_of_mass / l.m_num_particles;
		}

		for (size_t i = 0; i < leafs.size(); i++)
		{
			cell &c1 = *leafs[i];

			for (size_t j = i + 1; j < leafs.size(); j++)
			{
				cell &c2 = *leafs[j];
				cell_pair_interaction(c1, c2);
			}

			for (size_t k = 0; k < c1.m_num_particles; k++)
			{
				particle &p1 = c1.m_particles[k];
				for (size_t l = k + 1; l < c1.m_num_particles; l++)
				{
					particle &p2 = c1.m_particles[l];
					particle_pair_interaction(p1, p2);
				}
				p1.a = p1.a + c1.m_a;

				p1.pos = p1.pos + p1.v * dt + p1.a * dt * dt * 0.5;
				p1.v = p1.v + p1.a * dt;

				simple_wall(p1, {0, 0}, {1, 0});
				simple_wall(p1, {0, 0}, {0, 1});
				simple_wall(p1, {sim_width, sim_height}, {-1, 0});
				simple_wall(p1, {sim_width, sim_height}, {0, -1});

				p1.a = vec2();
			}

			c1.m_a = vec2();
		}
	}
};

void init()
{
	struct winsize w;
	ioctl(STDOUT_FILENO, TIOCGWINSZ, &w);
	screen_width = w.ws_col;
	screen_height = w.ws_row;
	if (screen_width == 0 || screen_height == 0)
	{
		screen_width = 100;
		screen_height = 50;
	}

	ratio = screen_width * char_ratio / screen_height;
	sim_width = sim_size * ratio;
	sim_height = sim_size;
	screen.resize((screen_width + 1) * screen_height + 1);
}

void clear_screen()
{
	screen.assign((screen_width + 1) * screen_height, ' ');
	for (uint32_t i = 0; i < screen_height; i++)
	{
		screen[(screen_width + 1) * i + screen_width] = '\n';
	}
	screen[(screen_width + 1) * screen_height] = '\0';
}

void draw_recursive(const cell &quad_tree)
{
	if (quad_tree.m_children.empty())
	{
		for (const particle &p : quad_tree.m_particles)
		{
			int x = p.pos.x / sim_width * screen_width;
			int y = p.pos.y / sim_height * screen_height;
			if (x >= 0 && x < screen_width && y >= 0 && y < screen_height)
			{
				screen[(screen_width + 1) * y + x] = '+';
			}
		}
	}
	else
	{
		for (const cell &child : quad_tree.m_children)
		{
			draw_recursive(child);
		}
	}
}

void draw_quad_tree(const cell &quad_tree)
{
	clear_screen();

	{
		present = true;
		std::lock_guard lock(m);
		draw_recursive(quad_tree);
		present = false;
	}
	cv.notify_one();

	printf("%s", screen.data());
}

void generate_particles(cell &quad_tree)
{
	for (size_t i = 0; i < num_particles; ++i)
	{
		particle p;
		p.pos = {(double)rand() / RAND_MAX * sim_width, (double)rand() / RAND_MAX * sim_height};
		p.pos = p.pos - vec2{sim_width, sim_height} * 0.5;
		p.pos = p.pos * 0.8;
		p.pos = p.pos + vec2{sim_width, sim_height} * 0.5;
		p.v = p.pos - vec2{sim_width, sim_height} * 0.5;
		p.v = vec2{p.v.y, -p.v.x} * initial_velocity_multiplier;
		quad_tree.add(p);
	}
}

void work(cell *quad_tree)
{
	std::unique_lock lock(m);
	while (true)
	{
		cv.wait(lock, []{ return !present;});
		//auto t1 = std::chrono::steady_clock::now();
		quad_tree->progress();
		//auto time = std::chrono::duration<double>(std::chrono::steady_clock::now() - t1);

		//printf("time: %fs\n", time.count());
	}
}

int main()
{
	init();

	cell quad_tree(nullptr, {{0, 0}, {sim_width, sim_height}});

	generate_particles(quad_tree);

	std::thread worker(work, &quad_tree);

	while (true)
	{
		const auto t1 = std::chrono::steady_clock::now();
		draw_quad_tree(quad_tree);
		const auto draw_time = std::chrono::steady_clock::now() - t1;

		const auto sleep_duration = 1000'000'000ns / fps - draw_time;

		std::this_thread::sleep_for(sleep_duration);
	}
	return 0;
}
