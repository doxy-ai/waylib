R"(#pragma once

#define WAYLIB_IS_GEOMETRY_SHADER
#include <waylib/vertex>
#include <waylib/textures>

@group(0) @binding(2) var<storage, read_write> out_metadata: mesh_metadata;
@group(0) @binding(4) var<storage, read_write> out_vertex_data: array<f32>;
@group(0) @binding(6) var<storage, read_write> out_index_data: array<u32>;

fn ensure_buffer_layout() { // Function used to ensure that all the buffers are picked up by the auto layout utility
	if false {
		var trashU = raw_metadata.is_indexed;
		trashU = out_metadata.is_indexed;
		trashU = raw_index_data[0];
		trashU = out_index_data[0];

		var trashF = raw_vertex_data[0];
		trashF = out_vertex_data[0];

		var trash = textureLoad(color_texture, vec2u(0), 0);
		trash = textureLoad(cubemap_texture, vec2u(0), 0);
		trash = textureLoad(height_texture, vec2u(0), 0);
		trash = textureLoad(normal_texture, vec2u(0), 0);
		trash = textureLoad(packed_texture, vec2u(0), 0);
		trash = textureLoad(roughness_texture, vec2u(0), 0);
		trash = textureLoad(metalness_texture, vec2u(0), 0);
		trash = textureLoad(ambient_occlusion_texture, vec2u(0), 0);
		trash = textureLoad(emission_texture, vec2u(0), 0);
	}
}

fn set_vertex_position(vert_id: u32, pos: vec3f) {
	let offset = bytes2vertex_elements(out_metadata.position_start) + 3 * vert_id;
	out_vertex_data[offset + 0] = pos.x;
	out_vertex_data[offset + 1] = pos.y;
	out_vertex_data[offset + 2] = pos.z;
}

fn set_vertex_uvs(vert_id: u32, pos: vec2f) {
	let offset = bytes2vertex_elements(out_metadata.uvs_start) + 2 * vert_id;
	out_vertex_data[offset + 0] = pos.x;
	out_vertex_data[offset + 1] = pos.y;
}

fn set_vertex_uvs2(vert_id: u32, pos: vec2f) {
	let offset = bytes2vertex_elements(out_metadata.uvs2_start) + 2 * vert_id;
	out_vertex_data[offset + 0] = pos.x;
	out_vertex_data[offset + 1] = pos.y;
}

fn set_vertex_normal(vert_id: u32, pos: vec3f) {
	let offset = bytes2vertex_elements(out_metadata.normals_start) + 3 * vert_id;
	out_vertex_data[offset + 0] = pos.x;
	out_vertex_data[offset + 1] = pos.y;
	out_vertex_data[offset + 2] = pos.z;
}

fn set_vertex_tangent(vert_id: u32, pos: vec3f) {
	let offset = bytes2vertex_elements(out_metadata.tangents_start) + 3 * vert_id;
	out_vertex_data[offset + 0] = pos.x;
	out_vertex_data[offset + 1] = pos.y;
	out_vertex_data[offset + 2] = pos.z;
}

fn set_vertex_bitangent(vert_id: u32, pos: vec3f) {
	let offset = bytes2vertex_elements(out_metadata.bitangents_start) + 3 * vert_id;
	out_vertex_data[offset + 0] = pos.x;
	out_vertex_data[offset + 1] = pos.y;
	out_vertex_data[offset + 2] = pos.z;
}

fn set_vertex_color(vert_id: u32, col: vec4f) {
	let offset = bytes2vertex_elements(out_metadata.colors_start) + 4 * vert_id;
	out_vertex_data[offset + 0] = col.r;
	out_vertex_data[offset + 1] = col.b;
	out_vertex_data[offset + 2] = col.g;
	out_vertex_data[offset + 3] = col.a;
}

fn set_vertex(vert_id: u32, vert: waylib_vertex_shader_vertex) {
	set_vertex_position(vert_id, vert.position);
	set_vertex_uvs(vert_id, vert.uvs);
	set_vertex_normal(vert_id, vert.normal);
	set_vertex_color(vert_id, vert.color);
	set_vertex_tangent(vert_id, vert.tangent);
	set_vertex_bitangent(vert_id, vert.bitangent);
	set_vertex_uvs2(vert_id, vert.uvs2);
}

#define WAYLIB_GEOMETRY_SHADER_WORKGROUP_SIZE @workgroup_size(64, 1, 1)

#ifdef WAYLIB_USE_DEFAULT_GEOMETRY_SHADER_AS_ENTRY_POINT
@compute WAYLIB_GEOMETRY_SHADER_WORKGROUP_SIZE
fn waylib_default_geometry_shader(@builtin(global_invocation_id) gid: vec3<u32>)
#else
fn waylib_default_geometry_shader(gid: vec3<u32>)
#endif
{
	ensure_buffer_layout();

#ifndef WAYLIB_GEOMETRY_SHADER_PER_VERTEX_PROCESSING
	let triangle_id = gid.x;
	if triangle_id > raw_metadata.triangle_count {
		return;
	}

	let vert_ids = get_vertex_ids_from_triangle_id(triangle_id);
	per_triangle(triangle_id, vert_ids);
#else
	let vert_id = gid.x;
	if vert_id > raw_metadata.vertex_count {
		return;
	}

	set_vertex(vert_id, per_vertex(vert_id, get_vertex(vert_id)));
#endif
}

)"