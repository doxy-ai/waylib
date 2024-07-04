#pragma once
#include "common.hpp"

#include <filesystem>
#include <set>

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

struct shader_preprocessor {
	std::unordered_map<std::filesystem::path, std::string> file_cache;

	std::set<std::filesystem::path> search_paths;
	std::string defines = "";
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

void time_calculations(time& time);
void time_upload(
	wgpu_frame_state& frame, 
	time& time
);

void end_drawing(
	wgpu_frame_state& frame
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
	wgpu_frame_state& frame,
	camera3D& camera, 
	vec2i window_dimensions
);

void begin_camera_mode2D(
	wgpu_frame_state& frame,
	camera2D& camera, 
	vec2i window_dimensions
);

void end_camera_mode(
	wgpu_frame_state& frame
);
#endif // WAYLIB_NO_CAMERAS

#ifdef WAYLIB_NAMESPACE_NAME
}
#endif