#include "window.hpp"
#include "GLFW/glfw3.h"

#ifdef WAYLIB_NAMESPACE_NAME
namespace WAYLIB_NAMESPACE_NAME {
#endif

bool glfwInitialized = false;

bool maybe_initialize_glfw() {
	if(!glfwInitialized) {
		if(!glfwInit())
			return false;
		glfwInitialized = true;
	}
	return true;
}

monitor* get_primary_monitor() {
	if(!maybe_initialize_glfw()) return nullptr;
	return glfwGetPrimaryMonitor();
}

void close_all_windows() {
	if(glfwInitialized) {
		glfwTerminate();
		glfwInitialized = false;
	}
}

bool poll_all_window_events() {
	if(!glfwInitialized) return false;
	glfwPollEvents();
	return true;
}

window_initialization_configuration default_window_initialization_configuration() {
	return window_initialization_configuration {
		.user_resizable = true,
		.initially_visible = true,
		.initially_focused = true,
		.initially_maximized = false,
		.decorated = true,
		.always_on_top = false,
		.transparent_framebuffer = false,
		.focus_on_show = false,
		.fullscreen = { 
			.monitor = nullptr,
			.auto_minimize_when_focus_lost = false,
			.forcibly_center_cursor = true
		}
	};
}

window* create_window(size_t width, size_t height, const char* title /*= "waylib"*/, window_initialization_configuration config /*= default*/) {
	if(!maybe_initialize_glfw()) return nullptr;

	glfwWindowHint(GLFW_RESIZABLE, config.user_resizable);
	glfwWindowHint(GLFW_VISIBLE, config.initially_visible);
	glfwWindowHint(GLFW_DECORATED, config.decorated);
	glfwWindowHint(GLFW_FOCUSED, config.initially_focused);
	glfwWindowHint(GLFW_AUTO_ICONIFY, config.fullscreen.auto_minimize_when_focus_lost);
	glfwWindowHint(GLFW_FLOATING, config.always_on_top);
	glfwWindowHint(GLFW_MAXIMIZED, config.initially_maximized);
	glfwWindowHint(GLFW_CENTER_CURSOR, config.fullscreen.forcibly_center_cursor);
	glfwWindowHint(GLFW_TRANSPARENT_FRAMEBUFFER, config.transparent_framebuffer);
	glfwWindowHint(GLFW_FOCUS_ON_SHOW, config.focus_on_show);
	glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API); // No OpenGL Surface!
	return glfwCreateWindow(width, height, title, config.fullscreen.monitor, nullptr);
}
window* create_window(size_t width, size_t height, std::string_view title, window_initialization_configuration config /*= default*/) {
	return create_window(width, height, title.data(), config); 
}

void window_free(window* window) {
	glfwDestroyWindow(window);
}

bool window_should_close(window* window, bool should_poll_events /*= true*/) {
	if(should_poll_events) poll_all_window_events();
	return glfwWindowShouldClose(window);
}

#ifdef WAYLIB_NAMESPACE_NAME
}
#endif