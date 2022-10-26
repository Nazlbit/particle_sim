#pragma once
#include <cstddef>
#include <cstdint>
#include <vector>
#include <mutex>
#include <shared_mutex>
#include <condition_variable>
#include <thread>
#include "math.hpp"
#include "barrier.hpp"

struct particle
{
    vec2 pos;
    vec2 v;
    vec2 a;
};

class simulation
{
private:
    struct cell
    {
		static constexpr uint8_t m_num_children = 4;

		const size_t m_particles_limit;
		cell *const m_parent = nullptr;
        const rect m_rect;

        std::vector<particle> m_particles;
        std::vector<cell> m_children;
        size_t m_num_particles = 0;
        vec2 m_center_of_mass = {};
        vec2 m_a = {};
        std::vector<const cell *> m_surrounding_cells;

		cell(cell *const parent, const rect r, const size_t particles_limit);

		void subdivide();

		void unsubdivide();

		void add(const particle &p);

		void propagate_particles_up();

		void propagate_particles_down();

		void find_leafs(std::vector<cell *> &cells);

		void find_particles_pos(std::vector<vec2> &particles) const;

		void calculate_center_of_mass();
	};

	cell m_root;
    mutable std::mutex m_particles_mutex;
    mutable std::vector<vec2> m_all_particles[3];
	mutable bool m_swap_buffers = false;
    std::vector<cell *> m_leafs;
    std::vector<std::thread> m_workers;
    std::atomic_size_t m_leafs_iterator = 0;
    std::shared_mutex m_head_workers_mutex;
    std::condition_variable_any m_head_workers_cv;
    bool m_workers_awake = false;
	barrier m_barrier;
	barrier m_barrier_end;
	double m_dt;
	double m_particle_size;
	double m_g_const;
	double m_wall_collision_cor;
	double m_collision_max_force;
	double m_drag_factor;
	double m_cell_proximity_factor;

	void reset_leafs_iterator();

	void stop_workers();

	void cell_pair_interaction(cell &a, const cell &b);

	vec2 particle_pair_interaction(const particle &a, const particle &b);

	void particle_pair_interaction_local(particle &a, particle &b);

	void particle_pair_interaction_global(particle &a, const particle &b);

	void simple_wall(particle &p, vec2 wall_pos, vec2 wall_normal);

	void calculate_physics();

	double collision_force(const double distance_squared) const;

	double gravitational_force(const double distance_squared) const;

public:
	simulation(const rect r, const size_t num_threads, const double dt, const double particle_size,
			   const double g_const, const double wall_collision_cor, const double collision_max_force,
			   const double drag_factor, const size_t cell_particles_limit, const double cell_proximity_factor);

	~simulation();

	const std::vector<vec2> &get_particles_pos() const;

	void progress();

	void add(const particle &p);
};
