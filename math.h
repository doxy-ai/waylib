#ifndef WAYLIB_MATH_IS_AVAILABLE
#define WAYLIB_MATH_IS_AVAILABLE
#include "config.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct mat4x4f_ {
	float a0, a1, a2, a3;
	float b0, b1, b2, b3;
	float c0, c1, c2, c3;
	float d0, d1, d2, d3;
} mat4x4f_;

// Define mathematical types used... in C++ they are provided by glm so we just prototype them here!
#ifndef __cplusplus
typedef struct vec2i {
	int32_t x, y;
} vec2i;
typedef struct vec2f {
	float x, y;
} vec2f;
typedef struct vec3f {
	float x, y, z;
} vec3f;
typedef struct vec4f {
	float x, y, z, w;
} vec4f;
typedef struct mat4x4f_ mat4x4f; // The version we use in the c++ side is quite a bit more advanced
typedef float degree;
typedef float radian;
#else
struct vec2i;
struct vec2f;
struct vec3f;
struct vec4f;
struct mat4x4f;
struct degree;
struct radian;
#endif

radian deg_to_rad(degree);
degree rad_to_deg(radian);

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



#ifdef __cplusplus
} // End extern "C"
#endif

#endif // WAYLIB_MATH_IS_AVAILABLE