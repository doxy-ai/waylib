R"(
#define WAYLIB_USE_DEFAULT_VERTEX_SHADER_AS_ENTRY_POINT
#include <waylib/pbr>

@fragment
fn waylib_default_fragment_shader(vert: waylib_fragment_shader_vertex) -> @location(0) vec4f {
	let mat = build_default_pbr_material(vert, instances[vert.instance_index].transform);
	let light = lights[0];
	return vec4(linear_to_srgb_RGB(pbr_metalrough(light, mat, vert).rgb + pbr_simple_ambient(light, mat, vert).rgb), 1);
}
)"