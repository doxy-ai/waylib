#ifndef WAYLIB_WINDOW_IS_AVAILABLE
#define WAYLIB_WINDOW_IS_AVAILABLE

#ifndef __cplusplus
	#include <GLFW/glfw3.h>
#endif

#include "waylib/core/core.h"

typedef struct WAYLIB_PREFIXED(window_config) { // TODO: Fill out optional parameters flag
	bool resizable
#ifdef WAYLIB_ENABLE_DEFAULT_PARAMETERS
		 = true
#endif
	; bool visible 
#ifdef WAYLIB_ENABLE_DEFAULT_PARAMETERS
		= true
#endif
	; bool decorated 
#ifdef WAYLIB_ENABLE_DEFAULT_PARAMETERS
		= true
#endif
	; bool focused 
#ifdef WAYLIB_ENABLE_DEFAULT_PARAMETERS
		= true
#endif
	; bool auto_iconify 
#ifdef WAYLIB_ENABLE_DEFAULT_PARAMETERS
		= true
#endif
	; bool floating 
#ifdef WAYLIB_ENABLE_DEFAULT_PARAMETERS
		= false
#endif
	; bool maximized 
#ifdef WAYLIB_ENABLE_DEFAULT_PARAMETERS
		= false
#endif
	; bool center_cursor 
#ifdef WAYLIB_ENABLE_DEFAULT_PARAMETERS
		= true
#endif
	; bool transparent_framebuffer 
#ifdef WAYLIB_ENABLE_DEFAULT_PARAMETERS
		= false
#endif
	; bool focus_on_show 
#ifdef WAYLIB_ENABLE_DEFAULT_PARAMETERS
		= true
#endif
	; bool scale_to_monitor 
#ifdef WAYLIB_ENABLE_DEFAULT_PARAMETERS
		= false
#endif

	; int red_bits 
#ifdef WAYLIB_ENABLE_DEFAULT_PARAMETERS
		= 8
#endif
	; int green_bits 
#ifdef WAYLIB_ENABLE_DEFAULT_PARAMETERS
		= 8
#endif
	; int blue_bits 
#ifdef WAYLIB_ENABLE_DEFAULT_PARAMETERS
		= 8
#endif
	; int alpha_bits 
#ifdef WAYLIB_ENABLE_DEFAULT_PARAMETERS
		= 8
#endif
	; int depth_bits 
#ifdef WAYLIB_ENABLE_DEFAULT_PARAMETERS
		= 24
#endif
	; int stencil_bits 
#ifdef WAYLIB_ENABLE_DEFAULT_PARAMETERS
		= 8
#endif
	; int accum_red_bits 
#ifdef WAYLIB_ENABLE_DEFAULT_PARAMETERS
		= 0
#endif
	; int accum_green_bits 
#ifdef WAYLIB_ENABLE_DEFAULT_PARAMETERS
		= 0
#endif
	; int accum_blue_bits 
#ifdef WAYLIB_ENABLE_DEFAULT_PARAMETERS
		= 0
#endif
	; int accum_alpha_bits 
#ifdef WAYLIB_ENABLE_DEFAULT_PARAMETERS
		= 0
#endif

	; int aux_buffers 
#ifdef WAYLIB_ENABLE_DEFAULT_PARAMETERS
		= 0
#endif
	; int samples 
#ifdef WAYLIB_ENABLE_DEFAULT_PARAMETERS
		= 0
#endif
	; int refresh_rate 
#ifdef WAYLIB_ENABLE_DEFAULT_PARAMETERS
		= GLFW_DONT_CARE
#endif
	; bool stereo 
#ifdef WAYLIB_ENABLE_DEFAULT_PARAMETERS
		= false
#endif
	; bool srgb_capable 
#ifdef WAYLIB_ENABLE_DEFAULT_PARAMETERS
		= true
#endif
	; bool double_buffer 
#ifdef WAYLIB_ENABLE_DEFAULT_PARAMETERS
		= true
#endif
	;
} WAYLIB_PREFIXED(window_config);

WAYLIB_PREFIXED(window)* WAYLIB_PREFIXED(create_window) (
	WAYLIB_PREFIXED(vec2u) size, 
	const char* title
#ifdef WAYLIB_ENABLE_DEFAULT_PARAMETERS
		= "Waylib"
#endif
	, WAYLIB_PREFIXED(window_config) config
#ifdef WAYLIB_ENABLE_DEFAULT_PARAMETERS
		= {}
#endif
) WAYLIB_NOEXCEPT;

#endif // WAYLIB_WINDOW_IS_AVAILABLE