#include <vector>

#include "particle_renderer.hpp"
#include "math.hpp"
#include "helper.hpp"

GLuint particle_renderer::compile_shader(const char *shader_source, GLenum type)
{
	const auto &gl = m_wnd->gl();
	const GLuint shader = gl.CreateShader(type);
	gl.ShaderSource(shader, 1, &shader_source, NULL);
	gl.CompileShader(shader);

	// check for shader compile errors
	int success;
	char info_log[512];
	gl.GetShaderiv(shader, GL_COMPILE_STATUS, &success);
	if (!success)
	{
		gl.GetShaderInfoLog(shader, 512, NULL, info_log);
		THROW(info_log);
	}

	return shader;
}

GLuint particle_renderer::link_shader_program(const std::vector<GLuint> shaders)
{
	DEBUG_ASSERT(shaders.size() > 0);
	DEBUG_ASSERT(shaders.size() <= 6);

	const auto &gl = m_wnd->gl();
	const GLuint program = gl.CreateProgram();

	for(const GLuint shader : shaders)
	{
		gl.AttachShader(program, shader);
	}

	gl.LinkProgram(program);

	// check for linking errors
	int success;
	char info_log[512];
	gl.GetProgramiv(program, GL_LINK_STATUS, &success);
	if (!success)
	{
		gl.GetProgramInfoLog(program, 512, NULL, info_log);
		THROW(info_log);
	}

	return program;
}

particle_renderer::particle_renderer(const window *const wnd, const simulation *const sim) : m_wnd(wnd), m_sim(sim)
{
	const char *vertex_shader_source = "#version 410 core\n"
									   "layout (location = 0) in vec3 aPos;\n"
									   "void main()\n"
									   "{\n"
									   "   gl_Position = vec4(aPos, 1.0);\n"
									   "}\0";
	const char *fragment_shader_source = "#version 410 core\n"
										 "out vec4 FragColor;\n"
										 "void main()\n"
										 "{\n"
										 "   FragColor = vec4(1.0f, 1.0f, 1.0f, 0.05f);\n"
										 "}\n\0";

	const GLuint vertex_shader = compile_shader(vertex_shader_source, GL_VERTEX_SHADER);
	const GLuint fragment_shader = compile_shader(fragment_shader_source, GL_FRAGMENT_SHADER);

	// link shaders
	m_shader_program = link_shader_program({vertex_shader, fragment_shader});

	const auto &gl = m_wnd->gl();
	gl.DeleteShader(vertex_shader);
	gl.DeleteShader(fragment_shader);

	gl.GenVertexArrays(1, &m_VAO);
	gl.GenBuffers(1, &m_VBO);

	gl.BindVertexArray(m_VAO);

	gl.BindBuffer(GL_ARRAY_BUFFER, m_VBO);
	gl.BufferData(GL_ARRAY_BUFFER, sizeof(vec3_f) * m_sim->get_num_particles(), nullptr, GL_DYNAMIC_DRAW);

	gl.VertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(vec3_f), (void *)0);
	gl.EnableVertexAttribArray(0);
}

particle_renderer::particle_renderer(particle_renderer &&other) noexcept
	: m_wnd(other.m_wnd), m_sim(other.m_sim), m_shader_program(other.m_shader_program), m_VBO(other.m_VBO), m_VAO(other.m_VAO)
{
	other.clean();
}

particle_renderer &particle_renderer::operator=(particle_renderer &&other) noexcept
{
	safe_destroy();

	m_wnd = other.m_wnd;
	m_sim = other.m_sim;
	m_shader_program = other.m_shader_program;
	m_VBO = other.m_VBO;
	m_VAO = other.m_VAO;

	other.clean();

	return *this;
}

particle_renderer::~particle_renderer()
{
	safe_destroy();
}

void particle_renderer::configure_pipeline()
{
	const auto &gl = m_wnd->gl();
	gl.BindVertexArray(m_VAO);
	gl.UseProgram(m_shader_program);

	const dimensions viewport_size = m_wnd->get_framebuffer_size();
	const cube sim_cube = m_sim->get_sim_cube();
	gl.PointSize(m_sim->get_particle_size() / (sim_cube.half_size * 2) * viewport_size.height);
	gl.Enable(GL_BLEND);
	gl.BlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	gl.Enable(GL_MULTISAMPLE);
	gl.Viewport(0, 0, viewport_size.width, viewport_size.height);
}

void particle_renderer::render()
{
	const auto &gl = m_wnd->gl();
	gl.Clear(GL_COLOR_BUFFER_BIT);

	vec3_f *points = static_cast<vec3_f *>(gl.MapBuffer(GL_ARRAY_BUFFER, GL_WRITE_ONLY));
	const std::vector<vec3> &particles = m_sim->get_particles_positions();

	const cube sim_cube = m_sim->get_sim_cube();
	const dimensions viewport_size = m_wnd->get_framebuffer_size();

	for (size_t i = 0; i < particles.size(); ++i)
	{
		vec3 screen_coords = particles[i];
		screen_coords.x = screen_coords.x / sim_cube.half_size / viewport_size.width * viewport_size.height;
		screen_coords.y = screen_coords.y / sim_cube.half_size;
		screen_coords.z = 0;
		points[i] = screen_coords;
	}
	gl.UnmapBuffer(GL_ARRAY_BUFFER);

	gl.DrawArrays(GL_POINTS, 0, particles.size());
}

void particle_renderer::safe_destroy() noexcept
{
	if(m_wnd)
	{
		const auto &gl = m_wnd->gl();
		gl.DeleteVertexArrays(1, &m_VAO);
		gl.DeleteBuffers(1, &m_VBO);
		gl.DeleteProgram(m_shader_program);
	}

	clean();
}

void particle_renderer::clean() noexcept
{
	m_wnd = nullptr;
	m_sim = nullptr;
	m_shader_program = 0;
	m_VBO = 0;
	m_VAO = 0;
}