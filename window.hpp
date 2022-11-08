#pragma once
#include <functional>

#include "glfw_singleton.hpp"
#include "math.hpp"

class window
{
private:
	GLFWwindow *m_wnd = nullptr;
	GladGLContext m_gl{};
	dimensions m_size;
	std::function<void(const int&, const int&, const int&, const int&)> m_key_callback;
	std::function<void(const double &, const double &)> m_cursor_pos_callback;
	std::function<void(const int &, const int &, const int &)> m_mouse_button_callback;

	static void key_callback_static(GLFWwindow *wnd, int key, int scancode, int action, int mods);
	static void cursor_pos_callback_static(GLFWwindow *wnd, double x, double y);
	static void mouse_button_callback_static(GLFWwindow *wnd, int button, int action, int mods);

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
		dimensions size;
		glfwGetWindowSize(m_wnd, &size.width, &size.height);
		return size;
	}

	dimensions get_framebuffer_size() const
	{
		dimensions framebuffer_size;
		glfwGetFramebufferSize(m_wnd, &framebuffer_size.width, &framebuffer_size.height);
		return framebuffer_size;
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

	/* Key callback function prototype must be void(const int &, const int &, const int &) */
	void set_mouse_button_callback(std::function<void(const int &, const int &, const int &)> mouse_button_callback)
	{
		m_mouse_button_callback = std::move(mouse_button_callback);
	}

	void close()
	{
		glfwSetWindowShouldClose(m_wnd, GLFW_TRUE);
	}

	vec2<double> get_cursor_pos() const
	{
		vec2<double> pos;
		glfwGetCursorPos(m_wnd, &pos.x, &pos.y);
		return pos;
	}
};
