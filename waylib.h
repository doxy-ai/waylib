#ifndef WAYLIB_IS_AVAILABLE
#define WAYLIB_IS_AVAILABLE
#include "common.h"

#ifdef __EMSCRIPTEN__
	namespace detail_ {
		template<typename F>
		auto closure_to_function_pointer(F _f) {
			static F f = _f;
			return +[]{ f(); };
		}
	}

	#define WAYLIB_MAIN_LOOP_CONTINUE return
	#define WAYLIB_MAIN_LOOP_BREAK emscripten_cancel_main_loop()
	#ifdef WAYLIB_NAMESPACE_NAME
		#define WAYLIB_MAIN_LOOP(continue_expression, body)\
			auto callback = [&]() {\
				if(!(continue_expression)) WAYLIB_MAIN_LOOP_BREAK;\
				body\
			};\
			emscripten_set_main_loop(WAYLIB_NAMESPACE_NAME::detail_::closure_to_function_pointer(callback), 0, true);
	#else // !WAYLIB_NAMESPACE_NAME
		#define WAYLIB_MAIN_LOOP(continue_expression, body)\
			auto callback = [&]() {\
				if(!(continue_expression)) WAYLIB_MAIN_LOOP_BREAK;\
				body\
			};\
			emscripten_set_main_loop(detail_::closure_to_function_pointer(callback), 0, true);
	#endif // WAYLIB_NAMESPACE_NAME
#else // !__EMSCRIPTEN__
	#define WAYLIB_MAIN_LOOP(continue_expression, body)\
		while(continue_expression) {\
			body\
		}
	#define WAYLIB_MAIN_LOOP_CONTINUE continue
	#define WAYLIB_MAIN_LOOP_BREAK break
#endif // __EMSCRIPTEN__

