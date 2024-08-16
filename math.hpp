#pragma once

#include <cstdint>
#include <cstddef>
#include <type_traits>

#include "wgsl_types.hpp"

#ifdef WAYLIB_NAMESPACE_NAME
namespace WAYLIB_NAMESPACE_NAME {
#endif

#include "math.h"

struct radian {
	float value;
	constexpr static float DEG2RAD = 3.141592653589793238463/180.0;

	radian(const float radian) : value(radian) {}
	radian(const radian& other) : radian(other.value) {}
	radian() : radian(0) {}
	radian(const degree d); 

	radian& operator=(const radian& other) = default;

	inline operator float() const { return value; }

	inline radian operator+(const radian other) const { return value + other.value; }
	inline radian operator-(const radian other) const { return value - other.value; }
	inline radian operator*(const radian other) const { return value * other.value; }
	inline radian operator/(const radian other) const { return value / other.value; }

	inline radian& operator+=(const radian other) { *this = *this + other; return *this; }
	inline radian& operator-=(const radian other) { *this = *this - other; return *this; }
	inline radian& operator*=(const radian other) { *this = *this * other; return *this; }
	inline radian& operator/=(const radian other) { *this = *this / other; return *this; }

	float degree_value() const;
};

struct degree {
	float value;
	constexpr static float RAD2DEG = 180.0/3.141592653589793238463;

	degree(float degree) : value(degree) {}
	degree(const degree& other) : degree(other.value) {}
	degree() : degree(0) {}
	degree(const radian r) : degree(float(r) * RAD2DEG) {}

	degree& operator=(const degree& other) = default;

	inline operator float() const { return value; }

	inline degree operator+(const degree other) const { return value + other.value; }
	inline degree operator-(const degree other) const { return value - other.value; }
	inline degree operator*(const degree other) const { return value * other.value; }
	inline degree operator/(const degree other) const { return value / other.value; }

	inline degree& operator+=(const degree other) { *this = *this + other; return *this; }
	inline degree& operator-=(const degree other) { *this = *this - other; return *this; }
	inline degree& operator*=(const degree other) { *this = *this * other; return *this; }
	inline degree& operator/=(const degree other) { *this = *this / other; return *this; }

	inline float radian_value() const { return radian(*this); }
};

inline radian::radian(const degree d) : radian(float(d) * DEG2RAD) {}
inline float radian::degree_value() const { return degree(*this); }

inline degree rad_to_deg(radian r) { return r; }
inline radian deg_to_rad(degree d) { return d; }



float sin(radian);
float cos(radian);
float tan(radian);
radian asin(float);
radian acos(float);
radian atan(float);
radian atan2(float y, float x);
radian atan_vec(vec2f);
float sinh(radian);
float cosh(radian);
float tanh(radian);
radian asinh(float);
radian acosh(float);
radian atanh(float);

#ifdef WAYLIB_NAMESPACE_NAME
}
#endif