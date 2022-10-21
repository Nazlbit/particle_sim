#pragma once
#include <cstdio>
#include <cassert>
#include "exception.hpp"

#define THROW(msg) (throw app::exception(msg))

#define ASSERT_EX_M(expr, msg) (expr ? void(0) : THROW(msg))

#define S1(x) #x
#define S2(x) S1(x)
#define LOCATION __FILE__ ":" S2(__LINE__)

#define ASSERT_EX(expr) (ASSERT_EX_M(expr, "Assertion failure in " LOCATION ". " #expr))

#define DEBUG_ASSERT(expr) (assert(expr))

#define ERROR(msg, ...) (fprintf(stderr, "ERROR: " msg "\n", ##__VA_ARGS__))

#define WARNING(msg, ...) (fprintf(stdout, "WARNING: " msg "\n", ##__VA_ARGS__))

#define INFO(msg, ...) (fprintf(stdout, "INFO: " msg "\n", ##__VA_ARGS__))
