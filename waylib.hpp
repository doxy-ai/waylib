#pragma once
#include <webgpu/webgpu.hpp>

#include "math.hpp"

#include <cstdint>
#include <cstddef>
#include <array>
#include <filesystem>
#include <set>

#ifdef __cpp_exceptions
#include <stdexcept>
#endif

#ifdef WAYLIB_NAMESPACE_NAME
namespace WAYLIB_NAMESPACE_NAME {
inline namespace wgpu {
#else
namespace wgpu {
#endif

	using namespace ::wgpu;

	using instance = Instance;
	using adapter = Adapter;
	using device = Device;
	using surface = Surface;
	using queue = Queue;
}

#include "waylib.h"

constexpr static WGPUTextureFormat depth_texture_format = wgpu::TextureFormat::Depth24Plus;

struct shader_preprocessor {
	std::unordered_map<std::filesystem::path, std::string> file_cache;

	std::set<std::filesystem::path> search_paths;
	std::string defines = "";
};

struct pipeline_globals {
	bool created = false;
	// Group 0 is Instance Data
	// Group 1 is Time Data
	std::array<WGPUBindGroupLayout, 2> bindGroupLayouts;
	wgpu::PipelineLayout layout;
};

#ifndef WAYLIB_NO_CAMERAS
	// Camera, defines position/orientation in 3d space
	// From: raylib.h
	struct camera3D {
		WAYLIB_CAMERA_3D_MEMBERS
	};

	// Camera2D, defines position/orientation in 2d space
	// From: raylib.h
	struct camera2D {
		WAYLIB_CAMERA_2D_MEMBERS
	};
#endif // WAYLIB_NO_CAMERAS

// wgpu::Color to_webgpu(const color8bit& color);
wgpu::Color to_webgpu(const color& color);

std::string get_error_message_and_clear();
#ifdef __cpp_exceptions
	struct error: public std::runtime_error {
		using std::runtime_error::runtime_error;
	};

	template<typename T>
	T throw_if_null(const WAYLIB_OPTIONAL(T)& opt) {
		if(opt.has_value) return opt.value;

		auto msg = get_error_message_and_clear();
		throw error(msg.empty() ? "Null value encountered" : msg);
	}
#else
	template<typename T>
	T throw_if_null(const WAYLIB_OPTIONAL(T)& opt) {
		assert(opt.has_value, get_error_message());
		return opt.value;
	}
#endif
void set_error_message(const std::string_view view);
void set_error_message(const std::string& str);

void time_calculations(time& time);
void time_upload(
	webgpu_frame_state& frame, 
	time& time
);

void end_drawing(
	webgpu_frame_state& frame
);

#ifndef WAYLIB_NO_CAMERAS
mat4x4f camera3D_get_matrix(
	camera3D& camera, 
	vec2i window_dimensions
);

mat4x4f camera2D_get_matrix(
	camera2D& camera, 
	vec2i window_dimensions
);

void begin_camera_mode3D(
	webgpu_frame_state& frame,
	camera3D& camera, 
	vec2i window_dimensions
);

void begin_camera_mode2D(
	webgpu_frame_state& frame,
	camera2D& camera, 
	vec2i window_dimensions
);

void end_camera_mode(
	webgpu_frame_state& frame
);
#endif // WAYLIB_NO_CAMERAS

#ifdef WAYLIB_NAMESPACE_NAME
}
#endif