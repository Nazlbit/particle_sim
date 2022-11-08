#pragma once
#include "window.hpp"
#include "simulation.hpp"
#include "math.hpp"

class particle_renderer
{
private:
	const window *m_wnd = nullptr;
	const simulation *m_sim = nullptr;
	GLuint m_shader_program = 0;
	GLuint m_VBO = 0;
	GLuint m_VAO = 0;
	GLint m_world_uniform = -1;
	GLint m_view_uniform = -1;
	GLint m_projection_uniform = -1;
	GLint m_particle_size_uniform = -1;
	mat4<float> m_world_matrix = identity_matrix();
	float m_particle_scale = 1.f;

	GLuint compile_shader(const char *shader_source, GLenum type);
	GLuint link_shader_program(const std::vector<GLuint> shaders);

	void safe_destroy() noexcept;
	void clean() noexcept;
	void move(particle_renderer&& other) noexcept;

public:
	particle_renderer() = default;
	particle_renderer(const window *const wnd, const simulation *const sim, const float &particle_scale);
	particle_renderer(particle_renderer &&other) noexcept;
	~particle_renderer();

	particle_renderer &operator=(particle_renderer &&other) noexcept;

	void configure_pipeline();
	void render();

	void rotate_world(const vec2<float> &delta);
};
