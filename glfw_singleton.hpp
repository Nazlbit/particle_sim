#pragma once
#include <glad/gl.h>
#include <GLFW/glfw3.h>

#include "helper.hpp"

class glfw_singleton
{
	static glfw_singleton instance;

	static void error_callback(int code, const char *str);

	glfw_singleton()
	{
		CRITICAL_ASSERT_M(glfwInit(), "Failed to initialize GLFW");
		glfwSetErrorCallback(&glfw_singleton::error_callback);
	}

	~glfw_singleton()
	{
		glfwTerminate();
	}
};
