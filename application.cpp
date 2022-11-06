#include <thread>
#include <algorithm>

#include "application.hpp"

using namespace std::chrono_literals;

void application::init()
{
	m_wnd = window("particle_sim", 600, 600, false);

	m_wnd.set_key_callback([this](const int &key, const int &scancode, const int &action, const int &mods)
						   { window_key_callback(key, scancode, action, mods); });

	m_wnd.set_cursor_pos_callback([this](const double &x, const double &y)
								  { window_cursor_pos_callback(x, y); });

	m_wnd.set_mouse_button_callback([this](const int &button, const int &action, const int &mods)
									{ window_mouse_button_callback(button, action, mods); });

	m_simulation = std::make_unique<simulation>(cube{{0, 0}, sim_size * 0.5}, num_threads, dt, particle_size, g_const, wall_collision_cor, collision_max_force, drag_factor, cell_particles_limit, cell_proximity_factor);

	generate_particles();

	m_wnd.make_context_current();
	m_renderer = particle_renderer(&m_wnd, m_simulation.get());
}

void application::generate_particles()
{
	for (size_t i = 0; i < num_particles; ++i)
	{
		particle p;
		p.pos = {random_double(-0.5, 0.5), random_double(-0.5, 0.5), random_double(-0.5, 0.5)};
		p.pos = p.pos * (sim_size * generation_scale);

		p.v = vec3{p.pos.y, -p.pos.x, 0} * initial_velocity_factor;

		m_simulation->add(p);
	}
}

void application::run()
{
	m_simulation->start();
	while (!m_wnd.should_close())
	{
		window::poll_events();

		m_renderer.configure_pipeline();
		m_renderer.render();

		m_wnd.swap_buffers();
	}
}

application::application()
{
	init();
}

application::~application()
{
}

void application::window_key_callback(const int &key, const int &scancode, const int &action, const int &mods)
{
	if(key == GLFW_KEY_ESCAPE && action == GLFW_PRESS)
	{
		m_wnd.close();
	}
}

void application::window_cursor_pos_callback(const double &x, const double &y)
{
}

void application::window_mouse_button_callback(const int &button, const int &action, const int &mods)
{
}
