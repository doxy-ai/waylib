#pragma once
#include "texture.hpp"

#ifdef WAYLIB_NAMESPACE_NAME
namespace WAYLIB_NAMESPACE_NAME {
#endif

#include "model.h"

void mesh_upload(
	webgpu_state state,
	mesh& mesh
);

void material_upload(
	webgpu_state state,
	material& material,
	material_configuration config
#ifdef WAYLIB_ENABLE_DEFAULT_PARAMETERS
		= {}
#endif
);

material create_material(
	webgpu_state state,
	std::span<shader> shaders,
	material_configuration config
#ifdef WAYLIB_ENABLE_DEFAULT_PARAMETERS
		= {}
#endif
);
material create_material(
	webgpu_state state,
	shader& shader,
	material_configuration config
#ifdef WAYLIB_ENABLE_DEFAULT_PARAMETERS
		= {}
#endif
);

void model_upload(
	webgpu_state state,
	model& model
);

WAYLIB_OPTIONAL(model) load_model_from_memory(
	webgpu_state state,
	std::span<std::byte> data,
	model_process_configuration config
#ifdef WAYLIB_ENABLE_DEFAULT_PARAMETERS
		= default_model_process_configuration()
#endif
);

void model_draw_instanced(
	webgpu_frame_state& frame,
	model& model,
	std::span<model_instance_data> instances
);

void model_draw(
	webgpu_frame_state& frame,
	model& model
);

#ifdef WAYLIB_NAMESPACE_NAME
}
#endif