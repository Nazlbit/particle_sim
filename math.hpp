#pragma once
#include <cassert>

struct vec2
{
    double x = 0, y = 0;

	vec2 operator+(const vec2 &b) const
	{
		return {x + b.x, y + b.y};
	}

	vec2 operator-(const vec2 &b) const
	{
		return {x - b.x, y - b.y};
	}

	double operator*(vec2 b) const
	{
		return x * b.x + y * b.y;
	}

	vec2 operator*(double v) const
	{
		return {x * v, y * v};
	}

	vec2 operator/(double v) const
	{
		return {x / v, y / v};
	}
};

struct vec2_f
{
	float x = 0, y = 0;
	vec2_f() = default;
	vec2_f(const vec2 &v) : x(v.x), y(v.y){}

	vec2_f &operator=(const vec2 &v)
	{
		x = v.x;
		y = v.y;
		return *this;
	}
};

struct rect
{
    vec2 pos;
    vec2 half_size;

    bool is_inside_ordered(const vec2 &p) const
    {
		assert(half_size.x > 0);
		assert(half_size.y > 0);

		const vec2 r = p - pos;
		return r.x <= half_size.x && -half_size.x < r.x &&
			   r.y <= half_size.y && -half_size.y < r.y;
	}

	bool is_inside_unordered(const vec2 &p) const
	{
		assert(half_size.x > 0);
		assert(half_size.y > 0);

		const vec2 r = p - pos;
		return r.x <= half_size.x && -half_size.x <= r.x &&
			   r.y <= half_size.y && -half_size.y <= r.y;
	}
};

double random_double(double from, double to);
