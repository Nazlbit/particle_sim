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

	vec2 operator-() const
	{
		return {-x, -y};
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

struct rect
{
    vec2 bottom_left;
    vec2 top_right;

    bool is_inside_ordered(const vec2 &p) const
    {
		assert(bottom_left.x < top_right.x);
		assert(bottom_left.y < top_right.y);

		return bottom_left.x <= p.x && bottom_left.y <= p.y &&
			   p.x < top_right.x && p.y < top_right.y;
	}

	bool is_inside_unordered(const vec2 &p) const
	{
		assert(bottom_left.x < top_right.x);
		assert(bottom_left.y < top_right.y);

		return bottom_left.x <= p.x && bottom_left.y <= p.y &&
			   p.x <= top_right.x && p.y <= top_right.y;
	}

	vec2 center() const
	{
		return (bottom_left + top_right) * 0.5;
	}

	vec2 size() const
	{
		return top_right - bottom_left;
	}
};

double random_double(double from, double to);
