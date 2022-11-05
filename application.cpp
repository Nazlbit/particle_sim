#include <thread>
#include <algorithm>

#include "application.hpp"

using namespace std::chrono_literals;

void application::init()
{
	m_wnd = window("particle_sim", 0, 0, true);

	m_wnd.set_key_callback([this](const int &key, const int &scancode, const int &action, const int &mods)
						   { window_key_callback(key, scancode, action, mods); });

	m_wnd.set_cursor_pos_callback([this](const double &x, const double &y)
								  { window_cursor_pos_callback(x, y); });

	m_wnd.set_mouse_button_callback([this](const int &button, const int &action, const int &mods)
									{ window_mouse_button_callback(button, action, mods); });

	const window::dimensions wnd_size = m_wnd.get_size();
	const double ratio = (double)wnd_size.width / wnd_size.height;
	m_sim_width = sim_size * ratio;
	m_sim_height = sim_size;

	const vec2 half_sim_size = vec2{m_sim_width, m_sim_height} * 0.5;

	m_simulation = std::make_unique<simulation>(rect{-half_sim_size, half_sim_size}, num_threads, dt, particle_size, g_const, wall_collision_cor, collision_max_force, drag_factor, cell_particles_limit, cell_proximity_factor);

	generate_particles();

	m_renderer = particle_renderer(&m_wnd.gl(), m_simulation.get());

	m_renderer.configure_pipeline();

	m_worker = std::thread([this]
						   { work(); });
}

void application::generate_particles()
{
	for (size_t i = 0; i < num_particles; ++i)
	{
		particle p;
		p.pos = {random_double(-0.5, 0.5) * m_sim_width, random_double(-0.5, 0.5) * m_sim_height};
		p.pos = p.pos * generation_scale;

		p.v = vec2{p.pos.y, -p.pos.x} * initial_velocity_factor;

		m_simulation->add(p);
	}
}

void application::work()
{
	double dt = 0;
	int n = 0;
	while (m_worker_active)
	{
		const auto t1 = std::chrono::steady_clock::now();
		m_simulation->progress();
		const auto t2 = std::chrono::steady_clock::now();
		dt += std::chrono::duration<double>(t2-t1).count();
		++n;
		if(dt >= 1)
		{
			printf("FPS: %f\n", n/dt);
			dt = 0;
			n = 0;
		}
	}
}

void application::run()
{
	while (!m_wnd.should_close())
	{
		window::poll_events();

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
	m_worker_active = false;
	m_worker.join();
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
	const auto wnd_size = m_wnd.get_size();
	const double x_norm = std::clamp(x / wnd_size.width, 0.0, 1.0) - 0.5;
	const double y_norm = 0.5 - std::clamp(y / wnd_size.height, 0.0, 1.0);

	printf("Cursor pos normalized: %f:%f\n", x_norm, y_norm);
}

void application::window_mouse_button_callback(const int &button, const int &action, const int &mods)
{
	if(button == GLFW_MOUSE_BUTTON_LEFT)
	{
		if (action == GLFW_PRESS)
		{
			printf("Left mouse button press\n");
		}
		else
		{
			printf("Left mouse button release\n");
		}
	}
}
