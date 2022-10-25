#pragma once
#include "glfw_singleton.hpp"

class window
{
private:
	GLFWwindow *m_wnd;
	GladGLContext m_gl{};

public:
	window() = default;
	window(const char *title, int width, int height);

	const GladGLContext &gl() const
	{
		return m_gl;
	}
};
