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
	frame_state& frame,
	WAYLIB_OPTIONAL(camera_upload_data&) data,
	std::span<light> lights,
	WAYLIB_OPTIONAL(frame_time) frame_time
);

void end_drawing(
	frame_state& frame
);

void present_frame(
	frame_state& frame
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
	frame_state& frame,
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
	frame_state& frame,
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
	frame_state& frame,
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

void reset_camera_mode(
	frame_state& frame
);
#endif // WAYLIB_NO_CAMERAS

#ifndef WAYLIB_NO_LIGHTS
void light_upload(
	frame_state& frame,
	std::span<light> lights
);

// void begin_light_mode(
// 	frame_state& frame,
// 	light& light,
// 	vec2i shadow_map_dimensions
// );
#endif // WAYLIB_NO_LIGHTS

void release_shader(
	shader& shader
);

void release_geometry_transformation_shader(
	geometry_transformation_shader& shader
);

void mesh_generate_normals(
	mesh& mesh,
	bool weighted_normals
#ifdef WAYLIB_ENABLE_DEFAULT_PARAMETERS
		= false
#endif
);

void mesh_generate_tangents(mesh& mesh);

void mesh_upload(
	waylib_state state,
	mesh& mesh
);

void release_mesh(
	mesh& mesh
);

void material_upload(
	waylib_state state,
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
	, bool release_transformer
#ifdef WAYLIB_ENABLE_DEFAULT_PARAMETERS
		= true
#endif
);

material create_material(
	waylib_state state,
	std::span<shader> shaders,
	material_configuration config
#ifdef WAYLIB_ENABLE_DEFAULT_PARAMETERS
		= {}
#endif
);
material create_material(
	waylib_state state,
	shader& shader,
	material_configuration config
#ifdef WAYLIB_ENABLE_DEFAULT_PARAMETERS
		= {}
#endif
);


wgpu::BindGroupEntry pbr_material_default_data_binding_function(frame_state& frame, material& _mat);

pbr_material create_pbr_material(
	waylib_state state,
	std::span<shader> shaders,
	std::span<WAYLIB_NULLABLE(texture*)> textures
#ifdef WAYLIB_ENABLE_DEFAULT_PARAMETERS
		= {}
#endif
	, material_configuration config
#ifdef WAYLIB_ENABLE_DEFAULT_PARAMETERS
		= {}
#endif
);
// TODO: Do we always want the option of a geometry shader?
pbr_material create_pbr_material(
	waylib_state state,
	shader& shader,
	std::span<WAYLIB_NULLABLE(texture*)> textures
#ifdef WAYLIB_ENABLE_DEFAULT_PARAMETERS
		= {}
#endif
	, material_configuration config
#ifdef WAYLIB_ENABLE_DEFAULT_PARAMETERS
		= {}
#endif
);

WAYLIB_OPTIONAL(pbr_material) create_default_pbr_material(
	waylib_state state,
	std::span<WAYLIB_NULLABLE(texture*)> textures
#ifdef WAYLIB_ENABLE_DEFAULT_PARAMETERS
		= {}
#endif
	, material_configuration config
#ifdef WAYLIB_ENABLE_DEFAULT_PARAMETERS
		= {}
#endif
);

void model_upload(
	waylib_state state,
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
	waylib_state state,
	std::span<std::byte> data,
	model_process_configuration config
#ifdef WAYLIB_ENABLE_DEFAULT_PARAMETERS
		= default_model_process_configuration()
#endif
);

void model_draw_instanced(
	frame_state& frame,
	model& model,
	std::span<model_instance_data> instances
);

void model_draw(
	frame_state& frame,
	model& model
);

WAYLIB_OPTIONAL(texture) create_texture_from_image(
	waylib_state state,
	image& image,
	texture_config config
#ifdef WAYLIB_ENABLE_DEFAULT_PARAMETERS
		= {}
#endif
);

WAYLIB_OPTIONAL(texture) create_texture_from_image(
	waylib_state state,
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

void gpu_buffer_upload(
	waylib_state state, 
	gpu_buffer& gpu_buffer, 
	WGPUBufferUsageFlags usage 
#ifdef WAYLIB_ENABLE_DEFAULT_PARAMETERS
		= wgpu::BufferUsage::CopyDst | wgpu::BufferUsage::Storage
#endif
	, bool free_cpu_data
#ifdef WAYLIB_ENABLE_DEFAULT_PARAMETERS
	= false
#endif
);

gpu_buffer create_gpu_buffer(
	waylib_state state, 
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
inline gpu_buffer create_gpu_buffer(
	waylib_state state, 
	T& data, 
	WGPUBufferUsageFlags usage = wgpu::BufferUsage::CopyDst | wgpu::BufferUsage::Storage, 
	WAYLIB_OPTIONAL(std::string_view) label = {}
){
	return create_gpu_buffer(state, std::span<std::byte>{(std::byte*)&data, sizeof(T)}, usage, label);
}

template<typename T>
requires(!std::is_same_v<T, std::byte>)
inline gpu_buffer create_gpu_buffer(
	waylib_state state, 
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
	return create_gpu_buffer(state, std::span<std::byte>{(std::byte*)data.data(), data.size() * sizeof(T)}, usage, label);
}

void* gpu_buffer_map(
	waylib_state state, 
	gpu_buffer& gpu_buffer, 
	WGPUMapModeFlags mode 
#ifdef WAYLIB_ENABLE_DEFAULT_PARAMETERS
		= wgpu::MapMode::None
#endif
);
const void* gpu_buffer_map_const(
	waylib_state state, 
	gpu_buffer& gpu_buffer, 
	WGPUMapModeFlags mode 
#ifdef WAYLIB_ENABLE_DEFAULT_PARAMETERS
		= wgpu::MapMode::None
#endif
);

template<typename T>
inline T& gpu_buffer_map(
	waylib_state state, 
	gpu_buffer& gpu_buffer,
	WGPUMapModeFlags mode 
#ifdef WAYLIB_ENABLE_DEFAULT_PARAMETERS
		= wgpu::MapMode::None
#endif
) {
	assert(gpu_buffer.size == sizeof(T));
	return *(T*)gpu_buffer_map(state, gpu_buffer, mode);
}

template<typename T>
inline const T& gpu_buffer_map_const(
	waylib_state state, 
	gpu_buffer& gpu_buffer,
	WGPUMapModeFlags mode 
#ifdef WAYLIB_ENABLE_DEFAULT_PARAMETERS
		= wgpu::MapMode::None
#endif
) {
	assert(gpu_buffer.size == sizeof(T));
	return *(T*)gpu_buffer_map_const(state, gpu_buffer, mode);
}

template<typename T>
inline std::span<T> gpu_buffer_map(
	waylib_state state, 
	gpu_buffer& gpu_buffer, 
	WGPUMapModeFlags mode 
#ifdef WAYLIB_ENABLE_DEFAULT_PARAMETERS
		= wgpu::MapMode::None
#endif
) {
	assert((gpu_buffer.size % sizeof(T)) == 0);
	return {(T*)gpu_buffer_map(state, gpu_buffer, mode), gpu_buffer.size / sizeof(T)};
}

template<typename T>
inline std::span<const T> gpu_buffer_map_const(
	waylib_state state, 
	gpu_buffer& gpu_buffer,
	WGPUMapModeFlags mode 
#ifdef WAYLIB_ENABLE_DEFAULT_PARAMETERS
		= wgpu::MapMode::None
#endif
) {
	assert((gpu_buffer.size % sizeof(T)) == 0);
	return {(const T*)gpu_buffer_map_const(state, gpu_buffer, mode), gpu_buffer.size / sizeof(T)};
}

void gpu_buffer_unmap(gpu_buffer& gpu_buffer);

void release_gpu_buffer(gpu_buffer& gpu_buffer);

void gpu_buffer_copy_record_existing(
	WGPUCommandEncoder encoder, 
	gpu_buffer& dest, 
	const gpu_buffer& source
);

void gpu_buffer_copy(
	waylib_state state, 
	gpu_buffer& dest, 
	const gpu_buffer& source
);

void gpu_buffer_download(
	waylib_state state, 
	gpu_buffer& gpu_buffer, 
	bool create_intermediate_gpu_buffer 
#ifdef WAYLIB_ENABLE_DEFAULT_PARAMETERS
		= true
#endif
);

void computer_upload(
	waylib_state state, 
	computer& compute, 
	WAYLIB_OPTIONAL(std::string_view) label
#ifdef WAYLIB_ENABLE_DEFAULT_PARAMETERS
		= {}
#endif
);

computer_recording_state computer_record_existing(
	waylib_state state,
	WGPUCommandEncoder encoder,
	computer& compute,
	vec3u workgroups,
	bool end
#ifdef WAYLIB_ENABLE_DEFAULT_PARAMETERS
		= true
#endif
);

void computer_dispatch(
	waylib_state state,
	computer& compute,
	vec3u workgroups
);

void release_computer(
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
	waylib_state state, 
	std::span<gpu_buffer> buffers, 
	std::span<texture> textures, 
	shader compute_shader, 
	vec3u workgroups
);

#ifdef WAYLIB_NAMESPACE_NAME
}
#endif