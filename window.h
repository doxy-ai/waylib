#ifndef WAYLIB_WINDOW_IS_AVAILABLE
#define WAYLIB_WINDOW_IS_AVAILABLE

#include "waylib.h"

#ifdef __cplusplus
extern "C" {
#else
#include "GLFW/glfw3.h"
#endif

#ifdef __cplusplus
using window = GLFWwindow;
using monitor = GLFWmonitor;
#else
struct GLFWwindow;
typedef GLFWwindow window;
struct GLFWmonitor;
typedef GLFWmonitor monitor;
#endif

typedef struct fullscreen_window_initialization_configuration {
	; monitor* monitor
#ifdef WAYLIB_ENABLE_DEFAULT_PARAMETERS
		= nullptr
#endif
	; bool auto_minimize_when_focus_lost
#ifdef WAYLIB_ENABLE_DEFAULT_PARAMETERS
		= false
#endif
	; bool forcibly_center_cursor
#ifdef WAYLIB_ENABLE_DEFAULT_PARAMETERS
		= true
#endif
	;
} fullscreen_window_initialization_configuration;

typedef struct window_initialization_configuration {
	; bool user_resizable
#ifdef WAYLIB_ENABLE_DEFAULT_PARAMETERS
		= true
#endif
	; bool initially_visible
#ifdef WAYLIB_ENABLE_DEFAULT_PARAMETERS
		= true
#endif
	; bool initially_focused
#ifdef WAYLIB_ENABLE_DEFAULT_PARAMETERS
		= true
#endif
	; bool initially_maximized
#ifdef WAYLIB_ENABLE_DEFAULT_PARAMETERS
		= false
#endif
	; bool decorated
#ifdef WAYLIB_ENABLE_DEFAULT_PARAMETERS
		= true
#endif
	; bool always_on_top
#ifdef WAYLIB_ENABLE_DEFAULT_PARAMETERS
		= false
#endif
	; bool transparent_framebuffer
#ifdef WAYLIB_ENABLE_DEFAULT_PARAMETERS
		= false
#endif
	; bool focus_on_show
#ifdef WAYLIB_ENABLE_DEFAULT_PARAMETERS
		= false
#endif
	; fullscreen_window_initialization_configuration fullscreen;
} window_initialization_configuration;



monitor* get_primary_monitor();

void close_all_windows();

bool poll_all_window_events();

window_initialization_configuration default_window_initialization_configuration();

window* create_window(
	size_t width,
	size_t height,
	const char* title
#ifdef WAYLIB_ENABLE_DEFAULT_PARAMETERS
		= "waylib"
#endif
	, window_initialization_configuration config
#ifdef WAYLIB_ENABLE_DEFAULT_PARAMETERS
		= default_window_initialization_configuration()
#endif
);

void release_window(
	window* window
);

bool window_should_close(
	window* window,
	bool should_poll_events
#ifdef WAYLIB_ENABLE_DEFAULT_PARAMETERS
		= true
#endif
);

vec2i window_get_dimensions(window* window);

WAYLIB_C_OR_CPP_TYPE(WGPUSurface, wgpu::Surface) window_get_surface(window* window, WGPUInstance instance);

wgpu_state window_get_wgpu_state(window* window, WGPUDevice device);

bool window_configure_surface(
	window* window,
	wgpu_state state,
	surface_configuration config
#ifdef WAYLIB_ENABLE_DEFAULT_PARAMETERS
		= {}
#endif
);

void window_automatically_reconfigure_surface_on_resize(
	window* window,
	wgpu_state state,
	surface_configuration config
#ifdef WAYLIB_ENABLE_DEFAULT_PARAMETERS
		= {}
#endif
);

wgpu_state create_default_device_from_window(
	window* window, bool prefer_low_power
#ifdef WAYLIB_ENABLE_DEFAULT_PARAMETERS
		= false
#endif
);

#ifndef WAYLIB_NO_CAMERAS
void window_begin_camera_mode3D(
	wgpu_frame_state* frame,
	window* window,
	camera3D* camera
);

void window_begin_camera_mode2D(
	wgpu_frame_state* frame,
	window* window,
	camera2D* camera
);

void window_begin_camera_mode_identity(
	wgpu_frame_state* frame,
	window* window
);
#endif // WAYLIB_NO_CAMERAS

#ifdef __cplusplus
} // End extern "C"
#endif

#endif // WAYLIB_WINDOW_IS_AVAILABLE