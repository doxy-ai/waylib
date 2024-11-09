#pragma once

#include <stylizer/pbr/material_data>
#include <stylizer/vertex_data>

@group(2) @binding(0) var<storage> material_settings: pbr_material_settings;

@group(2) @binding(1) var albedo_texture: texture_2d<f32>;
@group(2) @binding(2) var albedo_sampler: sampler;
@group(2) @binding(3) var emission_texture: texture_2d<f32>;
@group(2) @binding(4) var emission_sampler: sampler;
@group(2) @binding(5) var normal_texture: texture_2d<f32>;
@group(2) @binding(6) var normal_sampler: sampler;
@group(2) @binding(7) var packed_texture: texture_2d<f32>;
@group(2) @binding(8) var packed_sampler: sampler;
@group(2) @binding(9) var roughness_texture: texture_2d<f32>;
@group(2) @binding(10) var roughness_sampler: sampler;
@group(2) @binding(11) var metalness_texture: texture_2d<f32>;
@group(2) @binding(12) var metalness_sampler: sampler;
@group(2) @binding(13) var ambient_occlusion_texture: texture_2d<f32>;
@group(2) @binding(14) var ambient_occlusion_sampler: sampler;
@group(2) @binding(15) var environment_texture: texture_cube<f32>;
@group(2) @binding(16) var environment_sampler: sampler;

fn ensure_material_layout() {
	var trash = material_settings.albedo;
	trash = textureSample(albedo_texture, albedo_sampler, vec2(0));
	trash = textureSample(emission_texture, emission_sampler, vec2(0));
	trash = textureSample(normal_texture, normal_sampler, vec2(0));
	trash = textureSample(packed_texture, packed_sampler, vec2(0));
	trash = textureSample(roughness_texture, roughness_sampler, vec2(0));
	trash = textureSample(metalness_texture, metalness_sampler, vec2(0));
	trash = textureSample(ambient_occlusion_texture, ambient_occlusion_sampler, vec2(0));
	trash = textureSample(environment_texture, environment_sampler, vec3(0));
}

fn build_default_pbr_material(settings: pbr_material_settings, vert: vertex_output) -> pbr_material {
	let packed = textureSample(packed_texture, packed_sampler, vert.uv);
	var albedo = settings.albedo;
	if extractBits(settings.used_textures, STYLIZER_PBR_TEXTURE_SLOT_ALBEDO, 1) > 0 {
		albedo = textureSample(albedo_texture, albedo_sampler, vert.uv);
	}
	var emission = settings.emission;
	if extractBits(settings.used_textures, STYLIZER_PBR_TEXTURE_SLOT_EMISSION, 1) > 0 {
		emission = textureSample(emission_texture, emission_sampler, vert.uv);
	}
	var normal = vert.normal;
	if extractBits(settings.used_textures, STYLIZER_PBR_TEXTURE_SLOT_NORMAL, 1) > 0 {
		let tangent_space_normal = textureSample(normal_texture, normal_sampler, vert.uv);
		normal = mix(normal, normal_mapping(vert, tangent_space_normal.xyz), settings.normal_strength);
	}
	normal = normalize(normal);

	var environment = vec3f(.1);
	if extractBits(settings.used_textures, STYLIZER_PBR_TEXTURE_SLOT_ENVIRONMENT, 1) > 0 {
		let sample = textureSample(environment_texture, environment_sampler, normal);
		environment = sample.rgb * sample.a; // TODO: Pre multiplied alpha? Replace?
	}

	var metalness = settings.metalness;
	if extractBits(settings.used_textures, STYLIZER_PBR_TEXTURE_SLOT_PACKED, 1) > 0 {
		metalness = packed.r;
	} else if extractBits(settings.used_textures, STYLIZER_PBR_TEXTURE_SLOT_METALNESS, 1) > 0 {
		metalness = textureSample(metalness_texture, metalness_sampler, vert.uv).r;
	}

	var roughness = settings.roughness;
	if extractBits(settings.used_textures, STYLIZER_PBR_TEXTURE_SLOT_PACKED, 1) > 0 {
		roughness = packed.g;
	} else if extractBits(settings.used_textures, STYLIZER_PBR_TEXTURE_SLOT_ROUGHNESS, 1) > 0 {
		roughness = textureSample(roughness_texture, roughness_sampler, vert.uv).r;
	}

	var ao = 1.0;
	if extractBits(settings.used_textures, STYLIZER_PBR_TEXTURE_SLOT_PACKED, 1) > 0 {
		ao = packed.b;
	} else if extractBits(settings.used_textures, STYLIZER_PBR_TEXTURE_SLOT_AMBIENT_OCCLUSION, 1) > 0 {
		ao = textureSample(ambient_occlusion_texture, ambient_occlusion_sampler, vert.uv).r;
	}

	return pbr_material(
		albedo,
		emission,
		normal,
		environment,
		metalness,
		roughness,
		ao
	);
}