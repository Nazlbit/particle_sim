#pragma once
#include "simulation.hpp"
#include "window.hpp"

class application
{
private:
	static constexpr double sim_size = 200.;
	static constexpr double g_const = 0.02;
	static constexpr double particle_size = 1.;
	static constexpr double dt = 0.0025;
	static constexpr double drag_factor = 0.005;
	static constexpr double collision_max_force = 0.1;
	static constexpr double initial_velocity_factor = 0.02;
	static constexpr size_t num_particles = 32000;
	static constexpr size_t cell_particles_limit = 32;
	static constexpr double wall_collision_cor = 0.;
	static constexpr double generation_scale = 0.4;
	static constexpr size_t num_threads = 7;
	static constexpr double cell_proximity_factor = 1.01;

	/* I don't want to make it DefaultConstructible because I'm lazy. */
	std::unique_ptr<simulation> m_simulation;

	double m_sim_width, m_sim_height;

	window m_wnd;

	void init();
	void generate_particles();
	void work();

public:
	void run();
};
