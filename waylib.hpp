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





// wgpu::Color to_webgpu(const color8bit& color);
wgpu::Color to_webgpu(const color& color);

void time_calculations(frame_time& frame_time);

void upload_utility_data(
	wgpu_frame_state& frame, 
	WAYLIB_OPTIONAL(camera_upload_data&) data,
	std::span<light> lights, 
	WAYLIB_OPTIONAL(frame_time) frame_time
);

void end_drawing(
	wgpu_frame_state& frame
);

void present_frame(
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
	vec2i window_dimensions,
	std::span<light> lights
#ifdef WAYLIB_ENABLE_DEFAULT_PARAMETERS
		= {}
#endif
	, WAYLIB_OPTIONAL(frame_time) frame_time
#ifdef WAYLIB_ENABLE_DEFAULT_PARAMETERS
		= {}
#endif
);

void begin_camera_mode2D(
	wgpu_frame_state& frame,
	camera2D& camera,
	vec2i window_dimensions,
	std::span<light> lights
#ifdef WAYLIB_ENABLE_DEFAULT_PARAMETERS
		= {}
#endif
	, WAYLIB_OPTIONAL(frame_time) frame_time
#ifdef WAYLIB_ENABLE_DEFAULT_PARAMETERS
		= {}
#endif
);

void begin_camera_mode_identity(
	wgpu_frame_state& frame,
	vec2i window_dimensions,
	std::span<light> lights
#ifdef WAYLIB_ENABLE_DEFAULT_PARAMETERS
		= {}
#endif
	, WAYLIB_OPTIONAL(frame_time) frame_time
#ifdef WAYLIB_ENABLE_DEFAULT_PARAMETERS
		= {}
#endif
);

void end_camera_mode(
	wgpu_frame_state& frame
);
#endif // WAYLIB_NO_CAMERAS

#ifndef WAYLIB_NO_LIGHTS
void light_upload(
	wgpu_frame_state& frame,
	std::span<light> lights
);

// void begin_light_mode(
// 	wgpu_frame_state& frame,
// 	light& light,
// 	vec2i shadow_map_dimensions
// );
#endif // WAYLIB_NO_LIGHTS

void mesh_upload(
	wgpu_state state,
	mesh& mesh
);

void material_upload(
	wgpu_state state,
	material& material,
	material_configuration config
#ifdef WAYLIB_ENABLE_DEFAULT_PARAMETERS
		= {}
#endif
);

material create_material(
	wgpu_state state,
	std::span<shader> shaders,
	material_configuration config
#ifdef WAYLIB_ENABLE_DEFAULT_PARAMETERS
		= {}
#endif
);
material create_material(
	wgpu_state state,
	shader& shader,
	material_configuration config
#ifdef WAYLIB_ENABLE_DEFAULT_PARAMETERS
		= {}
#endif
);

void model_upload(
	wgpu_state state,
	model& model
);

WAYLIB_OPTIONAL(model) load_model_from_memory(
	wgpu_state state,
	std::span<std::byte> data,
	model_process_configuration config
#ifdef WAYLIB_ENABLE_DEFAULT_PARAMETERS
		= default_model_process_configuration()
#endif
);

void model_draw_instanced(
	wgpu_frame_state& frame,
	model& model,
	std::span<model_instance_data> instances
);

void model_draw(
	wgpu_frame_state& frame,
	model& model
);

WAYLIB_OPTIONAL(texture) create_texture_from_image(
	wgpu_state state, 
	image& image, 
	texture_config config
#ifdef WAYLIB_ENABLE_DEFAULT_PARAMETERS
		= {}
#endif
);

WAYLIB_OPTIONAL(texture) create_texture_from_image(
	wgpu_state state, 
	WAYLIB_OPTIONAL(image)&& image, 
	texture_config config
#ifdef WAYLIB_ENABLE_DEFAULT_PARAMETERS
		= {}
#endif
);

#ifdef WAYLIB_NAMESPACE_NAME
}
#endif