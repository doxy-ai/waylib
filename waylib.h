#pragma once
#include "config.h"

#ifdef __cplusplus
extern "C" {
#endif

// Define mathematical types used... in C++ they are provided by glm so we just prototype them here!
typedef struct mat4x4f_ {
	float a0, a1, a2, a3;
	float b0, b1, b2, b3;
	float c0, c1, c2, c3;
	float d0, d1, d2, d3;
} mat4x4f_;

#ifndef __cplusplus
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
struct vec2f;
struct vec3f;
struct vec4f;
struct mat4x4f;
#endif

typedef struct {
	unsigned char r, g, b, a;
} color8bit;

typedef struct {
	float r, g, b, a;
} color32bit;


// Shader 
typedef struct shader {
	
} shader;

// Mesh, vertex data
// From: raylib.h
typedef struct mesh {
	index_t vertexCount;       // Number of vertices stored in arrays
	index_t triangleCount;     // Number of triangles stored (indexed or not)

	// Vertex attributes data
	vec3f* positions;         // Vertex position (shader-location = 0)
	vec2f* texcoords;         // Vertex texture coordinates (shader-location = 1)
	vec2f* texcoords2;        // Vertex texture second coordinates (shader-location = 5)
	vec3f* normals;           // Vertex normals (shader-location = 2)
	vec4f* tangents;          // Vertex tangents (shader-location = 4)
	color8bit* colors;        // Vertex colors (shader-location = 3)
	index_t* indices;         // Vertex indices (in case vertex data comes indexed)

	// Animation vertex data
	vec3f* anim_vertices;     // Animated vertex positions (after bones transformations)
	vec3f* anim_normals;      // Animated normals (after bones transformations)
	unsigned char* bone_ids;  // Vertex bone ids, max 255 bone ids, up to 4 bones influence by vertex (skinning)
	vec4f* bone_weights;      // Vertex bone weight, up to 4 bones influence by vertex (skinning)
} mesh;

// 
// From: raylib.h
typedef struct material {
	/*What should go here?*/
} material;

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
WAYLIB_ENUM image_format {
	C_PREPEND(IMAGE_FORMAT_, RGBA8) = 0,
	// C_PREPEND(IMAGE_FORMAT_, RGB8),
	// C_PREPEND(IMAGE_FORMAT_, Gray8),
	C_PREPEND(IMAGE_FORMAT_, RGBAF32),
	// C_PREPEND(IMAGE_FORMAT_, RGBF32),
	// C_PREPEND(IMAGE_FORMAT_, Gray32),
};

// Image, pixel data stored in CPU memory (RAM)
// From: raylib.h
typedef struct image {
	void* data;             // Image raw data
	int width;              // Image base width
	int height;             // Image base height
	int mipmaps;            // Mipmap levels, 1 by default
	image_format format;    // Data format (PixelFormat type)
} image;

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

#ifdef __cplusplus
} // End extern "C"
#endif
