#ifndef WAYLIB_COMMON_IS_AVAILABLE
#define WAYLIB_COMMON_IS_AVAILABLE
#include "math.h"

// Macro which defines an optional struct
#ifdef __cplusplus
	template<typename T>
	struct optional {
		bool has_value = false;
		T value;

		optional() requires(!std::is_reference_v<T>) : has_value(false) {}
		optional() requires(std::is_reference_v<T>) : has_value(false), value([]() -> T {
			static std::remove_reference_t<T> def = {};
			return def;
		}()) {}
		optional(const T& value) : has_value(true), value(value) {}
		optional(bool has_value, const T& value) : has_value(has_value), value(value) {}
		optional(const optional&) = default;
		optional(optional&&) = default;
		optional& operator=(const optional&) = default;
		optional& operator=(optional&&) = default;
	};
	#ifdef WAYLIB_NAMESPACE_NAME
		#define WAYLIB_OPTIONAL(type) WAYLIB_NAMESPACE_NAME::optional<type>
	#else
		#define WAYLIB_OPTIONAL(type) optional<type>
	#endif
#else
	#define WAYLIB_OPTIONAL(type) struct {\
		bool has_value;\
		type value;\
	}
#endif

#define WAYLIB_NULLABLE(type) type

#ifdef __cplusplus
extern "C" {
#endif

typedef uint32_t bool32;

struct shader_preprocessor;
struct camera_globals;

typedef struct color {
	float r, g, b, a;
} color;
#ifdef __cplusplus
	template struct optional<color>;
#endif

typedef struct color8 {
	unsigned char r, g, b, a;
} color8;
#ifdef __cplusplus
	template struct optional<color8>;
#endif

// typedef struct {
// 	unsigned char r, g, b, a;
// } color8bit;

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
	bool float_data;
	union {
		color8* data;
		color* data32;      // Image raw data
	};
	int width;              // Image base width
	int height;             // Image base height
	int mipmaps				// Mipmap levels, 1 by default
#ifdef WAYLIB_ENABLE_DEFAULT_PARAMETERS
		= 1
#endif
	; int frames			// Number of frames in an animated image, 1 by default
#ifdef WAYLIB_ENABLE_DEFAULT_PARAMETERS
		= 1
#endif
	// image_format format; // Data format (PixelFormat type)
	; bool heap_allocated // Wether or not this image is stored on the heap and should be automatically cleaned up
#ifdef WAYLIB_ENABLE_DEFAULT_PARAMETERS
		= false
#endif
	;
} image;
#ifdef __cplusplus
	template struct optional<image>;
#endif

WAYLIB_ENUM texture_slot {
	C_PREPEND(TEXTURE_SLOT_, Color) = 0,
	C_PREPEND(TEXTURE_SLOT_, Cubemap),
	C_PREPEND(TEXTURE_SLOT_, Height),
	C_PREPEND(TEXTURE_SLOT_, Normal),
	C_PREPEND(TEXTURE_SLOT_, PackedMap),
	C_PREPEND(TEXTURE_SLOT_, Roughness),
	C_PREPEND(TEXTURE_SLOT_, Metalness),
	C_PREPEND(TEXTURE_SLOT_, AmbientOcclusion),
	C_PREPEND(TEXTURE_SLOT_, Emission),
	// Count of how many slots are supported
	C_PREPEND(TEXTURE_SLOT_, Max),
};
#ifndef WAYLIB_NAMESPACE_NAME
	#define WAYLIB_TEXTURE_SLOT_COUNT WAYLIB_C_OR_CPP_TYPE(TEXTURE_SLOT_Max, (index_t)texture_slot::Max)
#else
	#define WAYLIB_TEXTURE_SLOT_COUNT WAYLIB_C_OR_CPP_TYPE(TEXTURE_SLOT_Max, (index_t)WAYLIB_NAMESPACE_NAME::texture_slot::Max)
#endif

typedef struct texture {
	image* cpu_data;
	WAYLIB_C_OR_CPP_TYPE(WGPUTexture, wgpu::Texture) gpu_data;
	WAYLIB_C_OR_CPP_TYPE(WGPUTextureView, wgpu::TextureView) view; // TODO: Needed?
	WAYLIB_C_OR_CPP_TYPE(WGPUSampler, wgpu::Sampler) sampler;
	bool heap_allocated // Wether or not this texture is stored on the heap and should be automatically cleaned up
#ifdef WAYLIB_ENABLE_DEFAULT_PARAMETERS
		= false
#endif
	;
} texture;
#ifdef __cplusplus
	template struct optional<texture>;
