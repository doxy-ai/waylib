#ifndef STYLIZER_PBR_IS_AVAILABLE
#define STYLIZER_PBR_IS_AVAILABLE

#include "stylizer/core/core.h"

#ifdef __cplusplus
typedef struct pbr_geometry_bufferC: public geometry_bufferC {
#else
typedef struct pbr_geometry_bufferC {
	geometry_bufferC base; // C style inheritance
#endif
	STYLIZER_PREFIXED_C_CPP_TYPE(texture, textureC) packed; // metalness (r), roughness (g), ambient occlusion (b), shadow maps (a)
} STYLIZER_PREFIXED(pbr_geometry_bufferC);


enum pbr_texture_slot {
	C_PREPEND(TEXTURE_SLOT_, None) = 0,
	C_PREPEND(TEXTURE_SLOT_, Albedo) = (1 << 0),
	C_PREPEND(TEXTURE_SLOT_, Emission) = (1 << 1),
	C_PREPEND(TEXTURE_SLOT_, Normal) = (1 << 2),
	C_PREPEND(TEXTURE_SLOT_, Packed) = (1 << 3),
	C_PREPEND(TEXTURE_SLOT_, Roughness) = (1 << 4),
	C_PREPEND(TEXTURE_SLOT_, Metalness) = (1 << 5),
	C_PREPEND(TEXTURE_SLOT_, Ambient_Occlusion) = (1 << 6),
	C_PREPEND(TEXTURE_SLOT_, Environment) = (1 << 7),

	C_PREPEND(TEXTURE_SLOT_, All) = (1 << 0) & (1 << 1) & (1 << 2) & (1 << 3) & (1 << 4) & (1 << 5) & (1 << 6) & (1 << 7),
};

#ifdef __cplusplus
typedef struct pbr_materialC: public materialC {
#else
typedef struct pbr_materialC {
	materialC base; // C style inheritance
#endif

	STYLIZER_MANAGEABLE(STYLIZER_NULLABLE(STYLIZER_PREFIXED_C_CPP_TYPE(texture, textureC)*)) albedo_texture;
	STYLIZER_MANAGEABLE(STYLIZER_NULLABLE(STYLIZER_PREFIXED_C_CPP_TYPE(texture, textureC)*)) emission_texture;
	// STYLIZER_MANAGEABLE(STYLIZER_NULLABLE(STYLIZER_PREFIXED_C_CPP_TYPE(texture, textureC)*)) height;
	STYLIZER_MANAGEABLE(STYLIZER_NULLABLE(STYLIZER_PREFIXED_C_CPP_TYPE(texture, textureC)*)) normal_texture;
	STYLIZER_MANAGEABLE(STYLIZER_NULLABLE(STYLIZER_PREFIXED_C_CPP_TYPE(texture, textureC)*)) packed_texture;
	STYLIZER_MANAGEABLE(STYLIZER_NULLABLE(STYLIZER_PREFIXED_C_CPP_TYPE(texture, textureC)*)) roughness_texture;
	STYLIZER_MANAGEABLE(STYLIZER_NULLABLE(STYLIZER_PREFIXED_C_CPP_TYPE(texture, textureC)*)) metalness_texture;
	STYLIZER_MANAGEABLE(STYLIZER_NULLABLE(STYLIZER_PREFIXED_C_CPP_TYPE(texture, textureC)*)) ambient_occlusion_texture;

	STYLIZER_MANAGEABLE(STYLIZER_NULLABLE(STYLIZER_PREFIXED_C_CPP_TYPE(texture, cube_texture)*)) environment_texture;


	STYLIZER_C_OR_CPP_TYPE(WGPUBuffer, wgpu::Buffer) settings_buffer; // Pointer to the following data on the gpu
	uint32_t material_id;

	float _pad0[3]; // Padding so data will line up on gpu
	STYLIZER_PREFIXED(colorC) albedo;
	STYLIZER_PREFIXED(colorC) emission;
	float metalness;
	float roughness;

// 	float height_displacement_factor // NOTE: Unused by default pipeline
// #ifdef WAYLIB_ENABLE_DEFAULT_PARAMETERS
// 		= 0
// #endif
	; float normal_strength
#ifdef WAYLIB_ENABLE_DEFAULT_PARAMETERS
		= 1
#endif
	; pbr_texture_slot used_textures; // NOTE: Not intended to be edited manually, will be overwritten when settings updated
} STYLIZER_PREFIXED_C_CPP_TYPE(pbr_material, pbr_materialC);

typedef struct pbr_geometry_buffer_config {
	geometry_buffer_config base;

	; WGPUTextureFormat packed_format
#ifdef STYLIZER_ENABLE_DEFAULT_PARAMETERS
		= wgpu::TextureFormat::RGBA8Unorm
#endif
	;
} STYLIZER_PREFIXED(pbr_geometry_buffer_config);

#endif // STYLIZER_PBR_IS_AVAILABLE