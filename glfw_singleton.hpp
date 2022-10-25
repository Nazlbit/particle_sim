#pragma once
#include <glad/gl.h>
#include <GLFW/glfw3.h>

#include "helper.hpp"

class glfw_singleton
{
	static glfw_singleton instance;

	glfw_singleton()
	{
		CRITICAL_ASSERT_M(glfwInit(), "Failed to initialize GLFW");
	}

	~glfw_singleton()
	{
		glfwTerminate();
	}
};

inline glfw_singleton glfw_singleton::instance;
