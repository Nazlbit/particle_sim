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
	std::function<void(const int&, const int&, const int&, const int&)> m_key_callback;
	std::function<void(const double&, const double&)> m_cursor_pos_callback;

	static void key_callback_static(GLFWwindow *wnd, int key, int scancode, int action, int mods);
	static void cursor_pos_callback_static(GLFWwindow *wnd, double x, double y);

	void move(window &&other);

public:
	window() = default;

	window(const char *title, const int width, const int height, const bool fullscreen);

	window(window &&other);

	window &operator=(window &&other);

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

	/* Key callback function prototype must be void(const int &key, const int &scancode, const int &action, const int &mods) */
	void set_key_callback(std::function<void(const int&, const int&, const int&, const int&)> key_callback)
	{
		m_key_callback = std::move(key_callback);
	}

	/* Key callback function prototype must be void(const double &x, const double &y) */
	void set_cursor_pos_callback(std::function<void(const double&, const double&)> cursor_pos_callback)
	{
		m_cursor_pos_callback = std::move(cursor_pos_callback);
	}

	void close()
	{
		glfwSetWindowShouldClose(m_wnd, GLFW_TRUE);
	}
};
