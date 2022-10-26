#pragma once
#include "simulation.hpp"
#include "window.hpp"

class application
{
private:
	static constexpr double sim_size = 300.;
	static constexpr double g_const = 0.02;
	static constexpr double particle_size = 1.;
	static constexpr double dt = 0.02;
	static constexpr double drag_factor = 0.1;
	static constexpr double collision_max_force = 5.0;
	static constexpr double initial_velocity_factor = 0.04;
	static constexpr size_t num_particles = 32000;
	static constexpr size_t cell_particles_limit = 48;
	static constexpr double wall_collision_cor = 0.;
	static constexpr double generation_scale = 0.5;
	static constexpr size_t num_threads = 7;
	static constexpr double cell_proximity_factor = 1.5;
	static constexpr int fps = 20;
	static constexpr double char_ratio = 0.47;

	std::unique_ptr<simulation> m_simulation;

	uint32_t m_screen_width, m_screen_height;
	double m_sim_width, m_sim_height;
	std::vector<char> m_screen;

	window m_wnd;

	void init();
	void clear_screen();
	void draw_quad_tree();
	void generate_particles();
	void work();

public:
	void run();
};
