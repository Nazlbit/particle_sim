#pragma once
#include "glfw_singleton.hpp"

class window
{
private:
	GLFWwindow *m_wnd = nullptr;
	GladGLContext m_gl{};

public:
	window() = default;

	window(const char *title, int width, int height);

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
};
