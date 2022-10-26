#include "glfw_singleton.hpp"
#include "helper.hpp"

glfw_singleton glfw_singleton::instance;

void glfw_singleton::error_callback(int code, const char *str)
{
	ERROR("%s", str);
}
