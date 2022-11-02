#include <thread>
#include <sys/ioctl.h>
#include <unistd.h>

#include "application.hpp"

using namespace std::chrono_literals;

void application::init()
{
	const window::dimensions wnd_size = m_wnd.get_size();
	const double ratio = (double)wnd_size.width / wnd_size.height;
	m_sim_width = sim_size * ratio;
	m_sim_height = sim_size;

	const vec2 half_sim_size = vec2{m_sim_width, m_sim_height} * 0.5;

	m_simulation = std::make_unique<simulation>(rect{-half_sim_size, half_sim_size}, num_threads, dt, particle_size, g_const, wall_collision_cor, collision_max_force, drag_factor, cell_particles_limit, cell_proximity_factor);
}

void application::generate_particles()
{
	for (size_t i = 0; i < num_particles - 2; ++i)
	{
		particle p;
		p.pos = {random_double(-0.5, 0.5) * m_sim_width, random_double(-0.5, 0.5) * m_sim_height};
		p.pos = p.pos * generation_scale;

		p.v = vec2{p.pos.y, -p.pos.x} * initial_velocity_factor;

		p.m = 1;
		p.size = particle_size;
		m_simulation->add(p);
	}

	for (size_t i = 0; i < 2; ++i)
	{
		particle p;
		p.pos = {random_double(-0.5, 0.5) * m_sim_width, random_double(-0.5, 0.5) * m_sim_height};
		p.pos = p.pos * generation_scale;

		p.v = vec2{p.pos.y, -p.pos.x} * initial_velocity_factor;

		p.m = 10000;
		p.size = particle_size * 10;
		m_simulation->add(p);
	}
}

void application::work()
{
	double dt = 0;
	int n = 0;
	while (true)
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
	m_wnd = window("particle_sim", 0, 0, true);

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
									   "   FragColor = vec4(1.0f, 1.0f, 1.0f, 0.025f);\n"
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

	gl.VertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 2 * sizeof(float), (void *)0);
	gl.EnableVertexAttribArray(0);

	gl.PointSize(particle_size / sim_size * 1600);
	gl.Enable(GL_BLEND);
	gl.BlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	gl.Enable(GL_MULTISAMPLE);

	init();
	generate_particles();

	std::thread worker([this]
					   { work(); });

	while (!m_wnd.should_close())
	{
		window::poll_events();

		gl.Clear(GL_COLOR_BUFFER_BIT);

		vec2_f *points = static_cast<vec2_f *>(gl.MapBuffer(GL_ARRAY_BUFFER, GL_WRITE_ONLY));
		const std::vector<vec2> &particles = m_simulation->get_particles_positions();
		for (size_t i = 0; i < num_particles; ++i)
		{
			vec2 screen_coords = particles[i];
			screen_coords.x = screen_coords.x / m_sim_width * 2;
			screen_coords.y = screen_coords.y / m_sim_height * 2;
			points[i] = screen_coords;
		}
		gl.UnmapBuffer(GL_ARRAY_BUFFER);

		gl.DrawArrays(GL_POINTS, 0, num_particles);

		m_wnd.swap_buffers();
	}

	gl.DeleteVertexArrays(1, &VAO);
	gl.DeleteBuffers(1, &VBO);
	gl.DeleteProgram(shaderProgram);

	// worker.join();
}
