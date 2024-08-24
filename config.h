#pragma once

#ifdef _MSC_VER
#pragma warning (push)
#pragma warning (disable : 4190) // Disable warning about incompatible C linkage
#define NOMINMAX
#endif

#ifdef __cplusplus
	#ifndef WAYLIB_DISABLE_CLASSES
	#define WAYLIB_ENABLE_CLASSES
	#endif

	#ifndef WAYLIB_DISABLE_DEFAULT_PARAMETERS
	#define WAYLIB_ENABLE_DEFAULT_PARAMETERS
	#endif

	#if !defined(WAYLIB_NAMESPACE_NAME) && !defined(WAYLIB_NO_NAMESPACE_NAME)
	#define WAYLIB_NAMESPACE_NAME wl
	#endif
#else
	#include <stdint.h> // NOTE: Not included on the C++ side since we get wrapped inside a namespace... the c++ side will include the headers itself!
	#include <stddef.h>
	#include <webgpu/webgpu.h>
#endif

#ifdef __EMSCRIPTEN__
	#include <emscripten.h>
#endif


// Macro which switches the type depending on if we are compiling in C or C++
#ifdef __cplusplus
	#define WAYLIB_C_OR_CPP_TYPE(ctype, cpptype) cpptype
#else
	#define WAYLIB_C_OR_CPP_TYPE(ctype, cpptype) ctype
#endif

#if defined(__cplusplus) && defined(__cpp_exceptions)
	#define WAYLIB_TRY try

	// Catches any exception and returns its argument as the function's default value if it catches anything
	#define WAYLIB_CATCH(ret) catch(const std::exception& e) { set_error_message_raw(e.what()); return ret; }
#else
	#define WAYLIB_TRY
	#define WAYLIB_CATCH(...)
#endif

// Macros used for nicer cross platform enums
#define WAYLIB_ENUM WAYLIB_C_OR_CPP_TYPE(enum, enum class)
#define C_PREPEND(pre, base) WAYLIB_C_OR_CPP_TYPE(pre##base, base)

// Macro used to convert one of the C handles to its C++ variant
#define WAYLIB_C_TO_CPP_CONVERSION(type, name) (*(type*)&(name))

// Define the type of indices... by default it is a u32 (2 byte integer)
#ifndef WAYLIB_INDEX_TYPE
	#define WAYLIB_INDEX_TYPE uint32_t
#endif
typedef WAYLIB_INDEX_TYPE index_t;

// Define the type of numbers... by default its a float
#ifndef WAYLIB_REAL_TYPE
	#define WAYLIB_REAL_TYPE float
#endif
typedef WAYLIB_REAL_TYPE real_t;