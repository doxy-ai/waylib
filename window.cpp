#include "window.hpp"
#include "waylib.hpp"

#include <glfw3webgpu.h>

#ifdef WAYLIB_NAMESPACE_NAME
namespace WAYLIB_NAMESPACE_NAME {
#endif

namespace detail {
	bool glfwInitialized = false;

	bool maybe_initialize_glfw() {
		if(!glfwInitialized) {
			if(!glfwInit())
				return false;
			glfwInitialized = true;
		}
		return true;
	}
}

//////////////////////////////////////////////////////////////////////
// #Monitor
//////////////////////////////////////////////////////////////////////

monitor* get_primary_monitor() {
	if(!detail::maybe_initialize_glfw()) return nullptr;
	return glfwGetPrimaryMonitor();
}

//////////////////////////////////////////////////////////////////////
// #Window
//////////////////////////////////////////////////////////////////////

void close_all_windows() {
	if(detail::glfwInitialized) {
		glfwTerminate();
		detail::glfwInitialized = false;
	}
}

bool poll_all_window_events() {
	if(!detail::glfwInitialized) return false;
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
	if(!detail::maybe_initialize_glfw()) return nullptr;

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
	return create_window(width, height, cstring_from_view(title.data()), config);
}

void release_window(window* window) {
	glfwDestroyWindow(window);
}

bool window_should_close(window* window, bool should_poll_events /*= true*/) {
	if(should_poll_events) poll_all_window_events();
	return glfwWindowShouldClose(window);
}

vec2i window_get_dimensions(window* window) {
	vec2i out;
	glfwGetWindowSize(window, &out.x, &out.y);
	return out;
}

wgpu::Surface window_get_surface(window* window, WGPUInstance instance) {
	return glfwGetWGPUSurface(instance, window);
}
wgpu::Surface window_get_surface(window* window, wgpu::Device device) {
	return glfwGetWGPUSurface(device.getAdapter().getInstance(), window);
}

wgpu_state window_get_wgpu_state(window* window, WGPUDevice _device) {
	wgpu::Device& device = WAYLIB_C_TO_CPP_CONVERSION(wgpu::Device, _device);
	return {device, window_get_surface(window, device)};
}

bool window_configure_surface(
	window* window, wgpu_state state,
	surface_configuration config /*= {}*/
) {
	return configure_surface(state, window_get_dimensions(window), config);
}

void window_automatically_reconfigure_surface_on_resize(
	window* window, wgpu_state state,
	surface_configuration config /*= {}*/
) {
	struct ResizeData {
		char magic[5] = "ReDa";
		wgpu_state state;
		surface_configuration config;
	};

	if(config.automatic_should_configure_now) window_configure_surface(window, state, config);

	if(auto data = (ResizeData*)glfwGetWindowUserPointer(window); data && std::string_view(data->magic) == "ReDa")
		delete data;

	glfwSetWindowUserPointer(window, new ResizeData{"ReDa", state, config});
	glfwSetWindowSizeCallback(window, [](GLFWwindow* window, int width, int height) {
		auto data = (ResizeData*)glfwGetWindowUserPointer(window);
		if(data && std::string_view(data->magic) == "ReDa")
			configure_surface(data->state, vec2i{width, height}, data->config);
	});
}

wgpu_state create_default_device_from_window(window* window, bool prefer_low_power /*= false*/) {
	WGPUInstance instance = wgpuCreateInstance({});
	wgpu::Surface surface = window_get_surface(window, instance);
	return {
		.device = create_default_device_from_instance(instance, surface, prefer_low_power),
		.surface = surface
	};
}

#ifndef WAYLIB_NO_CAMERAS
void window_begin_camera_mode3D(wgpu_frame_state* frame, window* window, camera3D* camera, light* lights /*= nullptr*/, size_t light_count /*=0*/, WAYLIB_OPTIONAL(frame_time) frame_time /*={}*/) {
	begin_camera_mode3D(*frame, *camera, window_get_dimensions(window), {lights, light_count}, frame_time);
}
void window_begin_camera_mode3D(wgpu_frame_state& frame, window* window, camera3D& camera, std::span<light> lights /*={}*/, WAYLIB_OPTIONAL(frame_time) frame_time /*={}*/) {
	begin_camera_mode3D(frame, camera, window_get_dimensions(window), lights, frame_time);
}

void window_begin_camera_mode2D(wgpu_frame_state* frame, window* window, camera2D* camera, light* lights /*= nullptr*/, size_t light_count /*=0*/, WAYLIB_OPTIONAL(frame_time) frame_time /*={}*/) {
	begin_camera_mode2D(*frame, *camera, window_get_dimensions(window), {lights, light_count}, frame_time);
}
void window_begin_camera_mode2D(wgpu_frame_state& frame, window* window, camera2D& camera, std::span<light> lights /*={}*/, WAYLIB_OPTIONAL(frame_time) frame_time /*={}*/) {
	begin_camera_mode2D(frame, camera, window_get_dimensions(window), lights, frame_time);
}

void window_begin_camera_mode_identity(wgpu_frame_state* frame, window* window, light* lights /*= nullptr*/, size_t light_count /*=0*/, WAYLIB_OPTIONAL(frame_time) frame_time /*={}*/) {
	begin_camera_mode_identity(*frame, window_get_dimensions(window), {lights, light_count}, frame_time);
}
void window_begin_camera_mode_identity(wgpu_frame_state& frame, window* window, std::span<light> lights /*={}*/, WAYLIB_OPTIONAL(frame_time) frame_time /*={}*/) {
	begin_camera_mode_identity(frame, window_get_dimensions(window), lights, frame_time);
}
#endif // WAYLIB_NO_CAMERAS

#ifdef WAYLIB_NAMESPACE_NAME
}
#endif