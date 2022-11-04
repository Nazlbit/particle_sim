#pragma once
#include <functional>

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
	std::function<void(int, int, int, int)> m_key_callback;

	static void key_callback_static(GLFWwindow *wnd, int key, int scancode, int action, int mods);

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

	/* Key callback function prototype must be void(int key, int scancode, int action, int mods) */
	void set_key_callback(std::function<void(int, int, int, int)> key_callback)
	{
		m_key_callback = std::move(key_callback);
	}

	void close()
	{
		glfwSetWindowShouldClose(m_wnd, GLFW_TRUE);
	}
};
