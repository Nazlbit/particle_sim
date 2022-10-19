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
#include <random>
#include <array>
#include <functional>

using namespace std::chrono_literals;

constexpr double char_ratio = 0.47;
constexpr double sim_size = 150.;
constexpr double G = 0.02;
constexpr double diameter = 1.;
constexpr double dt = 0.01;
constexpr double drag_factor = 0.1;
constexpr double collision_max_force = 10.0;
constexpr double initial_velocity_factor = 0.04;
constexpr size_t num_particles = 4000;
constexpr size_t max_particles_per_cell = 32;
constexpr double wall_collision_velocity = 0.;
constexpr double surrounding_cells_distance_multiplier = 2.;
constexpr double generation_scale = 0.5;
constexpr int fps = 20;
constexpr size_t num_threads = 10;

uint32_t screen_width, screen_height;
double ratio;
double sim_width, sim_height;
std::vector<char> screen;

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

class barrier
{
	const uint64_t m_num;
	std::atomic_uint64_t m_i = 0;
	std::atomic_bool m_wait = true;
	std::function<void()> m_on_barrier;

public:
	template<class T>
	barrier(const uint64_t num, T &&on_barrier)
		: m_num(num)
		, m_on_barrier(std::forward<T>(on_barrier)) {}

	void wait()
	{
		while(!m_wait)
		{
			// spinlock
		}

		if(++m_i < m_num)
		{
			while(m_wait)
			{
				// spinlock
			}
			--m_i;
		}
		else
		{
			m_on_barrier();
			--m_i;
			m_wait = false;
			while (m_i != 0)
			{
				// spinlock
			}
			m_wait = true;
		}
	}
};

class simulation
{
private:
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
		std::vector<const cell *> m_surrounding_cells;

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
				for (const particle &p : m_particles)
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
				for (const particle &p : m_particles)
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

