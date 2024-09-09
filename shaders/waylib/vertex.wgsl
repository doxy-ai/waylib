R"(#pragma once

#ifndef WAYLIB_IS_GEOMETRY_SHADER
	#include <waylib/instance>
	#include <waylib/camera>
	#ifdef WAYLIB_VERTEX_SUPPORTS_WIREFRAME
		#include <waylib/wireframe>
	#endif
#endif

struct mesh_metadata {
	is_indexed: u32,
	vertex_count: u32,
	triangle_count: u32,

	position_start: u32,
	uvs_start: u32,
	uvs2_start: u32,
	normals_start: u32,
	tangents_start: u32,
	bitangents_start: u32,
	colors_start: u32,
};
#ifndef WAYLIB_IS_GEOMETRY_SHADER
@group(0) @binding(2) var<storage, read> raw_metadata: mesh_metadata;
@group(0) @binding(3) var<storage, read> raw_vertex_data: array<f32>;
@group(0) @binding(4) var<storage, read> raw_index_data: array<u32>;
#else
@group(0) @binding(0) var<storage, read> raw_metadata: mesh_metadata;
@group(0) @binding(3) var<storage, read> raw_vertex_data: array<f32>;
@group(0) @binding(5) var<storage, read> raw_index_data: array<u32>;
#endif

fn get_vertex_ids_from_triangle_id(triangle_id: u32) -> vec3<u32> {
	var vertA : u32 = 3 * triangle_id + 0;
	var vertB : u32 = 3 * triangle_id + 1;
	var vertC : u32 = 3 * triangle_id + 2;
	if raw_metadata.is_indexed != 0 {
		vertA = raw_index_data[vertA];
		vertB = raw_index_data[vertB];
		vertC = raw_index_data[vertC];
	}
	return vec3u(vertA, vertB, vertC);
}

fn bytes2vertex_elements(byte: u32) -> u32 {
	return byte / 4;
}

fn get_vertex_position(vert_id: u32) -> vec3f {
	let offset = bytes2vertex_elements(raw_metadata.position_start) + 3 * vert_id;
	let x = raw_vertex_data[offset + 0];
	let y = raw_vertex_data[offset + 1];
	let z = raw_vertex_data[offset + 2];
	return vec3f(x, y, z);
}

fn get_vertex_uvs(vert_id: u32) -> vec2f {
	let offset = bytes2vertex_elements(raw_metadata.uvs_start) + 2 * vert_id;
	let u = raw_vertex_data[offset + 0];
	let v = raw_vertex_data[offset + 1];
	return vec2f(u, v);
}

fn get_vertex_uvs2(vert_id: u32) -> vec2f {
	let offset = bytes2vertex_elements(raw_metadata.uvs2_start) + 2 * vert_id;
	let u = raw_vertex_data[offset + 0];
	let v = raw_vertex_data[offset + 1];
	return vec2f(u, v);
}

fn get_vertex_normal(vert_id: u32) -> vec3f {
	let offset = bytes2vertex_elements(raw_metadata.normals_start) + 3 * vert_id;
	let x = raw_vertex_data[offset + 0];
	let y = raw_vertex_data[offset + 1];
	let z = raw_vertex_data[offset + 2];
	return vec3f(x, y, z);
}

fn get_vertex_tangent(vert_id: u32) -> vec3f {
	let offset = bytes2vertex_elements(raw_metadata.tangents_start) + 3 * vert_id;
	let x = raw_vertex_data[offset + 0];
	let y = raw_vertex_data[offset + 1];
	let z = raw_vertex_data[offset + 2];
	return vec3f(x, y, z);
}

fn get_vertex_bitangent(vert_id: u32) -> vec3f {
	let offset = bytes2vertex_elements(raw_metadata.bitangents_start) + 3 * vert_id;
	let x = raw_vertex_data[offset + 0];
	let y = raw_vertex_data[offset + 1];
	let z = raw_vertex_data[offset + 2];
	return vec3f(x, y, z);
}

fn get_vertex_color(vert_id: u32) -> vec4f {
	let offset = bytes2vertex_elements(raw_metadata.colors_start) + 4 * vert_id;
	let r = raw_vertex_data[offset + 0];
	let g = raw_vertex_data[offset + 1];
	let b = raw_vertex_data[offset + 2];
	let a = raw_vertex_data[offset + 3];
	return vec4f(r, g, b, a);
}

fn get_vertex(vert_id: u32) -> waylib_vertex_shader_vertex {
	return waylib_vertex_shader_vertex(
		get_vertex_position(vert_id),
		get_vertex_uvs(vert_id),
		get_vertex_normal(vert_id),
		get_vertex_color(vert_id),
		get_vertex_tangent(vert_id),
		get_vertex_bitangent(vert_id),
		get_vertex_uvs2(vert_id),
	);
}

struct waylib_vertex_shader_vertex {
	@location(0) position: vec3f,
	@location(1) uvs: vec2f,
	@location(2) normal: vec3f,
	@location(3) color: vec4f,
	@location(4) tangent: vec3f,
	@location(5) bitangent: vec3f,
	@location(6) uvs2: vec2f,
};

struct waylib_fragment_shader_vertex {
	@builtin(position) raw_position: vec4f,
	@location(0) position: vec3f,
	@location(1) uvs: vec2f,
	@location(2) normal: vec3f,
	@location(3) color: vec4f,
	@location(4) tangent: vec3f,
	@location(5) bitangent: vec3f,
	@location(6) uvs2: vec2f,
	@interpolate(flat) @location(7) instance_index: u32,
#ifdef WAYLIB_VERTEX_SUPPORTS_WIREFRAME
	@location(8) barycentric_coordinates: vec3f,
#endif
};

#ifndef WAYLIB_IS_GEOMETRY_SHADER
	#ifdef WAYLIB_USE_DEFAULT_VERTEX_SHADER_AS_ENTRY_POINT
	@vertex
	fn waylib_default_vertex_shader(in: waylib_vertex_shader_vertex, @builtin(instance_index) instance_index: u32, @builtin(vertex_index) vertex_index: u32) -> waylib_fragment_shader_vertex
	#else
	fn waylib_default_vertex_shader(in: waylib_vertex_shader_vertex, instance_index: u32, vertex_index: u32) -> waylib_fragment_shader_vertex
	#endif
	{
		let transform = instances[instance_index].transform;
		let tint = instances[instance_index].tint;
		let world_position = transform * vec4f(in.position, 1);
		return waylib_fragment_shader_vertex(
			current_view_projection_matrix() * world_position,
			world_position.xyz,
			in.uvs,
			in.normal,
			tint * in.color,
			in.tangent,
			in.bitangent,
			in.uvs2,
			instance_index,
	#ifdef WAYLIB_VERTEX_SUPPORTS_WIREFRAME
			calculate_barycentric_coordinates(vertex_index)
	#endif
		);
	}
#endif
)"