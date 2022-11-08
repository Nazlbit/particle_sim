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

particle_renderer::particle_renderer(const window *const wnd, const simulation *const sim, const float &particle_scale) : m_wnd(wnd), m_sim(sim), m_particle_scale(particle_scale)
{
	const char *vertex_shader_source = "#version 410 core\n"
									   "layout (location = 0) in vec3 aPos;\n"
									   "uniform mat4 World;\n"
									   "uniform mat4 View;\n"
									   "uniform mat4 Projection;\n"
									   "uniform float ParticleSize;\n"
									   "void main()\n"
									   "{\n"
									   "   vec4 pos = Projection * View * World * vec4(aPos, 1.0);\n"
									   "   gl_Position = pos;\n"
									   "   gl_PointSize = ParticleSize / pos.w;\n"
									   "}\0";
	const char *fragment_shader_source = "#version 410 core\n"
										 "out vec4 FragColor;\n"
										 "void main()\n"
										 "{\n"
										 "   FragColor = vec4(1.0f, 1.0f, 1.0f, 0.025f);\n"
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
	gl.BufferData(GL_ARRAY_BUFFER, sizeof(vec3<float>) * m_sim->get_num_particles(), nullptr, GL_DYNAMIC_DRAW);

	gl.VertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(vec3<float>), (void *)0);
	gl.EnableVertexAttribArray(0);

	m_world_uniform = gl.GetUniformLocation(m_shader_program, "World");
	m_view_uniform = gl.GetUniformLocation(m_shader_program, "View");
	m_projection_uniform = gl.GetUniformLocation(m_shader_program, "Projection");
	m_particle_size_uniform = gl.GetUniformLocation(m_shader_program, "ParticleSize");
}

void particle_renderer::clean() noexcept
{
	m_wnd = nullptr;
}

void particle_renderer::move(particle_renderer &&other) noexcept
{
	m_wnd = other.m_wnd;
	m_sim = other.m_sim;
	m_shader_program = other.m_shader_program;
	m_VBO = other.m_VBO;
	m_VAO = other.m_VAO;
	m_world_uniform = other.m_world_uniform;
	m_view_uniform = other.m_view_uniform;
	m_projection_uniform = other.m_projection_uniform;
	m_particle_size_uniform = other.m_particle_size_uniform;
	m_world_matrix = other.m_world_matrix;
	m_particle_scale = other.m_particle_scale;

	other.clean();
}

void particle_renderer::safe_destroy() noexcept
{
	if (m_wnd)
	{
		const auto &gl = m_wnd->gl();
		gl.DeleteVertexArrays(1, &m_VAO);
		gl.DeleteBuffers(1, &m_VBO);
		gl.DeleteProgram(m_shader_program);
	}

	clean();
}

particle_renderer::particle_renderer(particle_renderer &&other) noexcept
{
	move(std::move(other));
}

particle_renderer &particle_renderer::operator=(particle_renderer &&other) noexcept
{
	safe_destroy();
	move(std::move(other));

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
	const double sim_size = m_sim->get_size();
	gl.Enable(GL_BLEND);
	gl.BlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	gl.Enable(GL_MULTISAMPLE);
	gl.Enable(GL_PROGRAM_POINT_SIZE);
	gl.Viewport(0, 0, viewport_size.width, viewport_size.height);

	const float fov = degrees_to_radians<float>(70);
	const float distance = sim_size * 0.5f / sinf(fov * 0.5);
	const mat4<float> view_matrix = look_to_matrix({0, 0, -distance}, {0, 0, 1}, {0, 1, 0});
	const mat4<float> projection_matrix = perspective_projection_matrix(fov, (distance - sim_size* 0.5) * 0.9, (distance +sim_size * 0.5) * 1.1, (float)viewport_size.width / viewport_size.height);
	gl.UniformMatrix4fv(m_world_uniform, 1, GL_TRUE, reinterpret_cast<const GLfloat *>(&m_world_matrix));
	gl.UniformMatrix4fv(m_view_uniform, 1, GL_TRUE, reinterpret_cast<const GLfloat *>(&view_matrix));
	gl.UniformMatrix4fv(m_projection_uniform, 1, GL_TRUE, reinterpret_cast<const GLfloat *>(&projection_matrix));
	gl.Uniform1f(m_particle_size_uniform, m_sim->get_particle_size() * viewport_size.height / tanf(fov / 2) * m_particle_scale);
}

void particle_renderer::render()
{
	const auto &gl = m_wnd->gl();
	gl.Clear(GL_COLOR_BUFFER_BIT);

	vec3<float> *points = static_cast<vec3<float> *>(gl.MapBuffer(GL_ARRAY_BUFFER, GL_WRITE_ONLY));
	const std::vector<vec3<double>> &particles = m_sim->get_particles_positions();

	const double sim_half_size = m_sim->get_size() / 2;
	const dimensions viewport_size = m_wnd->get_framebuffer_size();

	for (size_t i = 0; i < particles.size(); ++i)
	{
		points[i] = vec3<float>::type_cast(particles[i]);
	}
	gl.UnmapBuffer(GL_ARRAY_BUFFER);

	gl.DrawArrays(GL_POINTS, 0, particles.size());
}

void particle_renderer::rotate_world(const vec2<float> &delta)
{
	m_world_matrix = m_world_matrix * x_rotation_matrix(delta.y) * y_rotation_matrix(delta.x);
}
