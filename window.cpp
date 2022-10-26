#include "window.hpp"

window::window(const char *title, int width, int height)
{
	// MacOS only supports OpenGL 4.1
	glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
	glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 1);
	glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
	glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
	//glfwWindowHint(GLFW_RESIZABLE, GL_FALSE);
	glfwWindowHint(GLFW_SAMPLES, 4);
	m_wnd = glfwCreateWindow(width, height, title, nullptr, nullptr);
	ASSERT_EX_M(m_wnd, "Failed to create a window");

	glfwMakeContextCurrent(m_wnd);

	ASSERT_EX_M(gladLoadGLContext(&m_gl, glfwGetProcAddress), "Failed to initialize OpenGL context");
}

window::~window()
{
	glfwDestroyWindow(m_wnd);
}

window::window(window &&wnd)
{
	m_gl = wnd.m_gl;
	m_wnd = wnd.m_wnd;
	wnd.m_wnd = nullptr;
}

window &window::operator=(window &&wnd)
{
	glfwDestroyWindow(m_wnd);
	m_gl = wnd.m_gl;
	m_wnd = wnd.m_wnd;
	wnd.m_wnd = nullptr;
	return *this;
}