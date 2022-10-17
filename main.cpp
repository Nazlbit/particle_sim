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
#include <condition_variable>

using namespace std::chrono_literals;

constexpr double char_ratio = 0.5;
constexpr double sim_size = 200.;
constexpr double G = 0.02;
constexpr double diameter = 1.;
constexpr double dt = 0.01;
constexpr double drag_factor = 0.03;
constexpr double collision_max_force = 50.0;
constexpr double collision_inner_diameter_ratio = 0.5;
constexpr double initial_velocity_factor = 0.05;
constexpr size_t num_particles = 20000;
constexpr size_t max_particles_per_cell = 32;
constexpr double wall_collision_velocity = 0.;
constexpr double surrounding_cells_distance_multiplier = 2.;
constexpr double generation_scale = 0.5;
constexpr int fps = 20;

uint32_t screen_width, screen_height;
double ratio;
double sim_width, sim_height;
std::vector<char> screen;

struct vec2
{
	double x, y;
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

	cell *const m_parent = nullptr;
	const rect m_rect;

	std::vector<particle> m_particles;
	std::vector<cell> m_children;
	size_t m_num_particles = 0;
	vec2 m_center_of_mass = {};
	vec2 m_a = {};
	std::vector<cell *> m_surrounding_cells;
	static std::mutex m_mutex;
	static std::vector<particle> m_all_particles;
	static std::vector<particle> m_all_particles_tmp;

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
		static std::vector<particle> temp_particles;
		if (!m_children.empty())
		{
			for (cell &child : m_children)
			{
				child.propagate_particles_up();
			}
		}

		if (m_parent != nullptr && !m_particles.empty())
		{
			for (particle &p : m_particles)
			{
				if (m_rect.is_inside(p.pos))
				{
					temp_particles.push_back(p);
				}
				else
				{
					m_parent->m_particles.push_back(p);
					--m_num_particles;
				}
			}

			m_particles.swap(temp_particles);
			temp_particles.clear();
		}
	}

	void propagate_particles_down()
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
				if (!child.m_children.empty())
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

		m_all_particles_tmp.clear();
		find_particles(m_all_particles_tmp);
		{
			std::lock_guard lock(m_mutex);
			m_all_particles.swap(m_all_particles_tmp);
		}
	}

	static void cell_pair_interaction(cell &a, cell &b)
	{
		const vec2 ab = b.m_center_of_mass - a.m_center_of_mass;
		const double distance_squared = ab * ab;
		if (!std::isnormal(distance_squared)) [[unlikely]]
		{
			return;
		}

		if (distance_squared < sqrt(a.m_rect.size.x * a.m_rect.size.y * b.m_rect.size.x * b.m_rect.size.y) * surrounding_cells_distance_multiplier) 
		{
			a.m_surrounding_cells.push_back(&b);
			return;
		}

		const double distance = sqrt(distance_squared);
		const vec2 unit_vec = ab / distance;

		double g;
		if (distance < diameter) 
		{
			g = 0;
		}
		else 
		{
			g = G / distance_squared;
		}

		a.m_a = a.m_a + unit_vec * (g * b.m_num_particles);
		b.m_a = b.m_a - unit_vec * (g * a.m_num_particles);
	}

	static void particle_pair_interaction(particle &a, particle &b)
	{
		const vec2 ab = b.pos - a.pos;
		const double distance_squared = ab * ab;
		if (!std::isnormal(distance_squared)) [[unlikely]]
		{
			return;
		}

		const double distance = sqrt(distance_squared);
		const vec2 unit_vec = ab / distance;

		double f;

		constexpr double inner_diameter = diameter * collision_inner_diameter_ratio;
		if (distance < inner_diameter)
		{
			/* Collision */
			constexpr double inner_diameter_squared = inner_diameter * inner_diameter;
			constexpr double q = 1. / collision_max_force;
			f = -inner_diameter_squared * (1 + q) / (distance_squared + inner_diameter_squared * q) + 1;
		}
		else
		{
			/* Gravity */
			f = G / distance_squared;
		}

		if (distance < diameter)
		{
			/* Drag */
			const double relative_v = (b.v - a.v) * unit_vec;
			const double drag_f = drag_factor * relative_v * abs(relative_v);
			f += drag_f;
		}

		/* Assume that mass is equal to 1 */
		a.a = a.a + unit_vec * f;
		b.a = b.a - unit_vec * f;
	}

	static void simple_wall(particle &p, vec2 wall_pos, vec2 wall_normal)
	{
		const vec2 r_vec = p.pos - wall_pos;
		const double distance = r_vec * wall_normal;
		if (distance < diameter * 0.5) [[unlikely]]
		{
			if (distance < 0)
			{
				p.pos = p.pos - wall_normal * distance * 1.001;
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

			l.m_center_of_mass = {};
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

				for(cell *const c : c1.m_surrounding_cells)
				{
					cell &c2 = *c;
					for(particle &p2 : c2.m_particles)
					{
						particle_pair_interaction(p1, p2);
					}

				}
				p1.a = p1.a + c1.m_a;

				p1.pos = p1.pos + p1.v * dt + p1.a * dt * dt * 0.5;
				p1.v = p1.v + p1.a * dt;

				simple_wall(p1, {0, 0}, {1, 0});
				simple_wall(p1, {0, 0}, {0, 1});
				simple_wall(p1, {sim_width, sim_height}, {-1, 0});
				simple_wall(p1, {sim_width, sim_height}, {0, -1});

				p1.a = {};
			}

			c1.m_a = {};
			c1.m_surrounding_cells.clear();
		}
	}

	void find_particles(std::vector<particle> &particles) const
	{
		if (!m_children.empty())
		{
			for (const cell &child : m_children)
			{
				child.find_particles(particles);
			}
		}
		else
		{
			particles.insert(particles.end(), m_particles.begin(), m_particles.end());
		}
	}

	static std::vector<particle> get_particles()
	{
		std::lock_guard lock(m_mutex);
		return m_all_particles;
	}
};

