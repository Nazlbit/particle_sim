#include "window.hpp"

window::window(const char *title, const int width, const int height, const bool fullscreen)
{
	// MacOS only supports OpenGL 4.1
	glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
	glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 1);
	glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
	glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
	//glfwWindowHint(GLFW_RESIZABLE, GL_FALSE);
	glfwWindowHint(GLFW_SAMPLES, 4);

	GLFWmonitor *monitor = nullptr;
	if (fullscreen)
	{
		monitor = glfwGetPrimaryMonitor();
		const GLFWvidmode *mode = glfwGetVideoMode(monitor);

		glfwWindowHint(GLFW_RED_BITS, mode->redBits);
		glfwWindowHint(GLFW_GREEN_BITS, mode->greenBits);
		glfwWindowHint(GLFW_BLUE_BITS, mode->blueBits);
		glfwWindowHint(GLFW_REFRESH_RATE, mode->refreshRate);
		glfwWindowHint(GLFW_AUTO_ICONIFY, GLFW_FALSE);

		m_size.width = mode->width;
		m_size.height = mode->height;
	}
	else
	{
		m_size.width = width;
		m_size.height = height;
	}
	m_wnd = glfwCreateWindow(m_size.width, m_size.height, title, monitor, nullptr);
	ASSERT_EX_M(m_wnd, "Failed to create a window");

	glfwMakeContextCurrent(m_wnd);

	ASSERT_EX_M(gladLoadGLContext(&m_gl, glfwGetProcAddress), "Failed to initialize OpenGL context");

	glfwSetWindowUserPointer(m_wnd, this);
	glfwSetKeyCallback(m_wnd, &window::key_callback_static);
	glfwSetCursorPosCallback(m_wnd, &window::cursor_pos_callback_static);
	glfwSetMouseButtonCallback(m_wnd, &window::mouse_button_callback_static);
}

window::~window()
{
	glfwDestroyWindow(m_wnd);
}

void window::move(window &&other)
{
	m_gl = other.m_gl;
	m_wnd = other.m_wnd;
	m_size = other.m_size;
	other.m_wnd = nullptr;
	m_key_callback = std::move(other.m_key_callback);
	m_cursor_pos_callback = std::move(other.m_cursor_pos_callback);
	m_mouse_button_callback = std::move(other.m_mouse_button_callback);
	glfwSetWindowUserPointer(m_wnd, this);
}

window::window(window &&other)
{
	move(std::move(other));
}

window &window::operator=(window &&other)
{
	glfwDestroyWindow(m_wnd);
	move(std::move(other));
	return *this;
}

void window::key_callback_static(GLFWwindow *wnd, int key, int scancode, int action, int mods)
{
	window *const instance = static_cast<window*>(glfwGetWindowUserPointer(wnd));
	if(instance->m_key_callback)
	{
		instance->m_key_callback(key, scancode, action, mods);
	}
}

void window::cursor_pos_callback_static(GLFWwindow *wnd, double x, double y)
{
	window *const instance = static_cast<window *>(glfwGetWindowUserPointer(wnd));
	if (instance->m_cursor_pos_callback)
	{
		instance->m_cursor_pos_callback(x, y);
	}
}

void window::mouse_button_callback_static(GLFWwindow *wnd, int button, int action, int mods)
{
	window *const instance = static_cast<window *>(glfwGetWindowUserPointer(wnd));
	if (instance->m_mouse_button_callback)
	{
		instance->m_mouse_button_callback(button, action, mods);
	}
}
