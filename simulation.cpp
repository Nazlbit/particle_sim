#include <thread>
#include <cmath>

#include "simulation.hpp"

simulation::cell::cell(cell *const parent, const rect r, const size_t particles_limit) : m_parent(parent), m_rect(r), m_particles_limit(particles_limit)
{
	m_particles.reserve(m_particles_limit + 1);
	m_children.reserve(m_num_children);
}

void simulation::cell::subdivide()
{
	static int i = 0;
	i++;
	assert(m_children.empty());
	const vec2 cell_size = m_rect.half_size * 0.5;

	m_children.emplace_back(this, rect{{m_rect.pos.x - cell_size.x, m_rect.pos.y - cell_size.y}, cell_size}, m_particles_limit); // left bottom
	m_children.emplace_back(this, rect{{m_rect.pos.x + cell_size.x, m_rect.pos.y - cell_size.y}, cell_size}, m_particles_limit); // right bottom
	m_children.emplace_back(this, rect{{m_rect.pos.x - cell_size.x, m_rect.pos.y + cell_size.y}, cell_size}, m_particles_limit); // left top
	m_children.emplace_back(this, rect{{m_rect.pos.x + cell_size.x, m_rect.pos.y + cell_size.y}, cell_size}, m_particles_limit); // right top

	m_num_particles = 0;
	for (const particle &p : m_particles)
	{
		add(p);
	}

	m_particles.clear();
}

void simulation::cell::unsubdivide()
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

