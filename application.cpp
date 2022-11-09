#include <thread>
#include <algorithm>
#include <cmath>

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

	m_wnd.set_scroll_callback([this](const double &xoffset, const double &yoffset)
							  { window_scroll_callback(xoffset, yoffset); });

	m_simulation = std::make_unique<simulation>(sim_size, num_threads, dt, particle_size, g_const, wall_collision_cor, collision_max_force, drag_factor, cell_particles_limit, cell_proximity_factor);

	generate_particles();

	m_wnd.make_context_current();
	m_renderer = particle_renderer(&m_wnd, m_simulation.get(), particle_scale, degrees_to_radians(fov));

	m_cursor.pos = m_wnd.get_cursor_pos();
}

void application::generate_particles()
{
	for (size_t i = 0; i < num_particles; ++i)
	{
		/* Uniform points distribution inside the volume of a sphere:
		 * https://math.stackexchange.com/questions/87230/picking-random-points-in-the-volume-of-sphere-with-uniform-probability
		 */
		const double x = normal_random_double(0, 1);
		const double y = normal_random_double(0, 1);
		const double z = normal_random_double(0, 1);
		const double r = uniform_random_double(0, 1);

		particle p;
		p.pos = {x, y, z};
		p.pos = p.pos / sqrt(p.pos * p.pos) * pow(r, 1.0 / 3);
		p.pos = p.pos * (sim_size * 0.5 * generation_scale);

		p.v = vec3<double>{p.pos.y, -p.pos.x, 0} * initial_velocity_factor;

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
	const vec2<double> new_pos = {x, y};

	if (m_cursor.mlb)
	{
		vec2<double> delta = (new_pos - m_cursor.pos) * 0.01;

		m_renderer.rotate_world(vec2<float>::type_cast(delta));
	}

	m_cursor.pos = new_pos;
}

void application::window_mouse_button_callback(const int &button, const int &action, const int &mods)
{
	switch(button)
	{
	case GLFW_MOUSE_BUTTON_LEFT:
		m_cursor.mlb = (action == GLFW_PRESS);
		break;
	};
}

void application::window_scroll_callback(const double &xoffset, const double &yoffset)
{
	m_renderer.set_zoom(m_renderer.get_zoom() * powf(0.99, yoffset));
}