R"(#pragma once

// #include <waylib/instance>
// #include <waylib/camera>

struct waylib_input_vertex {
	@location(0) position: vec3f,
	@location(1) texcoords: vec2f,
	@location(2) normal: vec3f,
	@location(3) color: vec4f,
	@location(4) tangents: vec4f,
	@location(5) texcoords2: vec2f,
};

struct waylib_output_vertex {
	@builtin(position) position: vec4f,
	@location(0) world_position: vec3f,
	@location(1) texcoords: vec2f,
	@location(2) normal: vec3f,
	@location(3) color: vec4f,
	@location(4) tangents: vec4f,
	@location(5) texcoords2: vec2f,
};

#ifdef WAYLIB_USE_DEFAULT_VERTEX_SHADER_AS_ENTRY_POINT
@vertex
fn waylib_default_vertex_shader(in: waylib_input_vertex, @builtin(instance_index) instance_index: u32) -> waylib_output_vertex
#else
fn waylib_default_vertex_shader(in: waylib_input_vertex, instance_index: u32) -> waylib_output_vertex
#endif
{
	let transform = instances[instance_index].transform;
	let tint = instances[instance_index].tint;
	let world_position = transform * vec4f(in.position, 1);
	return waylib_output_vertex(
		camera.current_VP * world_position,
		world_position.xyz,
		in.texcoords,
		in.normal,
		tint * in.color,
		in.tangents,
		in.texcoords2
	);
}
)"