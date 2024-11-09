#pragma once

struct pbr_material_settings {
	material_id: u32,
	albedo: vec4f,
	emission: vec4f,
	metalness: f32,
	roughness: f32,
	normal_strength: f32,
	used_textures: u32,
};

struct pbr_material {
	albedo: vec4f,
	emission: vec4f,
	normal: vec3f, // NOTE: Assumed to be normalized
	environment: vec3f,
	metalness: f32,
	roughness: f32,
	ambient_occlusion: f32,
}

fn packed_from_pbr_material(mat: pbr_material, shadows: f32) -> vec4f {
	return vec4f(mat.metalness, mat.roughness, mat.ambient_occlusion, shadows);
}