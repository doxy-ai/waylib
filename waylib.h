#ifndef WAYLIB_IS_AVAILABLE
#define WAYLIB_IS_AVAILABLE
#include "math.h"

#ifdef __cplusplus
extern "C" {
#endif

struct shader_preprocessor;
struct camera_globals;

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
	WAYLIB_C_OR_CPP_TYPE(WGPUBuffer, wgpu::Buffer) instanceBuffer; // Pointer to the per-instance data on the gpu
} mesh;

// Bone, skeletal animation bone
// From: raylib.h
typedef struct bone_info {
	char name[32];          // Bone name
	index_t parent;         // Bone parent
	mat4x4f_ bind_pose;     // Bones base transformation (pose) // TODO: Why was this in model?

#ifdef WAYLIB_ENABLE_CLASSES
	inline mat4x4f& get_bind_pose() { return *(mat4x4f*)&bind_pose; }
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
	inline mat4x4f& get_transform() { return *(mat4x4f*)&transform; }
#endif
} model;

typedef struct model_instance_data {
	mat4x4f_ transform
	; color tint
#ifdef WAYLIB_ENABLE_DEFAULT_PARAMETERS
		= {1, 1, 1, 1}
#endif
	;

#ifdef WAYLIB_ENABLE_CLASSES
	inline mat4x4f& get_transform() { return *(mat4x4f*)&transform; }
#endif
} model_instance_data;

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

#ifndef WAYLIB_NO_CAMERAS
	#ifdef WAYLIB_ENABLE_DEFAULT_PARAMETERS
		#define WAYLIB_CAMERA_3D_MEMBERS\
			vec3f position; /* Camera position */\
			vec3f target; /* Camera target it looks-at*/\
			vec3f up = {0, 1, 0}; /* Camera up vector (rotation over its axis)*/\
			float field_of_view = 90; /* Camera field-of-view aperture in Y (degrees) in perspective, used as near plane width in orthographic */\
			float near_clip_distance = .01, far_clip_distance = 1000; /* Distances before objects are culled */\
			bool orthographic = false; /* True if the camera should use orthographic projection */

		#define WAYLIB_CAMERA_2D_MEMBERS\
			vec3f offset; /* Camera offset (displacement from target)*/\
			vec3f target; /* Camera target (rotation and zoom origin)*/\
			degree rotation = 0; /* Camera rotation in degrees*/\
			float near_clip_distance = .01, far_clip_distance = 1000; /* Distances before objects are culled */\
			float zoom = 1; /* Camera zoom (scaling), should be 1.0f by default*/
	#else
		#define WAYLIB_CAMERA_3D_MEMBERS\
			vec3f position; /* Camera position */\
			vec3f target; /* Camera target it looks-at*/\
			vec3f up; /* Camera up vector (rotation over its axis)*/\
			float field_of_view; /* Camera field-of-view aperture in Y (degrees) in perspective, used as near plane width in orthographic */\
			float near_clip_distance, far_clip_distance; /* Distances before objects are culled */\
			bool orthographic; /* True if the camera should use orthographic projection */

		#define WAYLIB_CAMERA_2D_MEMBERS\
			vec3f offset; /* Camera offset (displacement from target)*/\
			vec3f target; /* Camera target (rotation and zoom origin)*/\
			degree rotation; /* Camera rotation in degrees*/\
			float near_clip_distance, far_clip_distance; /* Distances before objects are culled */\
			float zoom; /* Camera zoom (scaling), should be 1.0f by default*/
	#endif // WAYLIB_ENABLE_DEFAULT_PARAMETERS

#ifndef __cplusplus
	// Camera, defines position/orientation in 3d space
	// From: raylib.h
	typedef struct camera3D {
		WAYLIB_CAMERA_3D_MEMBERS
	} camera3D;

	// Camera2D, defines position/orientation in 2d space
	// From: raylib.h
	typedef struct camera2D {
		WAYLIB_CAMERA_2D_MEMBERS
	} camera2D;
#else
	struct camera3D;
	struct camera2D;
#endif

typedef camera3D camera;	// By default a camera is a 3D camera
#endif // WAYLIB_NO_CAMERAS

// Struct holding all of the state needed by webgpu functions
typedef struct webgpu_state {
	WAYLIB_C_OR_CPP_TYPE(WGPUDevice, wgpu::Device) device;
	WAYLIB_C_OR_CPP_TYPE(WGPUSurface, wgpu::Surface) surface;
} webgpu_state;

typedef struct webgpu_frame_state {
	webgpu_state state;
	WAYLIB_C_OR_CPP_TYPE(WGPUTextureView, wgpu::TextureView) color_target;
	WAYLIB_C_OR_CPP_TYPE(WGPUTexture, wgpu::Texture) depth_texture;
	WAYLIB_C_OR_CPP_TYPE(WGPUTextureView, wgpu::TextureView) depth_target;
	WAYLIB_C_OR_CPP_TYPE(WGPUCommandEncoder, wgpu::CommandEncoder) encoder;
	WAYLIB_C_OR_CPP_TYPE(WGPURenderPassEncoder, wgpu::RenderPassEncoder) render_pass;
	mat4x4f_ current_VP;

#ifdef WAYLIB_ENABLE_CLASSES
	inline mat4x4f& get_current_view_projection_matrix() { return *(mat4x4f*)&current_VP; }
#endif
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
void time_upload(
	webgpu_frame_state* frame, 
	time* time
);

bool open_url(const char* url);

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
	vec2i render_texture_dimensions,
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
	webgpu_frame_state* frame
);

#ifndef WAYLIB_NO_CAMERAS
WAYLIB_C_OR_CPP_TYPE(mat4x4f_, mat4x4f) camera3D_get_matrix(
	camera3D* camera, 
	vec2i window_dimensions
);

WAYLIB_C_OR_CPP_TYPE(mat4x4f_, mat4x4f) camera2D_get_matrix(
	camera2D* camera, 
	vec2i window_dimensions
);

void begin_camera_mode3D(
	webgpu_frame_state* frame,
	camera3D* camera, 
	vec2i window_dimensions
);

void begin_camera_mode2D(
	webgpu_frame_state* frame,
	camera2D* camera, 
	vec2i window_dimensions
);

void end_camera_mode(
	webgpu_frame_state* frame
);
#endif // WAYLIB_NO_CAMERAS

#ifdef __cplusplus
} // End extern "C"
#endif

#endif // WAYLIB_IS_AVAILABLE