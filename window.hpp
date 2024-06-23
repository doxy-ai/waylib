#pragma once
#include <cstdint>
#include <cstddef>
#include <string>

#define GLFW_INCLUDE_NONE
#include <GLFW/glfw3.h>

#include <webgpu/webgpu.hpp>

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

wgpu::Surface window_get_surface(
	window* window,
	wgpu::Device webgpu
);

#ifdef WAYLIB_NAMESPACE_NAME
}
#endif