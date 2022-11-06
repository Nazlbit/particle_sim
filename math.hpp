#pragma once
#include <cassert>

struct vec3
{
    double x = 0, y = 0, z = 0;

	vec3 operator+(const vec3 &b) const
	{
		return {x + b.x, y + b.y, z + b.z};
	}

	vec3 operator-(const vec3 &b) const
	{
		return {x - b.x, y - b.y, z - b.z};
	}

	vec3 operator-() const
	{
		return {-x, -y, -z};
	}

	double operator*(const vec3 &b) const
	{
		return x * b.x + y * b.y + z * b.z;
	}

	vec3 operator*(const double &v) const
	{
		return {x * v, y * v, z * v};
	}

	vec3 operator/(const double &v) const
	{
		return {x / v, y / v, z / v};
	}
};

struct vec3_f
{
	float x = 0, y = 0, z = 0;
	vec3_f() = default;
	vec3_f(const vec3 &v) : x(v.x), y(v.y), z(v.z){}

	vec3_f &operator=(const vec3 &v)
	{
		x = v.x;
		y = v.y;
		z = v.z;
		return *this;
	}
};

struct rect
{
	vec3 left_bottom_near;
	vec3 right_top_far;

	bool is_inside_ordered(const vec3 &p) const
	{
		assert(left_bottom_near.x < right_top_far.x);
		assert(left_bottom_near.y < right_top_far.y);
		assert(left_bottom_near.z < right_top_far.z);

		return left_bottom_near.x < p.x && left_bottom_near.y < p.y && left_bottom_near.z < p.z &&
			   p.x <= right_top_far.x && p.y <= right_top_far.y && p.z <= right_top_far.z;
	}

	bool is_inside_unordered(const vec3 &p) const
	{
		assert(left_bottom_near.x < right_top_far.x);
		assert(left_bottom_near.y < right_top_far.y);
		assert(left_bottom_near.z < right_top_far.z);

		return left_bottom_near.x <= p.x && left_bottom_near.y <= p.y && left_bottom_near.z <= p.z &&
			   p.x <= right_top_far.x && p.y <= right_top_far.y && p.z <= right_top_far.z;
	}

	vec3 center() const
	{
		return (left_bottom_near + right_top_far) * 0.5;
	}

	vec3 size() const
	{
		return right_top_far - left_bottom_near;
	}
};

double random_double(double from, double to);

struct dimensions
{
	int width = 0, height = 0;
};
