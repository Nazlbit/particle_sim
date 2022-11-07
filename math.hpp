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

double uniform_random_double(double from, double to);

double normal_random_double(double mean, double stddev);

struct dimensions
{
	int width = 0, height = 0;
};

struct cube
{
	vec3 pos;
	double half_size = 0;

	bool is_inside_ordered(const vec3 &p) const
	{
		const vec3 r = p - pos;
		return -half_size < r.x && r.x <= half_size &&
		       -half_size < r.y && r.y <= half_size &&
		       -half_size < r.z && r.z <= half_size;
	}
};
