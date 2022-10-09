#include <iostream>
#include <cstring>
#include <cmath>
#include <sys/ioctl.h>
#include <vector>
#include <chrono>
#include <array>
#include <thread>
#include <cassert>
using namespace std::chrono_literals;

struct vec2
{
    float x = 0, y = 0;
};

vec2 operator+(const vec2 &a, const vec2 &b)
{
    return {a.x + b.x, a.y + b.y};
}

vec2 operator-(const vec2 &a, const vec2 &b)
{
    return {a.x - b.x, a.y - b.y};
}

float operator*(vec2 a, vec2 b)
{
    return a.x * b.x + a.y * b.y;
}

vec2 operator*(vec2 a, float v)
{
	return {a.x * v, a.y * v};
}

vec2 operator/(vec2 a, float v)
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
	static const size_t m_max_particles = 4;

	std::vector<particle> m_particles;
	cell *const m_parent = nullptr;
	std::vector<cell> m_children;
	const rect m_rect;
	uint32_t m_num_particles = 0;
	vec2 center_of_mass;

	cell(cell *const parent, const rect r) : m_parent(parent), m_rect(r)
	{
		m_particles.reserve(m_max_particles + 1);
		m_children.reserve(4);
	}

	void subdivide()
	{
		static int i = 0;
		i++;
		assert(m_children.empty());
		const vec2 cell_size = m_rect.size * 0.5f;
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
			if(!child.m_children.empty())
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

		if(m_children.empty())
		{
			m_particles.push_back(p);
			if(m_particles.size() > m_max_particles)
			{
				subdivide();
			}
		}
		else
		{
			const vec2 cell_size = m_rect.size * 0.5f;
			const vec2 middle_point = m_rect.pos + cell_size;

			if(p.pos.y <= middle_point.y)
			{
				if(p.pos.x <= middle_point.x)
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
				if(p.pos.x <= middle_point.x)
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
		if(!m_children.empty())
		{
			for (cell &child : m_children)
			{
				child.propagate_particles_up();
			}
		}

		if(m_parent != nullptr)
		{
			std::vector<particle> new_particles;
			new_particles.reserve(m_max_particles);
			size_t num_removed = 0;

			for (particle &p : m_particles)
			{
				if(m_rect.is_inside(p.pos))
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
		if(!m_children.empty())
		{
			if(m_num_particles <= m_max_particles)
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

	void update()
	{
		propagate_particles_up();
		propagate_particles_down();
	}
};

uint32_t screen_width, screen_height;
float ratio;
float sim_width, sim_height;
std::vector<char> screen;

constexpr float char_ratio = 0.35f;
constexpr uint32_t sim_size = 30;
constexpr float G = 0.01f;
constexpr float size = 0.3f;
constexpr float dt = 0.1f;
constexpr float stiffness = 0.1f;
constexpr float elastic_collision_factor = 0.9f;
constexpr float wall_collision_energy_dissipation = 0.01f;
constexpr uint32_t sim_draw_ratio = 100;
constexpr float initial_velocity_multiplier = 0.1f;

constexpr uint32_t num_particles = 100;
//std::array<particle, num_particles> particles;

void init()
{
	struct winsize w;
    ioctl(1, TIOCGWINSZ, &w);
	screen_width = w.ws_col;
	screen_height = w.ws_row;
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
        screen[(screen_width+1)*i + screen_width] = '\n';
    }
    screen[(screen_width + 1) * screen_height] = '\0';
}

void wall(particle &p, vec2 wall_pos, vec2 wall_normal)
{
	const vec2 r_vec = p.pos - wall_pos;
	const float distance = r_vec * wall_normal;
	if(distance < size * 0.5f)
	{
		float collision = stiffness * size / (distance * distance);
		if (p.v * wall_normal > 0)
		{
			collision *= 1.f - wall_collision_energy_dissipation;
		}
		p.a = p.a + wall_normal * collision;
	}
}

void force(particle &a, particle &b)
{
	const vec2 ab = b.pos - a.pos;
	const float distance = sqrtf(ab * ab);
	const vec2 unit_vec = ab / distance;

	float f = 0;
	const float g = G / (distance * distance);
	f += g;

	/* Collision */
	if(distance < size)
	{
		float collision = stiffness * size * 2.f / (distance * distance);
		if ((b.v - a.v) * unit_vec > 0)
		{
			collision *= elastic_collision_factor;
		}
		f -= collision;
	}

	/*Assume that mass is equal to 1 */
	a.a = a.a + unit_vec * f;
	b.a = b.a - unit_vec * f;
}

// void simulate()
// {
// 	for (uint32_t i = 0; i < num_particles; i++)
// 	{
// 		particle &a = particles[i];
// 		for (uint32_t j = i + 1; j < num_particles; j++)
// 		{
// 			particle &b = particles[j];
// 			force(a, b);
// 		}

// 		wall(a, {0, 0}, {1, 0});
// 		wall(a, {0, 0}, {0, 1});
// 		wall(a, {sim_width, sim_height}, {-1, 0});
// 		wall(a, {sim_width, sim_height}, {0, -1});

// 		a.pos = a.pos + a.v * dt + a.a * dt * dt * 0.5f;
// 		a.v = a.v + a.a * dt;
// 		a.a = {0, 0};
// 	}
// }

void calculate_forces(const cell &quad_tree)
{

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
	draw_recursive(quad_tree);
	printf("%s", screen.data());
}

void generate_particles(cell &quad_tree)
{
	for (uint32_t i = 0; i < num_particles; ++i)
	{
		particle p;
		p.pos = {(float)rand() / RAND_MAX * sim_width, (float)rand() / RAND_MAX * sim_height};
		p.v = p.pos - vec2{sim_width, sim_height} * 0.5f;
		p.v = vec2{p.v.y, -p.v.x} * initial_velocity_multiplier;
		quad_tree.add(p);
	}
}

void simple_wall(particle &p, vec2 wall_pos, vec2 wall_normal)
{
	const vec2 r_vec = p.pos - wall_pos;
	const float distance = r_vec * wall_normal;
	if(distance < size * 0.5f)
	{
		const float projected_v = p.v * wall_normal;
		if (projected_v < 0)
		{
			p.v = p.v - wall_normal * projected_v * 2;
		}
	}
}

void simulate(cell &quad_tree)
{
	if(quad_tree.m_children.empty())
	{
		for(particle &p : quad_tree.m_particles)
		{
			simple_wall(p, {0, 0}, {1, 0});
			simple_wall(p, {0, 0}, {0, 1});
			simple_wall(p, {sim_width, sim_height}, {-1, 0});
			simple_wall(p, {sim_width, sim_height}, {0, -1});
			p.pos = p.pos + p.v * dt + p.a * dt * dt * 0.5f;
		}
	}
	else
	{
		for(cell &child : quad_tree.m_children)
		{
			simulate(child);
		}
	}
}

int main()
{
	init();

	cell quad_tree(nullptr, {{0, 0}, {sim_width, sim_height}});

	generate_particles(quad_tree);

	while(true)
	{
		simulate(quad_tree);
		draw_quad_tree(quad_tree);
		std::this_thread::sleep_for(50ms);
		quad_tree.update();
	}
	return 0;
}
