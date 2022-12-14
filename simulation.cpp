#include <thread>
#include <cmath>

#include "simulation.hpp"

simulation::cell::cell(cell *const parent, const cube<double> &c, const size_t &particles_limit) : m_parent(parent), m_cube(c), m_particles_limit(particles_limit)
{
	m_particles.reserve(m_particles_limit + 1);
	m_children.reserve(8);
}

void simulation::cell::subdivide()
{
	assert(m_children.empty());

	const double half_size = m_cube.half_size * 0.5;
	m_children.emplace_back(this, cube<double>{m_cube.pos + vec3<double>{-half_size, -half_size, -half_size}, half_size}, m_particles_limit);
	m_children.emplace_back(this, cube<double>{m_cube.pos + vec3<double>{-half_size, -half_size, half_size}, half_size}, m_particles_limit);
	m_children.emplace_back(this, cube<double>{m_cube.pos + vec3<double>{-half_size, half_size, -half_size}, half_size}, m_particles_limit);
	m_children.emplace_back(this, cube<double>{m_cube.pos + vec3<double>{-half_size, half_size, half_size}, half_size}, m_particles_limit);
	m_children.emplace_back(this, cube<double>{m_cube.pos + vec3<double>{half_size, -half_size, -half_size}, half_size}, m_particles_limit);
	m_children.emplace_back(this, cube<double>{m_cube.pos + vec3<double>{half_size, -half_size, half_size}, half_size}, m_particles_limit);
	m_children.emplace_back(this, cube<double>{m_cube.pos + vec3<double>{half_size, half_size, -half_size}, half_size}, m_particles_limit);
	m_children.emplace_back(this, cube<double>{m_cube.pos + vec3<double>{half_size, half_size, half_size}, half_size}, m_particles_limit);

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
		uint8_t i = 0;
		if (p.pos.x > m_cube.pos.x)
		{
			i |= 0b100;
		}
		if (p.pos.y > m_cube.pos.y)
		{
			i |= 0b010;
		}
		if (p.pos.z > m_cube.pos.z)
		{
			i |= 0b001;
		}

		m_children[i].add(p);
	}
}

void simulation::cell::propagate_particles_up(std::vector<particle> &temp_particles)
{
	if (!m_children.empty())
	{
		for (cell &child : m_children)
		{
			child.propagate_particles_up(temp_particles);
		}
	}

	if (m_parent != nullptr && !m_particles.empty())
	{
		for (const particle &p : m_particles)
		{
			if (m_cube.is_inside_ordered(p.pos))
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
		for(const particle &p : m_particles)
		{
			particles.push_back(p);
		}
	}
}

void simulation::cell::get_particles_positions(std::vector<vec3<double>> &particles) const
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
		size_t it = particles.size();
		particles.resize(particles.size() + m_particles.size());
		for (const particle &p : m_particles)
		{
			particles[it] = p.pos;
			++it;
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

simulation::simulation(const double &size, const size_t &num_threads, const double &dt, const double &particle_size,
                       const double &g_const, const double &wall_collision_cor, const double &collision_max_force,
                       const double &drag_factor, const size_t &cell_particles_limit, const double &cell_proximity_factor) :
	m_root(nullptr, cube<double>{{}, size / 2}, cell_particles_limit),
	m_workers(num_threads),
	m_barrier(num_threads, [this] {
		reset_leafs_iterator();
	}),
	m_barrier_start(num_threads + 1, [this] {
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
}

simulation::~simulation()
{
	stop();
}

void simulation::spawn_worker_threads()
{
	if (!m_workers_alive)
	{
		m_workers_alive = true;

		for (auto &worker : m_workers)
		{
			worker = std::thread([this]
								 { calculate_physics(); });
		}
	}
}

void simulation::kill_worker_threads()
{
	if (m_workers_alive)
	{
		m_workers_alive = false;
		m_head_workers_cv.notify_all();

		for (auto &worker : m_workers)
		{
			worker.join();
		}
	}
}

inline void simulation::reset_leafs_iterator()
{
	m_leafs_iterator = 0;
}

inline void simulation::stop_workers()
{
	m_workers_awake = false;
}

const std::vector<vec3<double>> &simulation::get_particles_positions() const
{
	std::lock_guard lock(m_user_access_mutex);
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
	double dt = 0;
	size_t num = 0;
	while (m_head_alive)
	{
		const auto t1 = std::chrono::steady_clock::now();

		m_leafs.clear();
		m_root.find_leafs(m_leafs);

		m_workers_awake = true;
		lock.unlock();
		m_head_workers_cv.notify_all();
		m_barrier_start.wait();
		lock.lock();

		m_root.propagate_particles_up(m_temp_particles);
		m_root.propagate_particles_down();

		m_particles_positions[2].clear();
		m_particles_positions[2].reserve(m_root.m_num_particles);
		m_root.get_particles_positions(m_particles_positions[2]);

		{
			std::lock_guard lock(m_user_access_mutex);

			m_particles_positions[2].swap(m_particles_positions[1]);
			m_swap_buffers = true;

			m_user_pointer = m_user_pointer_tmp;
		}

		const auto t2 = std::chrono::steady_clock::now();
		dt += std::chrono::duration<double>(t2-t1).count();
		++num;

		if(dt > 1)
		{
			printf("FPS: %f\n", num / dt);
			num = 0;
			dt = 0;
		}
	}
}

void simulation::start()
{
	if(!m_head_alive)
	{
		spawn_worker_threads();

		m_head_alive = true;

		m_head = std::thread([this]
							 { progress(); });
	}
}

void simulation::stop()
{
	if (m_head_alive)
	{
		m_head_alive = false;

		m_head.join();

		kill_worker_threads();
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
							   { return m_workers_awake | !m_workers_alive; });
		if(!m_workers_alive)
		{
			return;
		}

		m_barrier_start.wait();

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

				if(m_user_pointer.active)
				{
					user_pointer_force(p1);
				}
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

				spherical_wall(p1);

				p1.a = {};
			}
		}
	}
}