void simulation::cell::add(const particle &p)
{
	++m_num_particles;

	if (m_children.empty())
	{
		m_particles.push_back(p);
		if (m_particles.size() > m_particles_limit)
		{
			subdivide();
		}
	}
	else
	{
		const vec2 cell_size = m_rect.half_size * 0.5;

		if (p.pos.y < m_rect.pos.y)
		{
			if (p.pos.x < m_rect.pos.x)
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
			if (p.pos.x < m_rect.pos.x)
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

void simulation::cell::propagate_particles_up()
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
			if (m_rect.is_inside_ordered(p.pos))
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

void simulation::cell::propagate_particles_down()
{
	if (m_num_particles <= m_particles_limit)
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

void simulation::cell::find_leafs(std::vector<cell *> &cells)
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

void simulation::cell::get_particles(std::vector<particle> &particles) const
{
	if (!m_children.empty())
	{
		for (const cell &child : m_children)
		{
			child.get_particles(particles);
		}
	}
	else
	{
		particles.insert(particles.end(), m_particles.begin(), m_particles.end());
	}
}

void simulation::cell::get_particles_positions(std::vector<vec2> &particles) const
{
	if (!m_children.empty())
	{
		for (const cell &child : m_children)
		{
			child.get_particles_positions(particles);
		}
	}
	else
	{
		auto it = particles.end();
		particles.resize(particles.size() + m_particles.size());
		for (const particle &p : m_particles)
		{
			*(it++) = p.pos;
		}
	}
}

void simulation::cell::calculate_center_of_mass()
{
	m_center_of_mass = {};
	for (const particle &p : m_particles)
	{
		m_center_of_mass = m_center_of_mass + p.pos;
	};
	m_center_of_mass = m_center_of_mass / m_num_particles;
}

simulation::simulation(const rect r, const size_t num_threads, const double dt, const double particle_size,
                       const double g_const, const double wall_collision_cor, const double collision_max_force,
					   const double drag_factor, const size_t cell_particles_limit, const double cell_proximity_factor) :
	m_root(nullptr, r, cell_particles_limit),
	m_workers(num_threads),
	m_barrier(num_threads, [this] {
		reset_leafs_iterator();
	}),
	m_barrier_end(num_threads, [this] {
		reset_leafs_iterator();
		stop_workers();
	}),
	m_dt(dt),
	m_particle_size(particle_size),
	m_g_const(g_const),
	m_wall_collision_cor(wall_collision_cor),
	m_collision_max_force(collision_max_force),
	m_drag_factor(drag_factor),
	m_cell_proximity_factor(cell_proximity_factor)
{
	for (auto &worker : m_workers)
	{
		worker = std::thread([this] {
			calculate_physics();
		});
	}
}

simulation::~simulation()
{
	for (auto &worker : m_workers)
	{
		worker.join();
	}
}

void simulation::reset_leafs_iterator()
{
	m_leafs_iterator = 0;
}

void simulation::stop_workers()
{
	m_workers_awake = false;
	m_head_workers_cv.notify_one();
}

const std::vector<vec2> &simulation::get_particles_positions() const
{
	std::lock_guard lock(m_particles_mutex);
	if(m_swap_buffers)
	{
		m_particles_positions[0].swap(m_particles_positions[1]);
		m_swap_buffers = false;
	}
	return m_particles_positions[0];
}

void simulation::progress()
{
	std::unique_lock lock{m_head_workers_mutex};
	// const auto t1 = std::chrono::steady_clock::now();

	m_leafs.clear();
	m_root.find_leafs(m_leafs);

	// const auto t2 = std::chrono::steady_clock::now();

	m_workers_awake = true;
	m_head_workers_cv.notify_all();
	m_head_workers_cv.wait(lock, [this]
						   { return !m_workers_awake; });

	// const auto t3 = std::chrono::steady_clock::now();

	m_root.propagate_particles_up();
	m_root.propagate_particles_down();

	// const auto t4 = std::chrono::steady_clock::now();

	m_particles_positions[2].clear();
	m_particles_positions[2].reserve(m_root.m_num_particles);
	m_root.get_particles_positions(m_particles_positions[2]);

	// const auto t5 = std::chrono::steady_clock::now();

	// const auto dt1 = (t2 - t1).count();
	// const auto dt2 = (t3 - t2).count();
	// const auto dt3 = (t4 - t3).count();
	// const auto dt4 = (t5 - t4).count();

	// printf("dt1: %lluns\n", dt1);
	// printf("dt2: %lluns\n", dt2);
	// printf("dt3: %lluns\n", dt3);
	// printf("dt4: %lluns\n", dt4);

	{
		std::lock_guard lock(m_particles_mutex);
		m_particles_positions[2].swap(m_particles_positions[1]);
		m_swap_buffers = true;
	}
}

void simulation::add(const particle &p)
{
	m_root.add(p);
	m_particles_positions[0].push_back(p.pos);
}

void simulation::calculate_physics()
{
	int i;
	std::shared_lock lock(m_head_workers_mutex);
	while (true)
	{
		m_head_workers_cv.wait(lock, [this]
							   { return m_workers_awake; });

		const size_t num = m_leafs.size();

		while ((i = m_leafs_iterator++) < num)
		{
			cell &c = *m_leafs[i];
			c.calculate_center_of_mass();
		}

		m_barrier.wait();

		while ((i = m_leafs_iterator++) < num)
		{
			cell &c1 = *m_leafs[i];

			for (size_t j = 0; j < num; j++)
			{
				if (j == i)
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

		while ((i = m_leafs_iterator++) < num)
		{
			cell &c1 = *m_leafs[i];
			for (particle &p1 : c1.m_particles)
			{
				p1.pos = p1.pos + p1.v * m_dt + p1.a * m_dt * m_dt * 0.5;
				p1.v = p1.v + p1.a * m_dt;

				simple_wall(p1, {m_root.m_rect.pos.x - m_root.m_rect.half_size.x, m_root.m_rect.pos.y}, {1, 0});
				simple_wall(p1, {m_root.m_rect.pos.x + m_root.m_rect.half_size.x, m_root.m_rect.pos.y}, {-1, 0});
				simple_wall(p1, {m_root.m_rect.pos.x, m_root.m_rect.pos.y - m_root.m_rect.half_size.y}, {0, 1});
				simple_wall(p1, {m_root.m_rect.pos.x, m_root.m_rect.pos.y + m_root.m_rect.half_size.y}, {0, -1});

				p1.a = {};
			}
		}
		m_barrier_end.wait();
	}
}

void simulation::simple_wall(particle &p, vec2 wall_pos, vec2 wall_normal)
{
	const vec2 r_vec = p.pos - wall_pos;
	const double distance = r_vec * wall_normal;
	const double r = m_particle_size * 0.5;
	if (distance < r) [[unlikely]]
	{
		p.pos = p.pos + wall_normal * (r - distance) * 1.001;

		const double projected_v = p.v * wall_normal;
		if (projected_v < 0)
		{
			p.v = p.v - wall_normal * projected_v * (1.0 + m_wall_collision_cor);
		}
	}
}

void simulation::particle_pair_interaction_local(particle &a, particle &b)
{
	const vec2 f = particle_pair_interaction(a, b);
	a.a = a.a + f;
	b.a = b.a - f;
}

void simulation::particle_pair_interaction_global(particle &a, const particle &b)
{
	a.a = a.a + particle_pair_interaction(a, b);
}

vec2 simulation::particle_pair_interaction(const particle &a, const particle &b)
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

	if (distance < m_particle_size)
	{
		/* Collision */
		f = collision_force(distance_squared);

		/* Drag */
		const double relative_v = (b.v - a.v) * unit_vec;
		const double drag_f = m_drag_factor * relative_v;
		f += drag_f;
	}
	else
	{
		/* Gravity */
		f = gravitational_force(distance_squared);
	}

	/* Assume that mass is equal to 1 */
	return unit_vec * f;
}

inline double simulation::collision_force(const double distance_squared) const
{
	const double diameter_squared = m_particle_size * m_particle_size;
	const double gravity_at_diameter = m_g_const / diameter_squared;
	const double q = 1. / (m_collision_max_force + gravity_at_diameter);
	return 1 - diameter_squared * (1 + q) / (distance_squared + diameter_squared * q) + gravity_at_diameter;
}

inline double simulation::gravitational_force(const double distance_squared) const
{
	return m_g_const / distance_squared;
}

void simulation::cell_pair_interaction(cell &a, const cell &b)
{
	{
		const vec2 size_sum = (a.m_rect.half_size + b.m_rect.half_size) * m_cell_proximity_factor;
		const rect r{a.m_center_of_mass, size_sum};
		if (r.is_inside_unordered(b.m_center_of_mass))
		{
			a.m_surrounding_cells.push_back(&b);
			return;
		}
	}

	const vec2 ab = b.m_center_of_mass - a.m_center_of_mass;
	const double distance_squared = ab * ab;
	const double distance = sqrt(distance_squared);
	const vec2 unit_vec = ab / distance;

	double f;
	if (distance < m_particle_size)
	{
		f = collision_force(distance_squared);
	}
	else
	{
		f = gravitational_force(distance_squared);
	}

	a.m_a = a.m_a + unit_vec * f * b.m_num_particles;
}
