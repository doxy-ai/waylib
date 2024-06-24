#include "window.hpp"
#include "waylib.hpp"

#include <glfw3webgpu.h>

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

void release_window(window* window) {
	glfwDestroyWindow(window);
}

bool window_should_close(window* window, bool should_poll_events /*= true*/) {
	if(should_poll_events) poll_all_window_events();
	return glfwWindowShouldClose(window);
}

wgpu::Surface window_get_surface(window* window, WGPUInstance instance) {
	return glfwGetWGPUSurface(instance, window);
}
wgpu::Surface window_get_surface(window* window, wgpu::Device device) {
	return glfwGetWGPUSurface(device.getAdapter().getInstance(), window);
}

webgpu_state window_get_webgpu_state(window* window, WGPUDevice _device) {
	wgpu::Device& device = WAYLIB_C_TO_CPP_CONVERSION(wgpu::Device, _device);
	return {device, window_get_surface(window, device)};
}

bool window_configure_surface(
	window* window, webgpu_state state, 
	WGPUPresentMode present_mode /*= wgpu::PresentMode::Mailbox*/, 
	WGPUCompositeAlphaMode alpha_mode /*= wgpu::CompositeAlphaMode::Auto*/
) {
	int width, height;
	glfwGetWindowSize(window, &width, &height);
	return configure_surface(state, vec2i{width, height}, present_mode, alpha_mode);
}

void window_automatically_reconfigure_surface_on_resize(
	window* window, webgpu_state state, 
	WGPUPresentMode present_mode /*= wgpu::PresentMode::Mailbox*/, 
	WGPUCompositeAlphaMode alpha_mode /*= wgpu::CompositeAlphaMode::Auto*/, 
	bool configure_now /*= true*/
) {
	struct ResizeData {
		char magic[5] = "ReDa";
		webgpu_state state;
		WGPUPresentMode present_mode;
		WGPUCompositeAlphaMode alpha_mode;
	};

	if(configure_now) window_configure_surface(window, state, present_mode, alpha_mode);
	
	if(auto data = (ResizeData*)glfwGetWindowUserPointer(window); data && std::string_view(data->magic) == "ReDa")
		delete data;

	glfwSetWindowUserPointer(window, new ResizeData{"ReDa", state, present_mode, alpha_mode});
	glfwSetWindowSizeCallback(window, [](GLFWwindow* window, int width, int height) {
		auto data = (ResizeData*)glfwGetWindowUserPointer(window);
		if(data && std::string_view(data->magic) == "ReDa")
			configure_surface(data->state, vec2i{width, height}, data->present_mode, data->alpha_mode);
	});
}

webgpu_state create_default_device_from_window(window* window, bool prefer_low_power /*= false*/) {
	WGPUInstance instance = wgpuCreateInstance({});
	wgpu::Surface surface = window_get_surface(window, instance);
	return {
		.device = create_default_device_from_instance(instance, surface, prefer_low_power),
		.surface = surface
	};
}

#ifdef WAYLIB_NAMESPACE_NAME
}
#endif