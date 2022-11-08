#pragma once
#include <cassert>
#include <cmath>

template<typename T>
struct vec2
{
	T x = 0, y = 0;

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

	T operator*(const vec2 &b) const
	{
		return x * b.x + y * b.y;
	}

	vec2 operator*(const T &v) const
	{
		return {x * v, y * v};
	}

	vec2 operator/(const T &v) const
	{
		return {x / v, y / v};
	}

	T length() const
	{
		return sqrt(x * x + y * y);
	}

	vec2 normalize() const
	{
		return *this / length();
	}

	template <typename A>
	static inline vec2<T> type_cast(const vec2<A> v)
	{
		return {static_cast<T>(v.x), static_cast<T>(v.y)};
	}
};

template <typename T>
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
inline float vec2<float>::length() const
{
	return sqrtf(x * x + y * y);
}

template <>
inline float vec3<float>::length() const
{
	return sqrtf(x * x + y * y + z * z);
}

template<typename T>
struct mat4
{
	float values[4][4] = {};

	mat4 operator*(const mat4 &b) const
	{
		mat4 res;
		for (int i = 0; i < 4; i++)
		{
			for (int j = 0; j < 4; j++)
			{
				res.values[j][i] = 0;
				for (int k = 0; k < 4; k++)
				{
					res.values[j][i] += values[k][i] * b.values[j][k];
				}
			}
		}
		return res;
	}
};

template<typename T>
inline T degrees_to_radians(const T &degrees)
{
	return degrees / 180 * M_PI;
}

inline mat4<float> perspective_projection_matrix(const float &fov, const float &near_plane, const float &far_plane, const float &aspect_ratio)
{
	const float tg = tanf(fov / 2);
	return {1.0f / (tg * aspect_ratio), 0, 0, 0,
			0, 1.0f / tg, 0, 0,
			0, 0, far_plane / (far_plane - near_plane), -near_plane * far_plane / (far_plane - near_plane),
			0, 0, 1, 0};
}

inline mat4<float> look_to_matrix(const vec3<float> &pos, const vec3<float> &dir, const vec3<float> &up)
{
	vec3<float> Right = up.cross(dir).normalize();
	vec3<float> Dir = dir.normalize();
	vec3<float> Up = Dir.cross(Right);

	return {Right.x, Right.y, Right.z, -pos.x * Right.x - pos.y * Right.y - pos.z * Right.z,
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

inline mat4<float> z_rotation_matrix(float angle)
{
	float cosb = cosf(angle);
	float sinb = sinf(angle);
	return {cosb, -sinb, 0, 0, sinb, cosb, 0, 0, 0, 0, 1.0f, 0, 0, 0, 0, 1.0f};
}

inline mat4<float> y_rotation_matrix(float angle)
{
	float cosb = cosf(angle);
	float sinb = sinf(angle);
	return {cosb, 0, -sinb, 0, 0, 1.0f, 0, 0, sinb, 0, cosb, 0, 0, 0, 0, 1.0f};
}

inline mat4<float> x_rotation_matrix(float angle)
{
	float cosb = cosf(angle);
	float sinb = sinf(angle);
	return {1.0f, 0, 0, 0, 0, cosb, sinb, 0, 0, -sinb, cosb, 0, 0, 0, 0, 1.0f};
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