void simulation::simple_wall(particle &p, vec3<double> wall_pos, vec3<double> wall_normal)
{
	const vec3<double> r_vec = p.pos - wall_pos;
	const double distance = r_vec * wall_normal;
	const double r = m_particle_size * 0.5;
	if (distance < r) [[unlikely]]
	{
		p.pos = p.pos + wall_normal * (m_particle_size - distance) * 1.001;

		const double projected_v = p.v * wall_normal;
		if (projected_v < 0)
		{
			p.v = p.v - wall_normal * projected_v * (1.0 + m_wall_collision_cor);
		}
	}
}

void simulation::spherical_wall(particle &p)
{
	const double distance = sqrt(p.pos * p.pos);
	const vec3<double> normal = -p.pos.normalize();
	const double delta = distance + m_particle_size * 0.5 - m_root.m_cube.half_size;
	if (delta > 0)
	{
		p.pos = p.pos + normal * delta;
		const double projected_v = p.v * normal;
		if (projected_v < 0)
		{
			p.v = p.v - normal * projected_v * (1.0 + m_wall_collision_cor);
		}
	}
}

void simulation::particle_pair_interaction_local(particle &a, particle &b)
{
	const vec3<double> f = particle_pair_interaction(a, b);
	a.a = a.a + f;
	b.a = b.a - f;
}

void simulation::particle_pair_interaction_global(particle &a, const particle &b)
{
	a.a = a.a + particle_pair_interaction(a, b);
}

vec3<double> simulation::particle_pair_interaction(const particle &a, const particle &b)
{
	const vec3<double> ab = b.pos - a.pos;
	const double distance_squared = ab * ab;
	if (!std::isnormal(distance_squared)) [[unlikely]]
	{
		return {};
	}

	const double distance = sqrt(distance_squared);
	const vec3<double> unit_vec = ab / distance;

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

inline double simulation::collision_force(const double &distance_squared) const
{
	const double diameter_squared = m_particle_size * m_particle_size;
	const double gravity_at_diameter = m_g_const / diameter_squared;
	const double q = 1. / (m_collision_max_force + gravity_at_diameter);
	return 1 - diameter_squared * (1 + q) / (distance_squared + diameter_squared * q) + gravity_at_diameter;
}

inline double simulation::gravitational_force(const double &distance_squared) const
{
	return m_g_const / distance_squared;
}

void simulation::cell_pair_interaction(cell &a, const cell &b)
{
	{
		const vec3<double> r = b.m_cube.pos - a.m_cube.pos;
		const double size_sum = (a.m_cube.half_size + b.m_cube.half_size) * m_cell_proximity_factor;
		if (r * r < size_sum * size_sum)
		{
			a.m_surrounding_cells.push_back(&b);
			return;
		}
	}

	const vec3<double> ab = b.m_center_of_mass - a.m_center_of_mass;
	const double distance_squared = ab * ab;
	if(!std::isnormal(distance_squared)) [[unlikely]]
	{
		return;
	}

	const double distance = sqrt(distance_squared);
	const vec3<double> unit_vec = ab / distance;

	const double f = gravitational_force(distance_squared);

	a.m_a = a.m_a + unit_vec * f * b.m_num_particles;
}

void simulation::user_pointer_force(particle &p)
{
	const vec3<double> ab = m_user_pointer.pos - p.pos;
	const double distance_squared = ab * ab;
	if (!std::isnormal(distance_squared)) [[unlikely]]
	{
		return;
	}

	const double distance = sqrt(distance_squared);
	const vec3<double> unit_vec = ab / distance;

	const double radius_sum = (m_particle_size + m_user_pointer.size) * 0.5;
	if (distance < radius_sum)
	{
		/* Collision */
		const double radius_sum_squared = radius_sum * radius_sum;
		const double gravity_at_collision_point = gravitational_force(radius_sum_squared) * m_user_pointer.mass;
		constexpr double mass_force_ratio = 10;
		const double q = 1. / (m_user_pointer.mass * mass_force_ratio + gravity_at_collision_point);
		const double collision_f = 1 - radius_sum_squared * (1 + q) / (distance_squared + radius_sum_squared * q) + gravity_at_collision_point;

		const vec3<double> drag = -p.v * m_user_pointer.drag_factor;

		p.a = p.a + unit_vec * collision_f + drag;
	}
	else
	{
		/* Gravity */
		p.a = p.a + unit_vec * (gravitational_force(distance_squared) * m_user_pointer.mass);
	}
}
