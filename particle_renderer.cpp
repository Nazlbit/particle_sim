#include <vector>

#include "particle_renderer.hpp"
#include "math.hpp"
#include "helper.hpp"

GLuint particle_renderer::compile_shader(const char *shader_source, GLenum type)
{
	const GLuint shader = m_gl->CreateShader(type);
	m_gl->ShaderSource(shader, 1, &shader_source, NULL);
	m_gl->CompileShader(shader);

	// check for shader compile errors
	int success;
	char info_log[512];
	m_gl->GetShaderiv(shader, GL_COMPILE_STATUS, &success);
	if (!success)
	{
		m_gl->GetShaderInfoLog(shader, 512, NULL, info_log);
		THROW(info_log);
	}

	return shader;
}

GLuint particle_renderer::link_shader_program(const std::vector<GLuint> shaders)
{
	DEBUG_ASSERT(shaders.size() > 0);
	DEBUG_ASSERT(shaders.size() <= 6);

	const GLuint program = m_gl->CreateProgram();

	for(const GLuint shader : shaders)
	{
		m_gl->AttachShader(program, shader);
	}

	m_gl->LinkProgram(program);

	// check for linking errors
	int success;
	char info_log[512];
	m_gl->GetProgramiv(program, GL_LINK_STATUS, &success);
	if (!success)
	{
		m_gl->GetProgramInfoLog(program, 512, NULL, info_log);
		THROW(info_log);
	}

	return program;
}

particle_renderer::particle_renderer(const GladGLContext *gl, const simulation *sim) : m_gl(gl), m_sim(sim)
{
	const char *vertex_shader_source = "#version 410 core\n"
									   "layout (location = 0) in vec2 aPos;\n"
									   "void main()\n"
									   "{\n"
									   "   gl_Position = vec4(aPos, 0.0, 1.0);\n"
									   "}\0";
	const char *fragment_shader_source = "#version 410 core\n"
										 "out vec4 FragColor;\n"
										 "void main()\n"
										 "{\n"
										 "   FragColor = vec4(1.0f, 1.0f, 1.0f, 0.1f);\n"
										 "}\n\0";

	const GLuint vertex_shader = compile_shader(vertex_shader_source, GL_VERTEX_SHADER);
	const GLuint fragment_shader = compile_shader(fragment_shader_source, GL_FRAGMENT_SHADER);

	// link shaders
	m_shader_program = link_shader_program({vertex_shader, fragment_shader});

	m_gl->DeleteShader(vertex_shader);
	m_gl->DeleteShader(fragment_shader);

	m_gl->GenVertexArrays(1, &m_VAO);
	m_gl->GenBuffers(1, &m_VBO);

	m_gl->BindVertexArray(m_VAO);

	m_gl->BindBuffer(GL_ARRAY_BUFFER, m_VBO);
	m_gl->BufferData(GL_ARRAY_BUFFER, sizeof(vec2_f) * m_sim->get_num_particles(), nullptr, GL_DYNAMIC_DRAW);

	m_gl->VertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 2 * sizeof(float), (void *)0);
	m_gl->EnableVertexAttribArray(0);
}

particle_renderer::particle_renderer(particle_renderer &&other) noexcept
	: m_gl(other.m_gl), m_sim(other.m_sim), m_shader_program(other.m_shader_program), m_VBO(other.m_VBO), m_VAO(other.m_VAO)
{
	other.clean();
}

particle_renderer &particle_renderer::operator=(particle_renderer &&other) noexcept
{
	safe_destroy();

	m_gl = other.m_gl;
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
	m_gl->BindVertexArray(m_VAO);
	m_gl->UseProgram(m_shader_program);
	m_gl->PointSize(3);
	m_gl->Enable(GL_BLEND);
	m_gl->BlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	m_gl->Enable(GL_MULTISAMPLE);
}

void particle_renderer::render()
{
	m_gl->Clear(GL_COLOR_BUFFER_BIT);

	vec2_f *points = static_cast<vec2_f *>(m_gl->MapBuffer(GL_ARRAY_BUFFER, GL_WRITE_ONLY));
	const std::vector<vec2> &particles = m_sim->get_particles_positions();

	const rect sim_rect = m_sim->get_sim_rect();
	const vec2 sim_size = sim_rect.size();
	for (size_t i = 0; i < particles.size(); ++i)
	{
		vec2 screen_coords = particles[i];
		screen_coords.x = screen_coords.x / sim_size.x * 2;
		screen_coords.y = screen_coords.y / sim_size.y * 2;
		points[i] = screen_coords;
	}
	m_gl->UnmapBuffer(GL_ARRAY_BUFFER);

	m_gl->DrawArrays(GL_POINTS, 0, particles.size());
}

void particle_renderer::safe_destroy() noexcept
{
	if(m_gl)
	{
		m_gl->DeleteVertexArrays(1, &m_VAO);
		m_gl->DeleteBuffers(1, &m_VBO);
		m_gl->DeleteProgram(m_shader_program);
	}

	clean();
}

void particle_renderer::clean() noexcept
{
	m_gl = nullptr;
	m_sim = nullptr;
	m_shader_program = 0;
	m_VBO = 0;
	m_VAO = 0;
}