#endif

struct buffer {
	size_t size;
	size_t offset;
	uint8_t* cpu_data;
	WAYLIB_C_OR_CPP_TYPE(WGPUBuffer, wgpu::Buffer) data;
	const char* label;
	bool heap_allocated;
};
#ifdef __cplusplus
	template struct optional<buffer>;
#endif



// Shader
typedef struct shader {
	// unsigned char* source;
	// size_t source_size;

	const char* compute_entry_point;
	const char* vertex_entry_point;
	const char* fragment_entry_point;
	WAYLIB_C_OR_CPP_TYPE(WGPUShaderModule, wgpu::ShaderModule) module;
	bool heap_allocated // Wether or not this shader is stored on the heap and should be automatically cleaned up
#ifdef WAYLIB_ENABLE_DEFAULT_PARAMETERS
		= false
#endif
	;
} shader;
#ifdef __cplusplus
	template struct optional<shader>;
#endif

struct computer {
	index_t buffer_count;
	buffer* buffers;
	index_t texture_count;
	texture* textures;

	shader shader;
	WAYLIB_C_OR_CPP_TYPE(WGPUComputePipeline, wgpu::ComputePipeline) pipeline;
	bool heap_allocated;

#ifdef WAYLIB_ENABLE_CLASSES
	inline std::span<buffer> get_buffers() { return {buffers, buffer_count}; }
	inline std::span<texture> get_textures() { return {textures, texture_count}; }
#endif
};
#ifdef __cplusplus
	template struct optional<computer>;
#endif

struct geometry_transformation_shader {
	computer computer;

	// bool process_instances;
	bool per_vertex_processing; // When false a shader instance is dispatched per triangle, when true its dispatched per vertex
	bool force_vertex_count_sync; // Forces the system to assume that the vertex count changed and sync it with cpu
	float vertex_multiplier // The geometry output by the shader will have this many times more verticies than the input (rounded up to closest multiple of 3)
#ifdef WAYLIB_ENABLE_DEFAULT_PARAMETERS
		= 1
#endif
	;
};

struct frame_state;
struct material;
typedef WGPUBindGroupEntry(material_data_binding_function_t)(frame_state*, material*);

// Material
typedef struct material {
	index_t shaderCount;
	shader* shaders;
	WAYLIB_NULLABLE(texture*) textures[WAYLIB_TEXTURE_SLOT_COUNT];
	WAYLIB_C_OR_CPP_TYPE(WGPURenderPipeline, wgpu::RenderPipeline) pipeline;
	WAYLIB_NULLABLE(material_data_binding_function_t*) material_data_binding_function;
	WAYLIB_NULLABLE(geometry_transformation_shader*) geometry_transformer;
	bool heap_allocated // Wether or not this material is stored on the heap and should be automatically cleaned up
#ifdef WAYLIB_ENABLE_DEFAULT_PARAMETERS
		= false
#endif
	;

#ifdef WAYLIB_ENABLE_CLASSES
	inline std::span<shader> get_shaders() { return {shaders, shaderCount}; }
	inline std::span<WAYLIB_NULLABLE(texture*), WAYLIB_TEXTURE_SLOT_COUNT> get_textures() { return {textures}; }
#endif
} material;
#ifdef __cplusplus
	template struct optional<material>;
#endif

