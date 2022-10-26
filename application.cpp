#include <thread>
#include <sys/ioctl.h>
#include <unistd.h>

#include "application.hpp"

using namespace std::chrono_literals;

void application::init()
{
	// struct winsize w;
	// ioctl(STDOUT_FILENO, TIOCGWINSZ, &w);
	// m_screen_width = w.ws_col;
	// m_screen_height = w.ws_row;

	// // debug
	// if (m_screen_width == 0 || m_screen_height == 0)
	// {
	// 	m_screen_width = 100;
	// 	m_screen_height = 50;
	// }

	const double ratio = 1; //m_screen_width *char_ratio / m_screen_height;
	m_sim_width = sim_size * ratio;
	m_sim_height = sim_size;
	//m_screen.reserve((m_screen_width + 1) * m_screen_height);

	m_simulation = std::make_unique<simulation>(rect{vec2{0, 0}, vec2{m_sim_width, m_sim_height} * 0.5}, num_threads, dt, particle_size, g_const, wall_collision_cor, collision_max_force, drag_factor, cell_particles_limit, cell_proximity_factor);
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
	for (const vec2 &p : m_simulation->get_particles_pos())
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
		//const auto t1 = std::chrono::steady_clock::now();
		m_simulation->progress();
		//const auto t2 = std::chrono::steady_clock::now();
		//const double dt = std::chrono::duration<double>(t2-t1).count();
		//printf("dt:%f\n", dt);
	}
}

void application::run()
{
	m_wnd = window("particle_sim", 1300, 1300);

	const GladGLContext &gl = m_wnd.gl();

	const char *vertexShaderSource = "#version 410 core\n"
									 "layout (location = 0) in vec2 aPos;\n"
									 "void main()\n"
									 "{\n"
									 "   gl_Position = vec4(aPos, 0.0, 1.0);\n"
									 "}\0";
	const char *fragmentShaderSource = "#version 410 core\n"
									   "out vec4 FragColor;\n"
									   "void main()\n"
									   "{\n"
									   "   FragColor = vec4(1.0f, 1.0f, 1.0f, 0.2f);\n"
									   "}\n\0";

	unsigned int vertexShader = gl.CreateShader(GL_VERTEX_SHADER);
	gl.ShaderSource(vertexShader, 1, &vertexShaderSource, NULL);
	gl.CompileShader(vertexShader);
	// check for shader compile errors
	int success;
	char infoLog[512];
	gl.GetShaderiv(vertexShader, GL_COMPILE_STATUS, &success);
	if (!success)
	{
		gl.GetShaderInfoLog(vertexShader, 512, NULL, infoLog);
		THROW(infoLog);
	}
	// fragment shader
	unsigned int fragmentShader = gl.CreateShader(GL_FRAGMENT_SHADER);
	gl.ShaderSource(fragmentShader, 1, &fragmentShaderSource, NULL);
	gl.CompileShader(fragmentShader);
	// check for shader compile errors
	gl.GetShaderiv(fragmentShader, GL_COMPILE_STATUS, &success);
	if (!success)
	{
		gl.GetShaderInfoLog(fragmentShader, 512, NULL, infoLog);
		THROW(infoLog);
	}
	// link shaders
	unsigned int shaderProgram = gl.CreateProgram();
	gl.AttachShader(shaderProgram, vertexShader);
	gl.AttachShader(shaderProgram, fragmentShader);
	gl.LinkProgram(shaderProgram);
	// check for linking errors
	gl.GetProgramiv(shaderProgram, GL_LINK_STATUS, &success);
	if (!success)
	{
		gl.GetProgramInfoLog(shaderProgram, 512, NULL, infoLog);
		THROW(infoLog);
	}
	gl.DeleteShader(vertexShader);
	gl.DeleteShader(fragmentShader);
	gl.UseProgram(shaderProgram);

	unsigned int VBO, VAO;
	gl.GenVertexArrays(1, &VAO);
	gl.GenBuffers(1, &VBO);
	// bind the Vertex Array Object first, then bind and set vertex buffer(s), and then configure vertex attributes(s).
	gl.BindVertexArray(VAO);

	gl.BindBuffer(GL_ARRAY_BUFFER, VBO);
	gl.BufferData(GL_ARRAY_BUFFER, sizeof(vec2_f) * num_particles, nullptr, GL_DYNAMIC_DRAW);
	vec2_f *points = static_cast<vec2_f *>(gl.MapBuffer(GL_ARRAY_BUFFER, GL_WRITE_ONLY));

	gl.VertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 2 * sizeof(float), (void *)0);
	gl.EnableVertexAttribArray(0);

	// note that this is allowed, the call to glVertexAttribPointer registered VBO as the vertex attribute's bound vertex buffer object so afterwards we can safely unbind
	//gl.BindBuffer(GL_ARRAY_BUFFER, 0);

	// remember: do NOT unbind the EBO while a VAO is active as the bound element buffer object IS stored in the VAO; keep the EBO bound.
	// glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);

	// You can unbind the VAO afterwards so other VAO calls won't accidentally modify this VAO, but this rarely happens. Modifying other
	// VAOs requires a call to glBindVertexArray anyways so we generally don't unbind VAOs (nor VBOs) when it's not directly necessary.
	//gl.BindVertexArray(0);

	gl.Enable(GL_BLEND);
	gl.BlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	gl.Enable(GL_MULTISAMPLE);
	init();
	generate_particles();

	std::thread worker([this]
					   { work(); });

	gl.PointSize(3);
	while (!m_wnd.should_close())
	{
		window::poll_events();

		gl.Clear(GL_COLOR_BUFFER_BIT);

		//gl.UseProgram(shaderProgram);
		//gl.BindVertexArray(VAO); // seeing as we only have a single VAO there's no need to bind it every time, but we'll do so to keep things a bit more organized
		const std::vector<vec2> &particles = m_simulation->get_particles_pos();
		for (size_t i = 0; i < num_particles; ++i)
		{
			points[i] = particles[i] / (sim_size * 0.5);
		}
		gl.DrawArrays(GL_POINTS, 0, num_particles);

		m_wnd.swap_buffers();
	}

	gl.UnmapBuffer(GL_ARRAY_BUFFER);
	gl.DeleteVertexArrays(1, &VAO);
	gl.DeleteBuffers(1, &VBO);
	gl.DeleteProgram(shaderProgram);

	// init();
	// generate_particles();

	// std::thread worker([this]
	// 					{ work(); });
	// while (true)
	// {
	// 	const auto t1 = std::chrono::steady_clock::now();

	// 	draw_quad_tree();

	// 	const auto draw_time = std::chrono::steady_clock::now() - t1;

	// 	const auto sleep_duration = 1000'000'000ns / fps - draw_time;

	// 	std::this_thread::sleep_for(sleep_duration);
	// }

	// worker.join();
}