std::mutex cell::m_mutex;
std::vector<particle> cell::m_all_particles;
std::vector<particle> cell::m_all_particles_tmp;

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

void draw_quad_tree(const cell &quad_tree)
{
	clear_screen();
	for (const particle &p : quad_tree.get_particles())
	{
		int x = p.pos.x / sim_width * screen_width;
		int y = p.pos.y / sim_height * screen_height;
		if (x >= 0 && x < screen_width && y >= 0 && y < screen_height)
		{
			screen[(screen_width + 1) * y + x] = '#';
		}
	}

	system("clear");
	printf("%s", screen.data());
}

void generate_particles(cell &quad_tree)
{
	for (size_t i = 0; i < num_particles; ++i)
	{
		particle p;
		p.pos = {(double)rand() / RAND_MAX * sim_width, (double)rand() / RAND_MAX * sim_height};
		p.pos = p.pos - vec2{sim_width, sim_height} * 0.5;
		p.pos = p.pos * generation_scale;
		p.pos = p.pos + vec2{sim_width, sim_height} * 0.5;
		//p.pos.y = 5;
		p.v = p.pos - vec2{sim_width, sim_height} * 0.5;
		p.v = vec2{p.v.y, -p.v.x} * initial_velocity_factor;
		quad_tree.add(p);
	}
}

void work(cell *quad_tree)
{
	//const auto t1 = std::chrono::steady_clock::now();
	// uint64_t n = 0;
	//while (std::chrono::duration<double>(std::chrono::steady_clock::now() - t1).count() < 10.)
	while(true)
	{
		quad_tree->progress();
		// ++n;
	}
	// const auto t2 = std::chrono::steady_clock::now();
	// const double dt = std::chrono::duration<double>(t2-t1).count();

	// const double fps = n / dt;

	// printf("fps: %f\n", fps);
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

	worker.join();
	return 0;
}
