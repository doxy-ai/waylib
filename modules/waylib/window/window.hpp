#pragma once

#include "glfwpp/event.h"
#include "waylib/core/core.hpp"

#include <glfwpp/glfwpp.h>

WAYLIB_BEGIN_NAMESPACE

	extern "C" {
		#include "window.h"
	}

	void maybe_initialize_glfw();

	struct window {
		glfw::Window raw;

		static struct window create(vec2u size, std::string_view name = "Waylib", window_config config = {}) {
			maybe_initialize_glfw();
			glfw::WindowHints{
				.resizable = config.resizable,
				.visible = config.visible,
				.decorated = config.decorated,
				.focused = config.focused,
				.autoIconify = config.auto_iconify,
				.floating = config.floating,
				.maximized = config.maximized,
				.centerCursor = config.center_cursor,
				.transparentFramebuffer = config.transparent_framebuffer,
				.focusOnShow = config.focus_on_show,
				.scaleToMonitor = config.scale_to_monitor,

				.redBits = config.red_bits,
				.greenBits = config.green_bits,
				.blueBits = config.blue_bits,
				.alphaBits = config.alpha_bits,
				.depthBits = config.depth_bits,
				.stencilBits = config.stencil_bits,
				.accumRedBits = config.accum_red_bits,
				.accumGreenBits = config.accum_green_bits,
				.accumBlueBits = config.accum_blue_bits,
				.accumAlphaBits = config.accum_alpha_bits,

				.auxBuffers = config.aux_buffers,
				.samples = config.samples,
				.refreshRate = config.refresh_rate,
				.stereo = config.stereo,
				.srgbCapable = config.srgb_capable,
				.doubleBuffer = config.double_buffer,

				.clientApi = glfw::ClientApi::None,
			}.apply();

			return {glfw::Window(size.x, size.y, cstring_from_view(name), nullptr, nullptr)};
		}

		bool should_close(bool poll_events = true) {
			if(poll_events) glfw::pollEvents();
			return raw.shouldClose();
		}
	};

WAYLIB_END_NAMESPACE