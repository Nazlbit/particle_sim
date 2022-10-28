#pragma once
#include <cstdio>
#include <cassert>
#include "exception.hpp"

#define ERROR(msg, ...) std::fprintf(stderr, "ERROR: " msg "\n", ##__VA_ARGS__)

#define WARNING(msg, ...) std::fprintf(stdout, "WARNING: " msg "\n", ##__VA_ARGS__)

#define INFO(msg, ...) std::fprintf(stdout, "INFO: " msg "\n", ##__VA_ARGS__)

#define THROW(msg) throw exception(msg);

#define THROW_PRINTF(msg, ...)                                                   \
	{                                                                            \
		char built_msg[exception::msg_size_limit];                               \
		std::snprintf(built_msg, exception::msg_size_limit, msg, ##__VA_ARGS__); \
		throw exception(built_msg);                                              \
	}

#define ASSERT_EX_M(expr, msg) \
	if (!expr)                 \
	{                          \
		THROW(msg);            \
	}

#define ASSERT_EX_M_PRINTF(expr, msg, ...) \
	if (!expr)                             \
	{                                      \
		THROW_PRINTF(msg, ##__VA_ARGS__);  \
	}

#define S1(x) #x
#define S2(x) S1(x)
#define LOCATION __FILE__ ":" S2(__LINE__)

#define ASSERT_EX(expr) ASSERT_EX_M(expr, "Assertion failure in " LOCATION ". Expression: " #expr)

#define DEBUG_ASSERT(expr) assert(expr)

#define CRITICAL_ASSERT_M(expr, msg, ...) \
	if (!expr)                            \
	{                                     \
		ERROR(msg, ##__VA_ARGS__);        \
		exit(-1);                         \
	}

#define CRITICAL_ASSERT(expr) CRITICAL_ASSERT_M(expr, "Critical assertion failure in " LOCATION ". Expression: " #expr)
