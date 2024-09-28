#ifndef WAYLIB_INTERFACES_f9641d712fbea9630f1ce0a0613269d7
#define WAYLIB_INTERFACES_f9641d712fbea9630f1ce0a0613269d7

#include "config.h"
#include <stdint.h>

//////////////////////////////////////////////////////////////////////
// # Math
//////////////////////////////////////////////////////////////////////

#ifndef WAYLIB_NO_SCALAR_ALIASES
	typedef uint8_t WAYLIB_PREFIXED(u8);
	typedef uint16_t WAYLIB_PREFIXED(u16);
	typedef uint32_t WAYLIB_PREFIXED(u32);
	typedef uint64_t WAYLIB_PREFIXED(u64);
	typedef int8_t WAYLIB_PREFIXED(i8);
	typedef int16_t WAYLIB_PREFIXED(i16);
	typedef int32_t WAYLIB_PREFIXED(i32);
	typedef int64_t WAYLIB_PREFIXED(i64);
	typedef float WAYLIB_PREFIXED(f32);
	typedef double WAYLIB_PREFIXED(f64);
	typedef uint32_t WAYLIB_PREFIXED(bool32);
#endif

typedef struct {
	uint32_t x, y;
} WAYLIB_PREFIXED_C_CPP_TYPE(vec2u, vec2uC);

typedef struct {
	int32_t x, y;
} WAYLIB_PREFIXED_C_CPP_TYPE(vec2i, vec2iC);

typedef struct {
	float x, y;
} WAYLIB_PREFIXED_C_CPP_TYPE(vec2f, vec2fC);

typedef struct {
	uint32_t x, y, z;
} WAYLIB_PREFIXED_C_CPP_TYPE(vec3u, vec3uC);

typedef struct {
	int32_t x, y, z;
} WAYLIB_PREFIXED_C_CPP_TYPE(vec3i, vec3iC);

typedef struct {
	float x, y, z;
} WAYLIB_PREFIXED_C_CPP_TYPE(vec3f, vec3fC);

typedef struct {
	uint32_t x, y, z, w;
} WAYLIB_PREFIXED_C_CPP_TYPE(vec4u, vec4uC);

typedef struct {
	int32_t x, y, z, w;
} WAYLIB_PREFIXED_C_CPP_TYPE(vec4i, vec4iC);

typedef struct {
	float x, y, z, w;
} WAYLIB_PREFIXED_C_CPP_TYPE(vec4f, vec4fC);

typedef struct {
	float m0, m4, m8, m12;      // Matrix first row (4 components)
    float m1, m5, m9, m13;      // Matrix second row (4 components)
    float m2, m6, m10, m14;     // Matrix third row (4 components)
    float m3, m7, m11, m15;     // Matrix fourth row (4 components)
} WAYLIB_PREFIXED_C_CPP_TYPE(mat4x4f, mat4x4fC);


//////////////////////////////////////////////////////////////////////
// # Opaque Types
//////////////////////////////////////////////////////////////////////


struct WAYLIB_PREFIXED(window);

#endif // WAYLIB_INTERFACES_f9641d712fbea9630f1ce0a0613269d7