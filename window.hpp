#pragma once
#include "glfw_singleton.hpp"

class window
{
public:
	struct dimensions
	{
		int width = 0, height = 0;
	};

private:
	GLFWwindow *m_wnd = nullptr;
	GladGLContext m_gl{};
	dimensions m_size;

public:
	window() = default;

	window(const char *title, const int width, const int height, const bool fullscreen);

	window(window &&wnd);

	window &operator=(window &&wnd);

	~window();

	const GladGLContext &gl() const
	{
		return m_gl;
	}

	void make_context_current()
	{
		glfwMakeContextCurrent(m_wnd);
	}

	void swap_buffers()
	{
		glfwSwapBuffers(m_wnd);
	}

	bool should_close() const
	{
		return glfwWindowShouldClose(m_wnd);
	}

	static void poll_events()
	{
		glfwPollEvents();
	}

	dimensions get_size() const
	{
		return m_size;
	}
};
