#ifndef WAYLIB_COMMON_IS_AVAILABLE
#define WAYLIB_COMMON_IS_AVAILABLE
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
	bool heap_allocated; // Wether or not this image is stored on the heap and should be automatically cleaned up
} image;

WAYLIB_ENUM texture_slot {
	C_PREPEND(TEXTURE_SLOT_, Color) = 0,
	C_PREPEND(TEXTURE_SLOT_, Height),
	C_PREPEND(TEXTURE_SLOT_, Normal),
	C_PREPEND(TEXTURE_SLOT_, PackedMap),
	C_PREPEND(TEXTURE_SLOT_, Roughness),
	C_PREPEND(TEXTURE_SLOT_, Metalic),
	C_PREPEND(TEXTURE_SLOT_, AmbientOcclusion),
	C_PREPEND(TEXTURE_SLOT_, Extra),
	// Count of how many slots are supported
	C_PREPEND(TEXTURE_SLOT_, Max),
};
#define WAYLIB_TEXTURE_SLOT_COUNT WAYLIB_C_OR_CPP_TYPE(TEXTURE_SLOT_Max, (index_t)texture_slot::Max)

typedef struct texture {
	image* cpu_data;
	WAYLIB_C_OR_CPP_TYPE(WGPUTexture, wgpu::Texture) gpu_data;
	WAYLIB_C_OR_CPP_TYPE(WGPUTextureView, wgpu::TextureView) view; // TODO: Needed?
	WAYLIB_OPTIONAL(WAYLIB_C_OR_CPP_TYPE(WGPUSampler, wgpu::Sampler)) sampler;
	bool heap_allocated;// Wether or not this texture is stored on the heap and should be automatically cleaned up
} texture;

// Shader
typedef struct shader {
	// unsigned char* source;
	// size_t source_size;

	const char* compute_entry_point;
	const char* vertex_entry_point;
	const char* fragment_entry_point;
	WAYLIB_C_OR_CPP_TYPE(WGPUShaderModule, wgpu::ShaderModule) module;
	bool heap_allocated;// Wether or not this shader is stored on the heap and should be automatically cleaned up
} shader;

// Material
typedef struct material {
	index_t shaderCount;
	shader* shaders;
	WAYLIB_OPTIONAL(texture) textures[WAYLIB_TEXTURE_SLOT_COUNT];
	WAYLIB_C_OR_CPP_TYPE(WGPURenderPipeline, wgpu::RenderPipeline) pipeline;
	bool heap_allocated;// Wether or not this material is stored on the heap and should be automatically cleaned up

#ifdef WAYLIB_ENABLE_CLASSES
	inline std::span<shader> get_shaders() { return {shaders, shaderCount}; }
	inline std::span<WAYLIB_OPTIONAL(texture), WAYLIB_TEXTURE_SLOT_COUNT> get_textures() { return {textures}; }
#endif
} material;

// Mesh, vertex data
// From: raylib.h
typedef struct mesh {
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
	bool heap_allocated;// Wether or not this mesh is stored on the heap and should be automatically cleaned up

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
	mat4x4f_ transform;			// Local transform matrix

	index_t mesh_count;			// Number of meshes
	mesh* meshes;				// Meshes array
	index_t material_count;		// Number of materials
	material* materials;		// Materials array
	index_t* mesh_materials;	// Mesh to material mapping (when null and )

	// Animation data
	index_t bone_count;			// Number of bones
	bone_info* bones;			// Bones information (skeleton)

	bool heap_allocated;	// Wether or not this model is stored on the heap and should be automatically cleaned up

#ifdef WAYLIB_ENABLE_CLASSES
	inline mat4x4f& get_transform() { return *(mat4x4f*)&transform; }

	index_t get_material_index_for_mesh(index_t meshID) {
		if(mesh_materials == nullptr && mesh_count == material_count)
			return meshID;
		return mesh_materials[meshID];
	}
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
typedef struct wgpu_state {
	WAYLIB_C_OR_CPP_TYPE(WGPUDevice, wgpu::Device) device;
	WAYLIB_C_OR_CPP_TYPE(WGPUSurface, wgpu::Surface) surface;
} wgpu_state;

typedef struct wgpu_frame_state {
	wgpu_state state;
	WAYLIB_C_OR_CPP_TYPE(WGPUTextureView, wgpu::TextureView) color_target;
	WAYLIB_C_OR_CPP_TYPE(WGPUTexture, wgpu::Texture) depth_texture;
	WAYLIB_C_OR_CPP_TYPE(WGPUTextureView, wgpu::TextureView) depth_target;
	WAYLIB_C_OR_CPP_TYPE(WGPUCommandEncoder, wgpu::CommandEncoder) encoder;
	WAYLIB_C_OR_CPP_TYPE(WGPURenderPassEncoder, wgpu::RenderPassEncoder) render_pass;
	mat4x4f_ current_VP;

#ifdef WAYLIB_ENABLE_CLASSES
	inline mat4x4f& get_current_view_projection_matrix() { return *(mat4x4f*)&current_VP; }
#endif
} wgpu_frame_state;

typedef struct time {
	float since_start;
	float delta;
	float average_delta;
} time;


const char* get_error_message();
void set_error_message_raw(const char* message);
void clear_error_message();

#ifdef __cplusplus
} // End extern "C"
#endif

#endif // WAYLIB_COMMON_IS_AVAILABLE