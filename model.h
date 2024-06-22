#pragma once
#include "texture.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct model_process_optimization_configuration {
	; bool vertex_cache_locality
#ifdef WAYLIB_ENABLE_DEFAULT_PARAMETERS
		= false
#endif
	; bool find_instances // Finds all duplicate meshes
#ifdef WAYLIB_ENABLE_DEFAULT_PARAMETERS
		= false
#endif
	; bool meshes
#ifdef WAYLIB_ENABLE_DEFAULT_PARAMETERS
		= false
#endif
	;
} model_process_optimization_configuration;

typedef struct model_process_configuration {
	; bool triangulate // If not triangulated the import process will fail...
		// but can be set to false to save time if model is already triangulated
#ifdef WAYLIB_ENABLE_DEFAULT_PARAMETERS
		= true
#endif
	; bool join_identical_vertecies
#ifdef WAYLIB_ENABLE_DEFAULT_PARAMETERS
		= true
#endif
	; bool generate_normals_if_not_present
#ifdef WAYLIB_ENABLE_DEFAULT_PARAMETERS
		= false
#endif
	; bool smooth_generated_normals
#ifdef WAYLIB_ENABLE_DEFAULT_PARAMETERS
		= true
#endif
	; bool calculate_tangets_from_normals // Useful for normal mapping
#ifdef WAYLIB_ENABLE_DEFAULT_PARAMETERS
		= true
#endif
	; bool remove_normals
#ifdef WAYLIB_ENABLE_DEFAULT_PARAMETERS
		= false
#endif
	; bool split_large_meshes
#ifdef WAYLIB_ENABLE_DEFAULT_PARAMETERS
		= true
#endif
	; bool split_large_bone_count
#ifdef WAYLIB_ENABLE_DEFAULT_PARAMETERS
		= false
#endif
	; bool fix_infacing_normals
#ifdef WAYLIB_ENABLE_DEFAULT_PARAMETERS
		= false
#endif
	; bool fix_invalid_data
#ifdef WAYLIB_ENABLE_DEFAULT_PARAMETERS
		= false
#endif
	; bool fix_invalid_texture_paths
#ifdef WAYLIB_ENABLE_DEFAULT_PARAMETERS
		= true
#endif

	; bool validate_model
#ifdef WAYLIB_ENABLE_DEFAULT_PARAMETERS
		= false
#endif

	; model_process_optimization_configuration optimize;
} model_process_configuration;




model_process_configuration default_model_process_configuration();

WAYLIB_OPTIONAL(model) load_model(
	const char * file_path,
	model_process_configuration config
#ifdef WAYLIB_ENABLE_DEFAULT_PARAMETERS
		= default_model_process_configuration()
#endif
);

WAYLIB_OPTIONAL(model) load_model_from_memory(
	const unsigned char* data, size_t size,
	model_process_configuration config
#ifdef WAYLIB_ENABLE_DEFAULT_PARAMETERS
		= default_model_process_configuration()
#endif
);

#ifdef __cplusplus
} // End extern "C"
#endif
