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

void release_shader(
	shader& shader
);

void mesh_upload(
	wgpu_state state,
	mesh& mesh
);

void release_mesh(
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

void release_material(
	material& material,
	bool release_textures
#ifdef WAYLIB_ENABLE_DEFAULT_PARAMETERS
		= true
#endif
	, bool release_shaders
#ifdef WAYLIB_ENABLE_DEFAULT_PARAMETERS
		= true
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

void release_model(
	model& model,
	bool release_meshes
#ifdef WAYLIB_ENABLE_DEFAULT_PARAMETERS
		= true
#endif
	, bool release_materials
#ifdef WAYLIB_ENABLE_DEFAULT_PARAMETERS
		= true
#endif
	, bool release_textures
#ifdef WAYLIB_ENABLE_DEFAULT_PARAMETERS
		= true
#endif
	, bool release_shaders
#ifdef WAYLIB_ENABLE_DEFAULT_PARAMETERS
		= true
#endif
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

void release_image(
	image& image
);

void release_texture(
	texture& texture
);

void upload_buffer(
	wgpu_state state, 
	buffer& buffer, 
	WGPUBufferUsageFlags usage 
#ifdef WAYLIB_ENABLE_DEFAULT_PARAMETERS
		= wgpu::BufferUsage::CopyDst | wgpu::BufferUsage::Storage
#endif
	, bool free_cpu_data
#ifdef WAYLIB_ENABLE_DEFAULT_PARAMETERS
	= false
#endif
);

buffer create_buffer(
	wgpu_state state, 
	std::span<std::byte> data, 
	WGPUBufferUsageFlags usage
#ifdef WAYLIB_ENABLE_DEFAULT_PARAMETERS
		 = wgpu::BufferUsage::CopyDst | wgpu::BufferUsage::Storage
#endif
	, WAYLIB_OPTIONAL(std::string_view) label
#ifdef WAYLIB_ENABLE_DEFAULT_PARAMETERS
		 = {}
#endif
);

template<typename T>
inline buffer create_buffer(
	wgpu_state state, 
	T& data, 
	WGPUBufferUsageFlags usage = wgpu::BufferUsage::CopyDst | wgpu::BufferUsage::Storage, 
	WAYLIB_OPTIONAL(std::string_view) label = {}
){
	return create_buffer(state, std::span<std::byte>{(std::byte*)&data, sizeof(T)}, usage, label);
}

template<typename T>
requires(!std::is_same_v<T, std::byte>)
inline buffer create_buffer(
	wgpu_state state, 
	std::span<T> data, 
	WGPUBufferUsageFlags usage 
#ifdef WAYLIB_ENABLE_DEFAULT_PARAMETERS
		= wgpu::BufferUsage::CopyDst | wgpu::BufferUsage::Storage, 
#endif
	WAYLIB_OPTIONAL(std::string_view) label 
#ifdef WAYLIB_ENABLE_DEFAULT_PARAMETERS
		= {}
#endif
){
	return create_buffer(state, std::span<std::byte>{(std::byte*)data.data(), data.size() * sizeof(T)}, usage, label);
}

void* buffer_map(
	wgpu_state state, 
	buffer& buffer, 
	WGPUMapModeFlags mode 
#ifdef WAYLIB_ENABLE_DEFAULT_PARAMETERS
		= wgpu::MapMode::None
#endif
);
const void* buffer_map_const(
	wgpu_state state, 
	buffer& buffer, 
	WGPUMapModeFlags mode 
#ifdef WAYLIB_ENABLE_DEFAULT_PARAMETERS
		= wgpu::MapMode::None
#endif
);

template<typename T>
inline T& buffer_map(
	wgpu_state state, 
	buffer& buffer,
	WGPUMapModeFlags mode 
#ifdef WAYLIB_ENABLE_DEFAULT_PARAMETERS
		= wgpu::MapMode::None
#endif
) {
	assert(buffer.size == sizeof(T));
	return *(T*)buffer_map(state, buffer, mode);
}

template<typename T>
inline const T& buffer_map_const(
	wgpu_state state, 
	buffer& buffer,
	WGPUMapModeFlags mode 
#ifdef WAYLIB_ENABLE_DEFAULT_PARAMETERS
		= wgpu::MapMode::None
#endif
) {
	assert(buffer.size == sizeof(T));
	return *(T*)buffer_map_const(state, buffer, mode);
}

template<typename T>
inline std::span<T> buffer_map(
	wgpu_state state, 
	buffer& buffer, 
	WGPUMapModeFlags mode 
#ifdef WAYLIB_ENABLE_DEFAULT_PARAMETERS
		= wgpu::MapMode::None
#endif
) {
	assert((buffer.size % sizeof(T)) == 0);
	return {(T*)buffer_map(state, buffer, mode), buffer.size / sizeof(T)};
}

template<typename T>
inline std::span<const T> buffer_map_const(
	wgpu_state state, 
	buffer& buffer,
	WGPUMapModeFlags mode 
#ifdef WAYLIB_ENABLE_DEFAULT_PARAMETERS
		= wgpu::MapMode::None
#endif
) {
	assert((buffer.size % sizeof(T)) == 0);
	return {(const T*)buffer_map_const(state, buffer, mode), buffer.size / sizeof(T)};
}

void buffer_unmap(buffer& buffer);

void buffer_release(buffer& buffer);

void buffer_copy_record_existing(
	WGPUCommandEncoder encoder, 
	buffer& dest, 
	buffer& source
);

void buffer_copy(
	wgpu_state state, 
	buffer& dest, 
	buffer& source
);

void buffer_download(
	wgpu_state state, 
	buffer& buffer, 
	bool create_intermediate_buffer 
#ifdef WAYLIB_ENABLE_DEFAULT_PARAMETERS
		= true
#endif
);

void upload_computer(
	wgpu_state state, 
	computer& compute, 
	WAYLIB_OPTIONAL(std::string_view) label
#ifdef WAYLIB_ENABLE_DEFAULT_PARAMETERS
		= {}
#endif
);

computer_recording_state computer_record_existing(
	wgpu_state state, 
	WGPUCommandEncoder encoder, 
	computer& compute, 
	vec3u workgroups, 
	bool end
#ifdef WAYLIB_ENABLE_DEFAULT_PARAMETERS
		= true
#endif
);

void computer_dispatch(
	wgpu_state state, 
	computer& compute, 
	vec3u workgroups
);

void computer_release(
	computer& compute, 
	bool free_shader
#ifdef WAYLIB_ENABLE_DEFAULT_PARAMETERS
		= true
#endif
	, bool free_buffers
#ifdef WAYLIB_ENABLE_DEFAULT_PARAMETERS
		= false
#endif
	, bool free_textures
#ifdef WAYLIB_ENABLE_DEFAULT_PARAMETERS
		= false
#endif
);

void quick_dispatch(
	wgpu_state state, 
	std::span<buffer> buffers, 
	std::span<texture> textures, 
	shader compute_shader, 
	vec3u workgroups
);

#ifdef WAYLIB_NAMESPACE_NAME
}
#endif