#include "pbr.hpp"

#include <battery/embed.hpp>

STYLIZER_BEGIN_NAMESPACE
namespace pbr {

	shader_preprocessor& initialize_shader_preprocessor_virtual_filesystem(shader_preprocessor& p, const shader_preprocessor::config &config /* = {} */) {
		p.process_from_memory_and_cache(b::embed<"shaders/pbr_gbuffer_data.wgsl">().str(), "stylizer/pbr/gbuffer_data", config);
		p.process_from_memory_and_cache(b::embed<"shaders/pbr_gbuffer.wgsl">().str(), "stylizer/pbr/gbuffer", config);
		p.process_from_memory_and_cache(b::embed<"shaders/pbr_material_data.wgsl">().str(), "stylizer/pbr/material_data", config);
		p.process_from_memory_and_cache(b::embed<"shaders/pbr_material.wgsl">().str(), "stylizer/pbr/material", config);
		return p;
	}

	shader_preprocessor& initialize_shader_preprocessor_defines(shader_preprocessor& p) {
		p.add_define("STYLIZER_PBR_TEXTURE_SLOT_ALBEDO", "0");
		p.add_define("STYLIZER_PBR_TEXTURE_SLOT_EMISSION", "1");
		p.add_define("STYLIZER_PBR_TEXTURE_SLOT_NORMAL", "2");
		p.add_define("STYLIZER_PBR_TEXTURE_SLOT_PACKED", "3");
		p.add_define("STYLIZER_PBR_TEXTURE_SLOT_ROUGHNESS", "4");
		p.add_define("STYLIZER_PBR_TEXTURE_SLOT_METALNESS", "5");
		p.add_define("STYLIZER_PBR_TEXTURE_SLOT_AMBIENT_OCCLUSION", "6");
		p.add_define("STYLIZER_PBR_TEXTURE_SLOT_ENVIRONMENT", "7");		
		return p;
	}

}
STYLIZER_END_NAMESPACE