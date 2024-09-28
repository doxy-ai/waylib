#include "window.hpp"
#include "waylib/core/config.h"

WAYLIB_BEGIN_NAMESPACE

bool glfw_initialized = false;
void maybe_initialize_glfw() {
	if(glfw_initialized) return;

	static auto glfw = glfw::init();
}


WAYLIB_PREFIXED(window)* WAYLIB_PREFIXED(create_window) (WAYLIB_PREFIXED(vec2u) size, const char* title /*= "Waylib"*/, WAYLIB_PREFIXED(window_config) config /*= {}*/) WAYLIB_NOEXCEPT {
	return new window(window::create(size, title, config));
}

WAYLIB_END_NAMESPACE