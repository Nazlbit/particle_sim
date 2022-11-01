#include <thread>
#include <sys/ioctl.h>
#include <unistd.h>

#include "application.hpp"

using namespace std::chrono_literals;

void application::init()
{
	struct winsize w;
	ioctl(STDOUT_FILENO, TIOCGWINSZ, &w);
	m_screen_width = w.ws_col;
	m_screen_height = w.ws_row;

	// debug
	if (m_screen_width == 0 || m_screen_height == 0)
	{
		m_screen_width = 100;
		m_screen_height = 50;
	}

	const double ratio = m_screen_width * char_ratio / m_screen_height;
	m_sim_width = sim_size * ratio;
	m_sim_height = sim_size;
	m_screen.reserve((m_screen_width + 1) * m_screen_height);

	const vec2 half_sim_size = vec2{m_sim_width, m_sim_height} * 0.5;

	m_simulation = std::make_unique<simulation>(rect{-half_sim_size, half_sim_size}, num_threads, dt, particle_size, g_const, wall_collision_cor, collision_max_force, drag_factor, cell_particles_limit, cell_proximity_factor);
}

void application::clear_screen()
{
	m_screen.assign((m_screen_width + 1) * m_screen_height, ' ');
	for (uint32_t i = 0; i < m_screen_height; i++)
	{
		m_screen[(m_screen_width + 1) * i + m_screen_width] = '\n';
	}
	m_screen[(m_screen_width + 1) * m_screen_height - 1] = '\0';
}

void application::draw_quad_tree()
{
	clear_screen();
	for (const vec2 &p : m_simulation->get_particles_positions())
	{
		const int x = (p.x / m_sim_width + 0.5) * m_screen_width;
		const int y = (p.y / m_sim_height + 0.5) * m_screen_height;
		if (x >= 0 && x < m_screen_width && y >= 0 && y < m_screen_height)
		{
			m_screen[(m_screen_width + 1) * y + x] = '#';
		}
	}

	system("clear");
	printf("%s", m_screen.data());
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
	while (true)
	{
		m_simulation->progress();
	}
}

void application::run()
{
	init();
	generate_particles();

	std::thread worker([this]
						{ work(); });
	while (true)
	{
		const auto t1 = std::chrono::steady_clock::now();

		draw_quad_tree();

		const auto draw_time = std::chrono::steady_clock::now() - t1;

		const auto sleep_duration = 1000'000'000ns / fps - draw_time;

		std::this_thread::sleep_for(sleep_duration);
	}

	worker.join();
}
