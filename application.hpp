#pragma once
#include <atomic>

#include "simulation.hpp"
#include "window.hpp"
#include "particle_renderer.hpp"

class application
{
private:
	static constexpr double sim_size = 200.;
	static constexpr double g_const = 0.02;
	static constexpr double particle_size = 0.5;
	static constexpr double dt = 0.005;
	static constexpr double drag_factor = 0.05;
	static constexpr double collision_max_force = 10;
	static constexpr double initial_velocity_factor = 0.0;
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
	particle_renderer m_renderer;

	void init();
	void generate_particles();
	void window_key_callback(const int &key, const int &scancode, const int &action, const int &mods);
	void window_cursor_pos_callback(const double &x, const double &y);
	void window_mouse_button_callback(const int &button, const int &action, const int &mods);

public:
	application();
	~application();

	void run();
};
