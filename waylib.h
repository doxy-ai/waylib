#pragma once
#include "config.h"

#ifdef __cplusplus
extern "C" {
#endif

struct shader_preprocessor;

// Define mathematical types used... in C++ they are provided by glm so we just prototype them here!
typedef struct mat4x4f_ {
	float a0, a1, a2, a3;
	float b0, b1, b2, b3;
	float c0, c1, c2, c3;
	float d0, d1, d2, d3;
} mat4x4f_;

#ifndef __cplusplus
typedef struct vec2i {
	int32_t x, y;
} vec2i;
typedef struct vec2f {
	float x, y;
} vec2f;
typedef struct vec3f {
	float x, y, z;
} vec3f;
typedef struct vec4f {
	float x, y, z, w;
} vec4f;
typedef struct mat4x4f_ mat4x4f; // The version we use in the c++ side is quite a bit more advanced
#else
struct vec2i;
struct vec2f;
struct vec3f;
struct vec4f;
struct mat4x4f;
#endif

typedef struct {
	float r, g, b, a;
} color;

// typedef struct {
// 	unsigned char r, g, b, a;
// } color8bit;


// Shader stages
WAYLIB_ENUM shader_stage {
	C_PREPEND(SHADER_STAGE_, Compute) = 0,
	C_PREPEND(SHADER_STAGE_, Vertex),
	C_PREPEND(SHADER_STAGE_, Fragment),
	// TODO: Add
	C_PREPEND(SHADER_STAGE_, Max),
};
#define WAYLIB_SHADER_STAGE_COUNT WAYLIB_C_OR_CPP_TYPE(SHADER_STAGE_max, (index_t)shader_stage::Max)

// Shader
typedef struct shader {
	bool waylib_heap_allocated;// Wether or not this mesh is stored on the heap and should be automatically cleaned up
	// unsigned char* source;
	// size_t source_size;

	const char* compute_entry_point;
	const char* vertex_entry_point;
	const char* fragment_entry_point;
	WAYLIB_C_OR_CPP_TYPE(WGPUShaderModule, wgpu::ShaderModule) module;
} shader;

// Material
typedef struct material {
	bool waylib_heap_allocated;// Wether or not this mesh is stored on the heap and should be automatically cleaned up
	index_t shaderCount;
	shader* shaders;
	WAYLIB_C_OR_CPP_TYPE(WGPURenderPipeline, wgpu::RenderPipeline) pipeline;
} material;

// Mesh, vertex data
// From: raylib.h
typedef struct mesh {
	bool waylib_heap_allocated;// Wether or not this mesh is stored on the heap and should be automatically cleaned up
	index_t vertexCount;       // Number of vertices stored in arrays
	index_t triangleCount;     // Number of triangles stored (indexed or not)

	// Vertex attributes data
	vec3f* positions;          // Vertex position (shader-location = 0)
	vec2f* texcoords;          // Vertex texture coordinates (shader-location = 1)
	vec2f* texcoords2;         // Vertex texture second coordinates (shader-location = 5)
	vec3f* normals;            // Vertex normals (shader-location = 2)
	vec4f* tangents;           // Vertex tangents (shader-location = 4)
	color* colors;             // Vertex colors (shader-location = 3)
	index_t* indices;          // Vertex indices (in case vertex data comes indexed)

	// Animation vertex data
	vec3f* anim_vertices;      // Animated vertex positions (after bones transformations)
	vec3f* anim_normals;       // Animated normals (after bones transformations)
	unsigned char* bone_ids;   // Vertex bone ids, max 255 bone ids, up to 4 bones influence by vertex (skinning)
	vec4f* bone_weights;       // Vertex bone weight, up to 4 bones influence by vertex (skinning)

	WAYLIB_C_OR_CPP_TYPE(WGPUBuffer, wgpu::Buffer) buffer; // Pointer to the data on the gpu
	WAYLIB_C_OR_CPP_TYPE(WGPUBuffer, wgpu::Buffer) indexBuffer; // Pointer to the index data on the gpu
} mesh;

// Bone, skeletal animation bone
// From: raylib.h
typedef struct bone_info {
	char name[32];          // Bone name
	index_t parent;         // Bone parent
	mat4x4f_ bind_pose;     // Bones base transformation (pose) // TODO: Why was this in model?

#ifdef WAYLIB_ENABLE_CLASSES
	mat4x4f& get_bind_pose() { return *(mat4x4f*)&bind_pose; }
#endif
} bone_info;

// Model, meshes, materials and animation data
// From: raylib.h
typedef struct model {
	mat4x4f_ transform;      // Local transform matrix

	index_t mesh_count;      // Number of meshes
	index_t material_count;  // Number of materials
	mesh* meshes;            // Meshes array
	material* materials;     // Materials array
	index_t* mesh_materials; // Mesh material number

	// Animation data
	index_t bone_count;      // Number of bones
	bone_info* bones;        // Bones information (skeleton)

#ifdef WAYLIB_ENABLE_CLASSES
	mat4x4f& get_transform() { return *(mat4x4f*)&transform; }
#endif
} model;

// Format of the data stored in the image
// WAYLIB_ENUM image_format {
// 	C_PREPEND(IMAGE_FORMAT_, RGBA8) = 0,
// 	// C_PREPEND(IMAGE_FORMAT_, RGB8),
// 	// C_PREPEND(IMAGE_FORMAT_, Gray8),
// 	C_PREPEND(IMAGE_FORMAT_, RGBAF32),
// 	// C_PREPEND(IMAGE_FORMAT_, RGBF32),
// 	// C_PREPEND(IMAGE_FORMAT_, Gray32),
// };

// Image, pixel data stored in CPU memory (RAM)
// From: raylib.h
typedef struct image {
	color* data;            // Image raw data
	int width;              // Image base width
	int height;             // Image base height
	int mipmaps;            // Mipmap levels, 1 by default
	// image_format format;    // Data format (PixelFormat type)
} image;

// Struct holding all of the state needed by webgpu functions
typedef struct webgpu_state {
	WAYLIB_C_OR_CPP_TYPE(WGPUDevice, wgpu::Device) device;
	WAYLIB_C_OR_CPP_TYPE(WGPUSurface, wgpu::Surface) surface;
} webgpu_state;

typedef struct webgpu_frame_state {
	WAYLIB_C_OR_CPP_TYPE(WGPUTextureView, wgpu::TextureView) target;
	WAYLIB_C_OR_CPP_TYPE(WGPUCommandEncoder, wgpu::CommandEncoder) encoder;
	WAYLIB_C_OR_CPP_TYPE(WGPURenderPassEncoder, wgpu::RenderPassEncoder) render_pass;
} webgpu_frame_state;

typedef struct time {
	float since_start;
	float delta;
	float average_delta;
} time;


typedef struct surface_configuration {
	WGPUPresentMode presentaion_mode
#ifdef WAYLIB_ENABLE_DEFAULT_PARAMETERS
		= wgpu::PresentMode::Mailbox
#endif
	; WGPUCompositeAlphaMode alpha_mode
#ifdef WAYLIB_ENABLE_DEFAULT_PARAMETERS
		= wgpu::CompositeAlphaMode::Auto
#endif
	; bool automatic_should_configure_now
#ifdef WAYLIB_ENABLE_DEFAULT_PARAMETERS
		= true
#endif
	;
} surface_configuration;

struct preprocess_shader_config {
	bool remove_comments
#ifdef WAYLIB_ENABLE_DEFAULT_PARAMETERS
		= false
#endif
	; bool remove_whitespace
#ifdef WAYLIB_ENABLE_DEFAULT_PARAMETERS
		= false
#endif
	; bool support_pragma_once
#ifdef WAYLIB_ENABLE_DEFAULT_PARAMETERS
		= true
#endif
	; const char* path
#ifdef WAYLIB_ENABLE_DEFAULT_PARAMETERS
		= nullptr
#endif
	;
};

typedef struct create_shader_configuration {
	const char* compute_entry_point
#ifdef WAYLIB_ENABLE_DEFAULT_PARAMETERS
		= nullptr
#endif
	; const char* vertex_entry_point
#ifdef WAYLIB_ENABLE_DEFAULT_PARAMETERS
		= nullptr
#endif
	; const char* fragment_entry_point
#ifdef WAYLIB_ENABLE_DEFAULT_PARAMETERS
		= nullptr
#endif
	; const char* name
#ifdef WAYLIB_ENABLE_DEFAULT_PARAMETERS
		= nullptr
#endif
	; shader_preprocessor* preprocessor
	; preprocess_shader_config preprocess_config
#ifdef WAYLIB_ENABLE_DEFAULT_PARAMETERS
		= {}
#endif
	;
} create_shader_configuration;





const char* get_error_message();
void set_error_message_raw(const char* message);
void clear_error_message();

void time_calculations(time* time);

WAYLIB_C_OR_CPP_TYPE(WGPUDevice, wgpu::Device) create_default_device_from_instance(
	WGPUInstance instance,
	WGPUSurface surface
#ifdef WAYLIB_ENABLE_DEFAULT_PARAMETERS
		= nullptr
#endif
	, bool prefer_low_power
#ifdef WAYLIB_ENABLE_DEFAULT_PARAMETERS
		= false
#endif
);

WAYLIB_C_OR_CPP_TYPE(WGPUDevice, wgpu::Device) create_default_device(
	WGPUSurface surface
#ifdef WAYLIB_ENABLE_DEFAULT_PARAMETERS
		= nullptr
#endif
	, bool prefer_low_power
#ifdef WAYLIB_ENABLE_DEFAULT_PARAMETERS
		= false
#endif
);

void release_device(
	WGPUDevice device,
	bool also_release_adapter
#ifdef WAYLIB_ENABLE_DEFAULT_PARAMETERS
		= true
#endif
	, bool also_release_instance
#ifdef WAYLIB_ENABLE_DEFAULT_PARAMETERS
		= true
#endif
);

void release_webgpu_state(
	webgpu_state state
);

bool configure_surface(
	webgpu_state state,
	vec2i size,
	surface_configuration config
#ifdef WAYLIB_ENABLE_DEFAULT_PARAMETERS
		= {}
#endif
);

shader_preprocessor* create_shader_preprocessor();

void release_shader_preprocessor(
	shader_preprocessor* processor
);

void preprocessor_add_define(
	shader_preprocessor* processor, 
	const char* name, 
	const char* value
);

bool preprocessor_add_search_path(
	shader_preprocessor* processor, 
	const char* _path
);

const char* preprocessor_get_cached_file(
	shader_preprocessor* processor, 
	const char* path
);

const char* preprocess_shader_from_memory(
	shader_preprocessor* processor, 
	const char* data, 
	preprocess_shader_config config
#ifdef WAYLIB_ENABLE_DEFAULT_PARAMETERS
		= {}
#endif
);

const char* preprocess_shader_from_memory_and_cache(
	shader_preprocessor* processor, 
	const char* data, 
	const char* path, 
	preprocess_shader_config config
#ifdef WAYLIB_ENABLE_DEFAULT_PARAMETERS
		= {}
#endif
);

const char* preprocess_shader(
	shader_preprocessor* processor, 
	const char* path, 
	preprocess_shader_config config
#ifdef WAYLIB_ENABLE_DEFAULT_PARAMETERS
		= {}
#endif
);

WAYLIB_OPTIONAL(shader) create_shader(
	webgpu_state state,
	const char* wgsl_source_code,
	create_shader_configuration config
#ifdef WAYLIB_ENABLE_DEFAULT_PARAMETERS
		= {}
#endif
);

WAYLIB_OPTIONAL(webgpu_frame_state) begin_drawing_render_texture(
	webgpu_state state,
	WGPUTextureView render_texture,
	WAYLIB_OPTIONAL(color) clear_color
#ifdef WAYLIB_ENABLE_DEFAULT_PARAMETERS
		= {}
#endif
);

WAYLIB_OPTIONAL(webgpu_frame_state) begin_drawing(
	webgpu_state state,
	WAYLIB_OPTIONAL(color) clear_color
#ifdef WAYLIB_ENABLE_DEFAULT_PARAMETERS
		= {}
#endif
);

void end_drawing(
	webgpu_state state,
	webgpu_frame_state frame
);

#ifdef __cplusplus
} // End extern "C"
#endif
