R"(#pragma once

#include <waylib/vertex>
#include <waylib/textures>

struct pbr_data_t {
	color: vec4f,
	emmission: vec4f,
	roughness: f32,
	metalness: f32,

	height_displacement_factor: f32,
	normal_strength: f32,

	use_color_map: u32,
	use_height_map: u32,
	use_normal_map: u32,
	use_packed_map: u32,
	use_roughness_map: u32,
	use_metalness_map: u32,
	use_ambient_occlusion_map: u32,
	use_emmission_map: u32,
	use_enviornment_map: u32,
};
@group(0) @binding(1) var<uniform> pbr_data: pbr_data_t;

#ifndef WAYLIB_IS_GEOMETRY_SHADER
	struct pbr_material {
		color: vec4f,
		emission: vec4f,
		normal: vec3f, // NOTE: Assumed to be normalized
		environment: vec3f,
		roughness: f32,
		metalness: f32,
		ambient_occlusion: f32,
	};

	fn build_default_pbr_material(vertex: waylib_fragment_shader_vertex, model_matrix: mat4x4f) -> pbr_material {
		let packed = waylib_sample_packed(vertex.uvs);
		var color = pbr_data.color;
		if pbr_data.use_color_map > 0 {
			color = waylib_sample_color(vertex.uvs);
		}
		var emmission = pbr_data.emmission;
		if pbr_data.use_emmission_map > 0 {
			emmission = waylib_sample_emission(vertex.uvs);
		}
		var normal = vertex.normal;
		if pbr_data.use_normal_map > 0 {
			normal = mix(normal, waylib_normal_mapping(vertex.uvs, vertex.tangent, vertex.bitangent, vertex.normal, model_matrix), pbr_data.normal_strength);
		}
		normal = normalize(normal);

		var enviornment = vec3f(.1);
		if pbr_data.use_enviornment_map > 0 {
			let sample = waylib_sample_cubemap(normal);
			enviornment = sample.rgb * sample.a; // TODO: Pre multiplied alpha? Replace?
		}

		var roughness = pbr_data.roughness;
		if pbr_data.use_packed_map > 0 {
			roughness = packed.r;
		} else if pbr_data.use_roughness_map > 0 {
			roughness = waylib_sample_roughness(vertex.uvs).r;
		}

		var metalness = pbr_data.metalness;
		if pbr_data.use_packed_map > 0 {
			metalness = packed.g;
		} else if pbr_data.use_metalness_map > 0 {
			metalness = waylib_sample_metalness(vertex.uvs).g;
		}

		var ao = 1.0;
		if pbr_data.use_packed_map > 0 {
			ao = packed.b;
		} else if pbr_data.use_ambient_occlusion_map > 0 {
			ao = waylib_sample_ambient_occlusion(vertex.uvs).b;
		}

		return pbr_material(
			color,
			emmission,
			normal,
			enviornment,
			roughness,
			metalness,
			ao
		);
	}
#endif // WAYLIB_IS_GEOMETRY_SHADER
)"