#ifdef WAYLIB_ENABLE_CLASSES
typedef struct pbr_material : public material {
#else
typedef struct pbr_material {
	material base; // "Inherits" from material
#endif

	vec4f_ color;
	vec4f_ emmision;
	float roughness;
	float metalness;

	float height_displacement_factor // NOTE: Unused by default pipeline
#ifdef WAYLIB_ENABLE_DEFAULT_PARAMETERS
		= 0
#endif
	; float normal_strength
#ifdef WAYLIB_ENABLE_DEFAULT_PARAMETERS
		= 1
#endif
	;

	bool32 use_color_map;
	bool32 use_height_map;
	bool32 use_normal_map;
	bool32 use_packed_map;
	bool32 use_roughness_map;
	bool32 use_metalness_map;
	bool32 use_ambient_occlusion_map;
	bool32 use_emmision_map;
	bool32 use_enviornment_map; // Located in cubemap slot

	char padding[20]; // Make it take up 96 bytes
} pbr_material;
#ifdef __cplusplus
	template struct optional<pbr_material>;
#endif

#ifndef WAYLIB_MATERIAL_DATA_SIZE
	#ifdef WAYLIB_NAMESPACE_NAME
		#define WAYLIB_MATERIAL_DATA_SIZE (sizeof(WAYLIB_NAMESPACE_NAME::pbr_material) - sizeof(WAYLIB_NAMESPACE_NAME::material) - sizeof(WAYLIB_NAMESPACE_NAME::material) % 16)
	#else
		#define WAYLIB_MATERIAL_DATA_SIZE (sizeof(pbr_material) - sizeof(material) - sizeof(material) % 16)
	#endif
#endif

struct mesh_metadata {
	bool32 is_indexed;
	uint32_t vertex_count;
	uint32_t triangle_count;

	uint32_t position_start;
	uint32_t uvs_start;
	uint32_t uvs2_start;
	uint32_t normals_start;
	uint32_t tangents_start;
	uint32_t bitangents_start;
	uint32_t colors_start;
};

// Mesh, vertex data
// From: raylib.h
typedef struct mesh {
	index_t vertexCount;       // Number of vertices stored in arrays
	index_t triangleCount;     // Number of triangles stored (indexed or not)

	// Vertex attributes data
	vec3f* positions;          // Vertex position (shader-location = 0)
	vec2f* uvs;          // Vertex texture coordinates (shader-location = 1)
	vec2f* uvs2;         // Vertex texture second coordinates (shader-location = 6)
	vec3f* normals;            // Vertex normals (shader-location = 2)
	vec3f* tangents;           // Vertex tangents (shader-location = 4)
	vec3f* bitangents;         // Vertex bitangents (shader-location = 5)
	color* colors;             // Vertex colors (shader-location = 3)
	index_t* indices;          // Vertex indices (in case vertex data comes indexed)

	// Animation vertex data
	// vec3f* anim_vertices;      // Animated vertex positions (after bones transformations)
	// vec3f* anim_normals;       // Animated normals (after bones transformations)
	vec4u* bone_ids;			// Vertex bone ids, max 255 bone ids, up to 4 bones influence by vertex (skinning)
	vec4f* bone_weights;		// Vertex bone weight, up to 4 bones influence by vertex (skinning)
	bool heap_allocated // Wether or not this mesh is stored on the heap and should be automatically cleaned up
#ifdef WAYLIB_ENABLE_DEFAULT_PARAMETERS
		= false
#endif
	;

	WAYLIB_C_OR_CPP_TYPE(WGPUBuffer, wgpu::Buffer) buffer; // Pointer to the data on the gpu
	WAYLIB_C_OR_CPP_TYPE(WGPUBuffer, wgpu::Buffer) indexBuffer; // Pointer to the index data on the gpu
	// WAYLIB_C_OR_CPP_TYPE(WGPUBuffer, wgpu::Buffer) instanceBuffer; // Pointer to the per-instance data on the gpu

#ifdef WAYLIB_ENABLE_CLASSES
	inline mesh_metadata get_metadata() {
		mesh_metadata meta = {
			.is_indexed = (bool)indexBuffer,
			.vertex_count = vertexCount,
			.triangle_count = triangleCount,

			.position_start = 0,
			.uvs_start = static_cast<uint32_t>(meta.position_start + vertexCount * sizeof(vec3f)),
			.uvs2_start = static_cast<uint32_t>(meta.uvs_start + vertexCount * sizeof(vec2f)),
			.normals_start = static_cast<uint32_t>(meta.uvs2_start + vertexCount * sizeof(vec2f)),
			.tangents_start = static_cast<uint32_t>(meta.normals_start + vertexCount * sizeof(vec3f)),
			.bitangents_start = static_cast<uint32_t>(meta.tangents_start + vertexCount * sizeof(vec3f)),
			.colors_start = static_cast<uint32_t>(meta.bitangents_start + vertexCount * sizeof(vec3f))
		};
		return meta;
	}
#endif
} mesh;
#ifdef __cplusplus
	template struct optional<mesh>;

	constexpr static size_t WAYLIB_MESH_VERTEX_SIZE = (sizeof(vec3f) * 4 + sizeof(vec2f) * 2 + sizeof(color));
#else
	#define WAYLIB_MESH_VERTEX_SIZE (sizeof(vec3f) * 4 + sizeof(vec2f) * 2 + sizeof(color))
#endif

// Bone, skeletal animation bone
// From: raylib.h
typedef struct bone_info {
	char name[32];          // Bone name
	index_t parent;         // Bone parent
	mat4x4f_ bind_pose;     // Bones base transformation (pose) // TODO: Why was this in model?
} bone_info;

// Model, meshes, materials and animation data
// From: raylib.h
typedef struct model {
	mat4x4f_ transform;			// Local transform matrix

	index_t mesh_count;			// Number of meshes
	mesh* meshes;				// Meshes array
	index_t material_count;		// Number of materials
	material* materials;		// Materials array
	index_t* mesh_materials;	// Mesh to material mapping (when null and )

	// Animation data
	index_t bone_count;			// Number of bones
	bone_info* bones;			// Bones information (skeleton)

	bool heap_allocated // Wether or not this model is stored on the heap and should be automatically cleaned up
#ifdef WAYLIB_ENABLE_DEFAULT_PARAMETERS
		= false
#endif
	;

#ifdef WAYLIB_ENABLE_CLASSES
	index_t get_material_index_for_mesh(index_t meshID) {
		if(mesh_materials == nullptr && mesh_count == material_count)
			return meshID;
		return mesh_materials[meshID];
	}

	inline std::span<mesh> get_meshes() { return {meshes, mesh_count}; }
	inline std::span<material> get_materials() { return {materials, material_count}; }
#endif
} model;
#ifdef __cplusplus
	template struct optional<model>;
#endif

typedef struct model_instance_data {
	mat4x4f_ transform;
	mat4x4f_ inverse_transform;
	color tint
#ifdef WAYLIB_ENABLE_DEFAULT_PARAMETERS
		= {1, 1, 1, 1}
#endif
	;
} model_instance_data;
#ifdef __cplusplus
	template struct optional<model_instance_data>;
#endif

#ifndef WAYLIB_NO_CAMERAS
	#ifdef WAYLIB_ENABLE_DEFAULT_PARAMETERS
		#define WAYLIB_CAMERA_3D_MEMBERS\
			vec3f position; /* Camera position */\
			float padding1;\
			vec3f target; /* Camera target it looks-at*/\
			float padding2;\
			vec3f up = {0, 1, 0}; /* Camera up vector (rotation over its axis)*/\
			float field_of_view = 90; /* Camera field-of-view aperture in Y (degrees) in perspective, used as near plane width in orthographic */\
			float near_clip_distance = .01, far_clip_distance = 1000; /* Distances before objects are culled */\
			bool32 orthographic = false; /* True if the camera should use orthographic projection */

		#define WAYLIB_CAMERA_2D_MEMBERS\
			vec3f offset; /* Camera offset (displacement from target)*/\
			float padding1;\
			vec3f target; /* Camera target (rotation and zoom origin)*/\
			degree rotation = 0; /* Camera rotation in degrees*/\
			float near_clip_distance = .01, far_clip_distance = 1000; /* Distances before objects are culled */\
			float zoom = 1; /* Camera zoom (scaling), should be 1.0f by default*/\
			bool32 pixel_perfect = false; /*When false screen is in range [0, 1] when true screen is in range [0, #pixels]*/
	#else
		#define WAYLIB_CAMERA_3D_MEMBERS\
			vec3f position; /* Camera position */\
			float padding1;\
			vec3f target; /* Camera target it looks-at*/\
			float padding2;\
			vec3f up; /* Camera up vector (rotation over its axis)*/\
			float field_of_view; /* Camera field-of-view aperture in Y (degrees) in perspective, used as near plane width in orthographic */\
			float near_clip_distance, far_clip_distance; /* Distances before objects are culled */\
			bool32 orthographic; /* True if the camera should use orthographic projection */\

		#define WAYLIB_CAMERA_2D_MEMBERS\
			vec3f offset; /* Camera offset (displacement from target)*/\
			float padding1;\
			vec3f target; /* Camera target (rotation and zoom origin)*/\
			degree rotation; /* Camera rotation in degrees*/\
			float near_clip_distance, far_clip_distance; /* Distances before objects are culled */\
			float zoom; /* Camera zoom (scaling), should be 1.0f by default*/\
			bool32 pixel_perfect; /*When false screen is in range [0, 1] when true screen is in range [0, #pixels]*/
	#endif // WAYLIB_ENABLE_DEFAULT_PARAMETERS

	#define WAYLIB_CAMERA_UPLOAD_DATA_MEMBERS\
		bool32 is_3D;\
		char padding1[12];\
		camera3D settings3D;\
		char padding2[4];\
		camera2D settings2D;\
		vec2i window_dimensions;\
		char padding4[8];\
		mat4x4f_ view_matrix;\
		mat4x4f_ projection_matrix;

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

	typedef struct camera_upload_data {
		WAYLIB_CAMERA_UPLOAD_DATA_MEMBERS
	} camera_upload_data;
#else
	struct camera3D;
	struct camera2D;
	struct camera_upload_data;
#endif

typedef camera3D camera;	// By default a camera is a 3D camera
#endif // WAYLIB_NO_CAMERAS

#ifndef WAYLIB_NO_LIGHTS
	WAYLIB_ENUM light_type {
		C_PREPEND(LIGHT_TYPE_, Directional) = 0,
		C_PREPEND(LIGHT_TYPE_, Point),
		C_PREPEND(LIGHT_TYPE_, Spot),
		// Count of how many slots are supported
		C_PREPEND(LIGHT_TYPE_, Max),
		C_PREPEND(LIGHT_TYPE_, FORCE32) = 0x7fffffff
	};

	#define WAYLIB_LIGHT_MEMBERS\
		light_type type;\
		vec3f padding1;\
		vec3f position;\
		float padding2;\
		vec3f direction;\
		float padding3;\
	\
		vec4f color;\
		float intensity;\
	\
		float cutoff_start_angle;\
		float cutoff_end_angle;\
	\
		float constant; /*There has to be a better way of storing attenuation...*/\
		float linear;\
		float quadratic;\
		vec2f padding4;

	#ifndef __cplusplus
		typedef struct light {
			WAYLIB_LIGHT_MEMBERS
		} light;
	#else
		struct light;
	#endif
#endif // WAYLIB_NO_LIGHTS

// Struct holding all of the state needed by webgpu functions
typedef struct waylib_state {
	WAYLIB_C_OR_CPP_TYPE(WGPUInstance, wgpu::Instance) instance;
	WAYLIB_C_OR_CPP_TYPE(WGPUAdapter, wgpu::Adapter) adapter;
	WAYLIB_C_OR_CPP_TYPE(WGPUDevice, wgpu::Device) device;
	WAYLIB_C_OR_CPP_TYPE(WGPUSurface, wgpu::Surface) surface;
} waylib_state;
#ifdef __cplusplus
	template struct optional<waylib_state>;
#endif

struct wgpu_frame_finalizers;

typedef struct frame_state {
	waylib_state state;
	WAYLIB_C_OR_CPP_TYPE(WGPUTextureView, wgpu::TextureView) color_target;
	WAYLIB_C_OR_CPP_TYPE(WGPUTexture, wgpu::Texture) depth_texture;
	WAYLIB_C_OR_CPP_TYPE(WGPUTextureView, wgpu::TextureView) depth_target;
	WAYLIB_C_OR_CPP_TYPE(WGPUCommandEncoder, wgpu::CommandEncoder) render_encoder;
	WAYLIB_C_OR_CPP_TYPE(WGPUCommandEncoder, wgpu::CommandEncoder) encoder;
	WAYLIB_C_OR_CPP_TYPE(WGPURenderPassEncoder, wgpu::RenderPassEncoder) render_pass;
	wgpu_frame_finalizers* finalizers;
} frame_state;
#ifdef __cplusplus
	template struct optional<frame_state>;
#endif

typedef struct frame_time {
	float since_start;
	float delta;
	float average_delta;
} frame_time;
#ifdef __cplusplus
	template struct optional<frame_time>;
#endif


WAYLIB_NULLABLE(const char*) get_error_message();
void set_error_message_raw(WAYLIB_NULLABLE(const char*) message);
void clear_error_message();

color convert_to_color(const color8 c);
color8 convert_to_color8(const color c);

WAYLIB_NULLABLE(void*) image_get_frame_and_size(
	size_t* out_size,
	image* image,
	size_t frame,
	size_t mip_level
#ifdef WAYLIB_ENABLE_DEFAULT_PARAMETERS
		= 0
#endif
);

WAYLIB_NULLABLE(void*) image_get_frame(
	image* image,
	size_t frame,
	size_t mip_level
#ifdef WAYLIB_ENABLE_DEFAULT_PARAMETERS
		= 0
#endif
);

WAYLIB_OPTIONAL(image) merge_images(
	image* images,
	size_t images_size,
	bool free_incoming
#ifdef WAYLIB_ENABLE_DEFAULT_PARAMETERS
		= true
#endif
);

#ifdef __cplusplus
} // End extern "C"
#endif

#endif // WAYLIB_COMMON_IS_AVAILABLE