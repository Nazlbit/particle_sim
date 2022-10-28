#pragma once
#include "simulation.hpp"
#include "window.hpp"

class application
{
private:
	static constexpr double sim_size = 300.;
	static constexpr double g_const = 0.02;
	static constexpr double particle_size = 1.;
	static constexpr double dt = 0.01;
	static constexpr double drag_factor = 0.03;
	static constexpr double collision_max_force = 3.0;
	static constexpr double initial_velocity_factor = 0.09;
	static constexpr size_t num_particles = 16000;
	static constexpr size_t cell_particles_limit = 32;
	static constexpr double wall_collision_cor = 0.;
	static constexpr double generation_scale = 0.25;
	static constexpr size_t num_threads = 7;
	static constexpr double cell_proximity_factor = 1.01;

	std::unique_ptr<simulation> m_simulation;

	uint32_t m_screen_width, m_screen_height;
	double m_sim_width, m_sim_height;

	window m_wnd;

	void init();
	void generate_particles();
	void work();

public:
	void run();
};
