#pragma once
#include <atomic>

#include "simulation.hpp"
#include "window.hpp"
#include "particle_renderer.hpp"

class application
{
private:
	static constexpr double sim_size = 100.;
	static constexpr double g_const = 0.02;
	static constexpr double particle_size = 0.5;
	static constexpr double dt = 0.0025;
	static constexpr double drag_factor = 0.025;
	static constexpr double collision_max_force = 1;
	static constexpr double initial_velocity_factor = 0.2;
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
	std::atomic_bool m_worker_active = true;
	particle_renderer m_renderer;
	std::thread m_worker;

	void init();
	void generate_particles();
	void work();
	void window_key_callback(const int &key, const int &scancode, const int &action, const int &mods);
	void window_cursor_pos_callback(const double &x, const double &y);

public:
	application();
	~application();

	void run();
};
