#pragma once
#include <glad/gl.h>
#include "simulation.hpp"

class particle_renderer
{
private:
	const GladGLContext *m_gl = nullptr;
	const simulation *m_sim = nullptr;
	GLuint m_shader_program = 0;
	GLuint m_VBO = 0;
	GLuint m_VAO = 0;

	GLuint compile_shader(const char *shader_source, GLenum type);
	GLuint link_shader_program(const std::vector<GLuint> shaders);

	void safe_destroy() noexcept;
	void clean() noexcept;

public:
	particle_renderer() = default;
	particle_renderer(const GladGLContext *gl, const simulation *sim);
	particle_renderer(particle_renderer &&other) noexcept;
	~particle_renderer();

	particle_renderer &operator=(particle_renderer &&other) noexcept;

	void configure_pipeline();
	void render();
};
