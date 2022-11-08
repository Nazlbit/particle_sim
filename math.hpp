#pragma once
#include <cassert>
#include <cmath>

template<typename T>
struct vec3
{
    T x = 0, y = 0, z = 0;

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

	T operator*(const vec3 &b) const
	{
		return x * b.x + y * b.y + z * b.z;
	}

	vec3 operator*(const T &v) const
	{
		return {x * v, y * v, z * v};
	}

	vec3 operator/(const T &v) const
	{
		return {x / v, y / v, z / v};
	}

	vec3 cross(const vec3 &v) const
	{
		return {y * v.z - z * v.y, z * v.x - x * v.z, x * v.y - y * v.x};
	}

	T length() const
	{
		return sqrt(x * x + y * y + z * z);
	}

	vec3 normalize() const
	{
		return *this / length();
	}

	template<typename A>
	static inline vec3<T> type_cast(const vec3<A> v)
	{
		return {static_cast<T>(v.x), static_cast<T>(v.y), static_cast<T>(v.z)};
	}
};

template<>
inline float vec3<float>::length() const
{
	return sqrtf(x * x + y * y + z * z);
}

template<typename T>
struct mat4
{
	float values[4][4] = {};
};

template<typename T>
inline T degrees_to_radians(const T &degrees)
{
	return degrees / 180 * M_PI;
}

inline mat4<float> perspective_projection_matrix(const float &fov, const float &near_plane, const float &far_plane, const float &aspect_ratio)
{
	const float tg = tanf(fov / 2);
	return mat4<float>{1.0f / (tg * aspect_ratio), 0, 0, 0,
				   0, 1.0f / tg, 0, 0,
				   0, 0, far_plane / (far_plane - near_plane), -near_plane * far_plane / (far_plane - near_plane),
				   0, 0, 1, 0};
}

inline mat4<float> look_to_matrix(const vec3<float> &pos, const vec3<float> &dir, const vec3<float> &up)
{
	vec3<float> Right = up.cross(dir).normalize();
	vec3<float> Dir = dir.normalize();
	vec3<float> Up = Dir.cross(Right);

	return mat4<float>{Right.x, Right.y, Right.z, -pos.x * Right.x - pos.y * Right.y - pos.z * Right.z,
				   Up.x, Up.y, Up.z, -pos.x * Up.x - pos.y * Up.y - pos.z * Up.z,
				   Dir.x, Dir.y, Dir.z, -pos.x * Dir.x - pos.y * Dir.y - pos.z * Dir.z,
				   0, 0, 0, 1};
}

inline mat4<float> identity_matrix()
{
	return {1, 0, 0, 0,
			0, 1, 0, 0,
			0, 0, 1, 0,
			0, 0, 0, 1};
}

double uniform_random_double(double from, double to);

double normal_random_double(double mean, double stddev);

struct dimensions
{
	int width = 0, height = 0;
};

template<typename T>
struct cube
{
	vec3<T> pos;
	T half_size = 0;

	bool is_inside_ordered(const vec3<T> &p) const
	{
		const vec3<T> r = p - pos;
		return -half_size < r.x && r.x <= half_size &&
		       -half_size < r.y && r.y <= half_size &&
		       -half_size < r.z && r.z <= half_size;
	}
};