		void calculate_center_of_mass()
		{
			m_center_of_mass = {};
			for (const particle &p : m_particles)
			{
				m_center_of_mass = m_center_of_mass + p.pos;
			};
			m_center_of_mass = m_center_of_mass / m_num_particles;
		}
	};

	cell m_root;
	mutable std::mutex m_mutex;
	std::vector<particle> m_all_particles;
	std::vector<particle> m_all_particles_tmp;
	std::vector<cell *> m_leafs;
	std::array<std::thread, num_threads> m_workers;
	std::atomic_size_t m_atomic = 0;
	barrier m_barrier{num_threads, [this] { m_atomic = 0; }};

	static void cell_pair_interaction(cell &a, const cell &b)
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
	}

	static vec2 particle_pair_interaction(const particle &a, const particle &b)
	{
		const vec2 ab = b.pos - a.pos;
		const double distance_squared = ab * ab;
		if (!std::isnormal(distance_squared)) [[unlikely]]
		{
			return {};
		}

		const double distance = sqrt(distance_squared);
		const vec2 unit_vec = ab / distance;

		double f;

		if (distance < diameter)
		{
			/* Collision */
			constexpr double diameter_squared = diameter * diameter;
			constexpr double q = 1. / collision_max_force;
			f = 1 - diameter_squared * (1 + q) / (distance_squared + diameter_squared * q);

			/* Drag */
			const double relative_v = (b.v - a.v) * unit_vec;
			const double drag_f = drag_factor * relative_v;
			f += drag_f;
		}
		else
		{
			/* Gravity */
			f = G / distance_squared;
		}

		/* Assume that mass is equal to 1 */
		return unit_vec * f;
	}

	static void particle_pair_interaction_local(particle &a, particle &b)
	{
		const vec2 f = particle_pair_interaction(a, b);
		a.a = a.a + f;
		b.a = b.a - f;
	}

	static void particle_pair_interaction_global(particle &a, const particle &b)
	{
		a.a = a.a + particle_pair_interaction(a, b);
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

	void calculate_physics()
	{
		const size_t num = m_leafs.size();
		int i;

		while ((i = m_atomic++) < num)
		{
			cell &c = *m_leafs[i];
			c.calculate_center_of_mass();
		}

		m_barrier.wait();

		while ((i = m_atomic++) < num)
		{
			cell &c1 = *m_leafs[i];

			for (size_t j = 0; j < num; j++)
			{
				if(j == i)
				{
					continue;
				}

				const cell &c2 = *m_leafs[j];
				cell_pair_interaction(c1, c2);
			}

			for (size_t k = 0; k < c1.m_num_particles; k++)
			{
				particle &p1 = c1.m_particles[k];
				for (size_t l = k + 1; l < c1.m_num_particles; l++)
				{
					particle &p2 = c1.m_particles[l];
					particle_pair_interaction_local(p1, p2);
				}

				for (const cell *const c : c1.m_surrounding_cells)
				{
					const cell &c2 = *c;
					for (const particle &p2 : c2.m_particles)
					{
						particle_pair_interaction_global(p1, p2);
					}
				}

				p1.a = p1.a + c1.m_a;
			}
			c1.m_surrounding_cells.clear();
			c1.m_a = {};
		}

		m_barrier.wait();

		while ((i = m_atomic++) < num)
		{
			cell &c1 = *m_leafs[i];
			for (particle &p1 : c1.m_particles)
			{
				p1.pos = p1.pos + p1.v * dt + p1.a * dt * dt * 0.5;
				p1.v = p1.v + p1.a * dt;

				simple_wall(p1, {0, 0}, {1, 0});
				simple_wall(p1, {0, 0}, {0, 1});
				simple_wall(p1, {sim_width, sim_height}, {-1, 0});
				simple_wall(p1, {sim_width, sim_height}, {0, -1});

				p1.a = {};
			}
		}
	}

	void wait_workers()
	{
		for (auto &worker : m_workers)
		{
			worker.join();
		}
	}

	template<class F>
	void spawn_workers(F &&f)
	{
		m_atomic = 0;
		for (auto &worker : m_workers)
		{
			worker = std::thread(std::forward<F>(f));
		}
	}

public:
	simulation(const rect r) : m_root(nullptr, r) {}

	std::vector<particle> get_particles() const
	{
		std::lock_guard lock(m_mutex);
		return m_all_particles;
	}

	void progress()
	{
		const auto t1 = std::chrono::steady_clock::now();

		m_leafs.clear();
		m_root.find_leafs(m_leafs);

		const auto t2 = std::chrono::steady_clock::now();

		spawn_workers([this]
					  { calculate_physics(); });

		wait_workers();

		const auto t3 = std::chrono::steady_clock::now();

		m_root.propagate_particles_up();
		m_root.propagate_particles_down();

		const auto t4 = std::chrono::steady_clock::now();

		m_all_particles_tmp.clear();
		m_root.find_particles(m_all_particles_tmp);

		const auto t5 = std::chrono::steady_clock::now();

		const auto dt1 = (t2 - t1).count();
		const auto dt2 = (t3 - t2).count();
		const auto dt3 = (t4 - t3).count();
		const auto dt4 = (t5 - t4).count();

		// printf("dt1: %lluns\n", dt1);
		// printf("dt2: %lluns\n", dt2);
		// printf("dt3: %lluns\n", dt3);
		// printf("dt4: %lluns\n", dt4);

		{
			std::lock_guard lock(m_mutex);
			m_all_particles.swap(m_all_particles_tmp);
		}
	}

	void add(const particle &p)
	{
		m_root.add(p);
		m_all_particles.push_back(p);
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

void draw_quad_tree(const simulation &sim)
{
	clear_screen();
	for (const particle &p : sim.get_particles())
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

static std::random_device r;
static std::seed_seq seed{r(), r(), r(), r(), r(), r(), r(), r()};
static std::mt19937_64 engine(seed);

double random_double(double from, double to)
{
	std::uniform_real_distribution dis(from, to);
	return dis(engine);
}

void generate_particles(simulation &sim)
{
	for (size_t i = 0; i < num_particles; ++i)
	{
		particle p;
		p.pos = {random_double(0, 1) * sim_width, random_double(0, 1) * sim_height};
		p.pos = p.pos - vec2{sim_width, sim_height} * 0.5;
		p.pos = p.pos * generation_scale;
		p.pos = p.pos + vec2{sim_width, sim_height} * 0.5;

		p.v = p.pos - vec2{sim_width, sim_height} * 0.5;
		p.v = vec2{p.v.y, -p.v.x} * initial_velocity_factor;

		sim.add(p);
	}
}

void work(simulation *sim)
{
	// const auto t1 = std::chrono::steady_clock::now();
	// uint64_t n = 0;
	// while (std::chrono::duration<double>(std::chrono::steady_clock::now() - t1).count() < 10.)
	while(true)
	{
		sim->progress();
		//  ++n;

	}
	// const auto t2 = std::chrono::steady_clock::now();
	// const double dt = std::chrono::duration<double>(t2-t1).count();

	// const double fps = n / dt;

	// printf("fps: %f\n", fps);
}

int main()
{
	init();

	simulation sim({{0, 0}, {sim_width, sim_height}});

	generate_particles(sim);

	std::thread worker(work, &sim);

	while (true)
	{
		const auto t1 = std::chrono::steady_clock::now();
		draw_quad_tree(sim);
		const auto draw_time = std::chrono::steady_clock::now() - t1;

		const auto sleep_duration = 1000'000'000ns / fps - draw_time;

		std::this_thread::sleep_for(sleep_duration);
	}

	worker.join();
	return 0;
}
