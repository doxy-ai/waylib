#pragma once
#ifdef __cplusplus
	#ifndef WAYLIB_DISABLE_CLASSES
	#define WAYLIB_ENABLE_CLASSES
	#endif

	#ifndef WAYLIB_DISABLE_DEFAULT_PARAMETERS
	#define WAYLIB_ENABLE_DEFAULT_PARAMETERS
	#endif
#else
	#include <stdint.h> // NOTE: Not included on the C++ side since we get wrapped inside a namespace... the c++ side will include the headers itself!
	#include <stddef.h>
	#include <webgpu/webgpu.h>
#endif


// Macro which switches the type depending on if we are compiling in C or C++
#ifdef __cplusplus
	#define WAYLIB_C_OR_CPP_TYPE(ctype, cpptype) cpptype
#else
	#define WAYLIB_C_OR_CPP_TYPE(ctype, cpptype) ctype
#endif

// Macros used for nicer cross platform enums
#define WAYLIB_ENUM WAYLIB_C_OR_CPP_TYPE(enum, enum class)
#define C_PREPEND(pre, base) WAYLIB_C_OR_CPP_TYPE(pre##base, base)

// Macro used to convert one of the C handles to its C++ variant
#define WAYLIB_C_TO_CPP_CONVERSION(type, name) (*(type*)&(name))

// Macro which defines an optional struct
#ifdef __cplusplus
	template<typename T>
	struct optional {
		bool has_value = false;
		T value;

		optional() : has_value(false) {}
		optional(const T& value) : has_value(true), value(value) {}
		optional(bool has_value, const T& value) : has_value(has_value), value(value) {}
		optional(const optional&) = default;
		optional(optional&&) = default;
		optional& operator=(const optional&) = default;
		optional& operator=(optional&&) = default;
	};
	#ifdef WAYLIB_NAMESPACE_NAME
		#define WAYLIB_OPTIONAL(type) WAYLIB_NAMESPACE_NAME::optional<type>
	#else
		#define WAYLIB_OPTIONAL(type) optional<type>
	#endif
#else
	#define WAYLIB_OPTIONAL(type) struct {\
		bool has_value;\
		type value;\
	}
#endif

// Define the type of indices... by default it is a u32 (2 byte integer)
#ifndef WAYLIB_INDEX_TYPE
	#define WAYLIB_INDEX_TYPE uint32_t
#endif
typedef WAYLIB_INDEX_TYPE index_t;