#pragma once
#include <cstddef>
#include <cstdint>
#include <vector>
#include <mutex>
#include <shared_mutex>
#include <condition_variable>
#include <thread>
#include <array>

#include "math.hpp"
#include "barrier.hpp"

struct particle
{
    vec3 pos;
    vec3 v;
    vec3 a;
};

class simulation
{
private:
    struct cell
    {
		const size_t m_particles_limit;
		cell *const m_parent = nullptr;
        const cube m_cube;

        std::vector<particle> m_particles;
        std::vector<cell> m_children;
		std::vector<const cell *> m_surrounding_cells;
		vec3 m_center_of_mass = {};
        vec3 m_a = {};
		size_t m_num_particles = 0;

		cell(cell *const parent, const cube &c, const size_t &particles_limit);

		void subdivide();

		void unsubdivide();

		void add(const particle &p);

		void propagate_particles_up(std::vector<particle> &temp_particles);

		void propagate_particles_down();

		void find_leafs(std::vector<cell *> &cells);

		void get_particles(std::vector<particle> &particles) const;

		void get_particles_positions(std::vector<vec3> &particles) const;

		void calculate_center_of_mass();
	};

	cell m_root;
    mutable std::mutex m_user_access_mutex;
	static constexpr uint8_t m_particles_buffer_num = 3;
	mutable std::array<std::vector<vec3>, m_particles_buffer_num> m_particles_positions;
	mutable bool m_swap_buffers = false;
    std::vector<cell *> m_leafs;
	std::thread m_head;
	std::vector<std::thread> m_workers;
	std::atomic_bool m_head_alive = false;
	std::atomic_bool m_workers_alive = false;
	std::atomic_size_t m_leafs_iterator = 0;
	std::shared_mutex m_head_workers_mutex;
    std::condition_variable_any m_head_workers_cv;
    bool m_workers_awake = false;
	barrier m_barrier;
	barrier m_barrier_start;
	double m_dt;
	double m_particle_size;
	double m_g_const;
	double m_wall_collision_cor;
	double m_collision_max_force;
	double m_drag_factor;
	double m_cell_proximity_factor;
	std::vector<particle> m_temp_particles;
	struct user_pointer{
		bool active = false;
		vec3 pos;
		double size = 20.0;
		double mass = 10000.0;
		double drag_factor = 0.5;
	} m_user_pointer, m_user_pointer_tmp;

	void spawn_worker_threads();

	void kill_worker_threads();

	void reset_leafs_iterator();

	void stop_workers();

	void cell_pair_interaction(cell &a, const cell &b);

	vec3 particle_pair_interaction(const particle &a, const particle &b);

	void particle_pair_interaction_local(particle &a, particle &b);

	void particle_pair_interaction_global(particle &a, const particle &b);

	void simple_wall(particle &p, vec3 wall_pos, vec3 wall_normal);

	void calculate_physics();

	double collision_force(const double &distance_squared) const;

	double gravitational_force(const double &distance_squared) const;

	void user_pointer_force(particle &p);

	void progress();

public:
	simulation(const cube &c, const size_t num_threads, const double dt, const double particle_size,
			   const double g_const, const double wall_collision_cor, const double collision_max_force,
			   const double drag_factor, const size_t cell_particles_limit, const double cell_proximity_factor);

	~simulation();

	const std::vector<vec3> &get_particles_positions() const;

	void start();

	void stop();

	void add(const particle &p);

	cube get_sim_cube() const
	{
		return m_root.m_cube;
	}

	size_t get_num_particles() const
	{
		return m_root.m_num_particles;
	}

	double get_particle_size() const
	{
		return m_particle_size;
	}

	void set_pointer_pos(const vec3 &pos)
	{
		std::lock_guard lock(m_user_access_mutex);
		m_user_pointer_tmp.pos = pos;
	}

	void activate_pointer()
	{
		std::lock_guard lock(m_user_access_mutex);
		m_user_pointer_tmp.active = true;
	}

	void deactivate_pointer()
	{
		std::lock_guard lock(m_user_access_mutex);
		m_user_pointer_tmp.active = false;
	}

	void set_pointer_mass(const double &mass)
	{
		std::lock_guard lock(m_user_access_mutex);
		m_user_pointer_tmp.mass = mass;
	}

	void set_pointer_size(const double &size)
	{
		std::lock_guard lock(m_user_access_mutex);
		m_user_pointer_tmp.size = size;
	}

	void set_pointer_drag_factor(const double &drag_factor)
	{
		std::lock_guard lock(m_user_access_mutex);
		m_user_pointer_tmp.drag_factor = drag_factor;
	}
};