#ifdef __cplusplus
extern "C" {
#endif

typedef struct surface_configuration {
	WGPUPresentMode presentation_mode
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

typedef struct material_configuration {
	WAYLIB_OPTIONAL(WGPUCompareFunction) depth_function
#ifdef WAYLIB_ENABLE_DEFAULT_PARAMETERS
		= wgpu::CompareFunction::Less // Disables writing depth if not provided
#endif
	; // TODO: Add stencil support
} material_configuration;

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
	; bool calculate_tangents_from_normals // Useful for normal mapping
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

struct texture_config {
	WGPUFilterMode color_filter
#ifdef WAYLIB_ENABLE_DEFAULT_PARAMETERS
		= wgpu::FilterMode::Linear
#endif
	; WGPUMipmapFilterMode mipmap_filter
#ifdef WAYLIB_ENABLE_DEFAULT_PARAMETERS
		= wgpu::MipmapFilterMode::Linear
#endif
	; WGPUAddressMode address_mode
#ifdef WAYLIB_ENABLE_DEFAULT_PARAMETERS
		= wgpu::AddressMode::Repeat
#endif
	; bool cubemap
#ifdef WAYLIB_ENABLE_DEFAULT_PARAMETERS
		= false
#endif
	 

	// Note: The following settings are overwritten when creating from an image!
	; bool float_data
#ifdef WAYLIB_ENABLE_DEFAULT_PARAMETERS
		= false
#endif
	; size_t mipmaps
#ifdef WAYLIB_ENABLE_DEFAULT_PARAMETERS
		= 1
#endif
	; size_t frames
#ifdef WAYLIB_ENABLE_DEFAULT_PARAMETERS
		= 1
#endif
	;
};




void time_calculations(frame_time* frame_time);

bool open_url(const char* url);

void upload_utility_data(
	wgpu_frame_state* frame, 
	WAYLIB_OPTIONAL(camera_upload_data*) data, 
	light* lights, 
	size_t light_count, 
	WAYLIB_OPTIONAL(frame_time) frame_time
);

wgpu_state create_default_state_from_instance(
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

wgpu_state create_default_state(
	WGPUSurface surface
#ifdef WAYLIB_ENABLE_DEFAULT_PARAMETERS
		= nullptr
#endif
	, bool prefer_low_power
#ifdef WAYLIB_ENABLE_DEFAULT_PARAMETERS
		= false
#endif
);

void release_wgpu_state(
	wgpu_state state,
	bool release_adapter
#ifdef WAYLIB_ENABLE_DEFAULT_PARAMETERS
		= true
#endif
	, bool release_instance
#ifdef WAYLIB_ENABLE_DEFAULT_PARAMETERS
		= true
#endif
);

bool configure_surface(
	wgpu_state state,
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

shader_preprocessor* preprocessor_initialize_virtual_filesystem(
	shader_preprocessor* processor,
	wgpu_state state,
	preprocess_shader_config config
#ifdef WAYLIB_ENABLE_DEFAULT_PARAMETERS
		= {}
#endif
);

shader_preprocessor* preprocessor_add_define(
	shader_preprocessor* processor,
	const char* name,
	const char* value
);

bool preprocessor_add_search_path(
	shader_preprocessor* processor,
	const char* _path
);

shader_preprocessor* preprocessor_initialize_platform_defines(
	shader_preprocessor* processor,
	wgpu_state state
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
	wgpu_state state,
	const char* wgsl_source_code,
	create_shader_configuration config
#ifdef WAYLIB_ENABLE_DEFAULT_PARAMETERS
		= {}
#endif
);

void release_shader(
	shader* shader
);

WAYLIB_OPTIONAL(wgpu_frame_state) begin_drawing_render_texture(
	wgpu_state state,
	WGPUTextureView render_texture,
	vec2i render_texture_dimensions,
	WAYLIB_OPTIONAL(color) clear_color
#ifdef WAYLIB_ENABLE_DEFAULT_PARAMETERS
		= {}
#endif
);

WAYLIB_OPTIONAL(wgpu_frame_state) begin_drawing(
	wgpu_state state,
	WAYLIB_OPTIONAL(color) clear_color
#ifdef WAYLIB_ENABLE_DEFAULT_PARAMETERS
		= {}
#endif
);

void end_drawing(
	wgpu_frame_state* frame
);

void present_frame(
	wgpu_frame_state* frame
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
	wgpu_frame_state* frame,
	camera3D* camera,
	vec2i window_dimensions,
	light* lights
#ifdef WAYLIB_ENABLE_DEFAULT_PARAMETERS
		= nullptr
#endif
	, size_t light_count
#ifdef WAYLIB_ENABLE_DEFAULT_PARAMETERS
		= 0
#endif
	, WAYLIB_OPTIONAL(frame_time) frame_time
#ifdef WAYLIB_ENABLE_DEFAULT_PARAMETERS
		= {}
#endif
);

void begin_camera_mode2D(
	wgpu_frame_state* frame,
	camera2D* camera,
	vec2i window_dimensions,
	light* lights
#ifdef WAYLIB_ENABLE_DEFAULT_PARAMETERS
		= nullptr
#endif
	, size_t light_count
#ifdef WAYLIB_ENABLE_DEFAULT_PARAMETERS
		= 0
#endif
	, WAYLIB_OPTIONAL(frame_time) frame_time
#ifdef WAYLIB_ENABLE_DEFAULT_PARAMETERS
		= {}
#endif
);

void begin_camera_mode_identity(
	wgpu_frame_state* frame,
	vec2i window_dimensions,
	light* lights
#ifdef WAYLIB_ENABLE_DEFAULT_PARAMETERS
		= nullptr
#endif
	, size_t light_count
#ifdef WAYLIB_ENABLE_DEFAULT_PARAMETERS
		= 0
#endif
	, WAYLIB_OPTIONAL(frame_time) frame_time
#ifdef WAYLIB_ENABLE_DEFAULT_PARAMETERS
		= {}
#endif
);

void end_camera_mode(
	wgpu_frame_state* frame
);
#endif // WAYLIB_NO_CAMERAS

void mesh_upload(
	wgpu_state state,
	mesh* mesh
);

void release_mesh(
	mesh* mesh
);

void material_upload(
	wgpu_state state,
	material* material,
	material_configuration config
#ifdef WAYLIB_ENABLE_DEFAULT_PARAMETERS
		= {}
#endif
);

material create_material(
	wgpu_state state,
	shader* shaders,
	size_t shader_count,
	material_configuration config
#ifdef WAYLIB_ENABLE_DEFAULT_PARAMETERS
		= {}
#endif
);

void release_material(
	material* material,
	bool release_textures
#ifdef WAYLIB_ENABLE_DEFAULT_PARAMETERS
		= true
#endif
	, bool release_shaders
#ifdef WAYLIB_ENABLE_DEFAULT_PARAMETERS
		= true
#endif
);

model_process_configuration default_model_process_configuration();

void model_upload(
	wgpu_state state,
	model* model
);

void release_model(
	model* model,
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

void model_draw_instanced(
	wgpu_frame_state* frame,
	model* model,
	model_instance_data* instances,
	size_t instance_count
);

void model_draw(
	wgpu_frame_state* frame,
	model* model
);

WAYLIB_OPTIONAL(texture) create_texture(
	wgpu_state state, 
	vec2i dimensions, 
	texture_config config
#ifdef WAYLIB_ENABLE_DEFAULT_PARAMETERS
		= {}
#endif
);

WAYLIB_OPTIONAL(texture) create_texture_from_image(
	wgpu_state state, 
	image* image, 
	texture_config config
#ifdef WAYLIB_ENABLE_DEFAULT_PARAMETERS
		= {}
#endif
);

#ifdef __cplusplus
	template struct optional<const texture*>;
#endif
WAYLIB_OPTIONAL(const texture*) get_default_texture(wgpu_state state);
WAYLIB_OPTIONAL(const texture*) get_default_cube_texture(wgpu_state state);

void release_image(
	image* image
);

void release_texture(
	texture* texture
);

#ifdef __cplusplus
} // End extern "C"
#endif

#endif // WAYLIB_IS_AVAILABLE