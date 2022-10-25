#include "window.hpp"

window::window(const char *title, int width, int height)
{
	// MacOS only supports OpenGL 4.1
	glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
	glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 1);
	glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
	glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
	m_wnd = glfwCreateWindow(width, height, title, nullptr, nullptr);
	ASSERT_EX_M(m_wnd, "Failed to create a window");

	glfwMakeContextCurrent(m_wnd);

	int version = gladLoadGLContext(&m_gl, glfwGetProcAddress);
}
