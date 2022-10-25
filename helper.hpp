#pragma once
#include <cstdio>
#include <cassert>
#include "exception.hpp"

#define ERROR(msg, ...) fprintf(stderr, "ERROR: " msg "\n", ##__VA_ARGS__)

#define WARNING(msg, ...) fprintf(stdout, "WARNING: " msg "\n", ##__VA_ARGS__)

#define INFO(msg, ...) fprintf(stdout, "INFO: " msg "\n", ##__VA_ARGS__)

#define THROW(msg) throw exception(msg)

#define ASSERT_EX_M(expr, msg) (expr ? void(0) : THROW(msg))

#define S1(x) #x
#define S2(x) S1(x)
#define LOCATION __FILE__ ":" S2(__LINE__)

#define ASSERT_EX(expr) ASSERT_EX_M(expr, "Assertion failure in " LOCATION ". Expression: " #expr)

#define DEBUG_ASSERT(expr) assert(expr)

#define CRITICAL_ASSERT_M(expr, msg) \
	if (!expr)                       \
	{                                \
		ERROR(msg);                  \
		exit(-1);                    \
	}

#define CRITICAL_ASSERT(expr) CRITICAL_ASSERT_M(expr, "Critical assertion failure in " LOCATION ". Expression: " #expr)
