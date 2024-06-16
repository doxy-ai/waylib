#pragma once
#include <cstdint>
#include <cstddef>
// #include <memory>
#include <string>

#define GLFW_INCLUDE_NONE
#include <GLFW/glfw3.h>

#ifdef WAYLIB_NAMESPACE_NAME
namespace WAYLIB_NAMESPACE_NAME {
#endif

#include "window.h"

// extern bool glfwInitialized;

window* create_window(
	size_t width, 
	size_t height, 
	std::string_view title,
	window_initialization_configuration config
#ifdef WAYLIB_ENABLE_DEFAULT_PARAMETERS
		= default_window_initialization_configuration()
#endif
);

#ifdef WAYLIB_NAMESPACE_NAME
}
#endif