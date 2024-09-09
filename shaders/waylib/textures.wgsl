R"(#pragma once

fn scale_0_to_1_RGB(color: vec3f) -> vec3f {
	return color / (color + vec3(1.0));
}

fn scale_0_to_1(color: vec4f) -> vec4f {
	return color / (color + vec4(1.0));
}

fn srgb_to_linear_RGB(color: vec3f) -> vec3f {
	let gamma = 2.2;
	return pow(color, vec3(gamma));
}
fn srgb_to_linear(color: vec4f) -> vec4f {
	return vec4f(srgb_to_linear_RGB(color.rgb), color.a);
}

fn linear_to_srgb_RGB(color: vec3f) -> vec3f {
	let inverseGamma = 1.0 / 2.2;
	return pow(color, vec3(inverseGamma));
}
fn linear_to_srgb(color: vec4f) -> vec4f {
	return vec4f(linear_to_srgb_RGB(color.rgb), color.a);
}

fn compute_normal_mapping(tangent_space_normal: vec3f, tangent: vec3f, bitangent: vec3f, normal: vec3f, model_matrix: mat4x4f) -> vec3f {
	let T = normalize((model_matrix * vec4f(tangent, 0.0)).xyz);
	let B = normalize((model_matrix * vec4f(bitangent, 0.0)).xyz);
	let N = normalize((model_matrix * vec4f(normal, 0.0)).xyz);
	let TBN = mat3x3f(T, B, N);
	return TBN * tangent_space_normal;
}

#ifndef WAYLIB_IS_GEOMETRY_SHADER
	@group(0) @binding(5) var color_texture: texture_2d<f32>;
	@group(0) @binding(6) var color_sampler: sampler;
	@group(0) @binding(7) var cubemap_texture: texture_cube<f32>;
	@group(0) @binding(8) var cubemap_sampler: sampler;
	@group(0) @binding(9) var height_texture: texture_2d<f32>;
	@group(0) @binding(10) var height_sampler: sampler;
	@group(0) @binding(11) var normal_texture: texture_2d<f32>;
	@group(0) @binding(12) var normal_sampler: sampler;
	@group(0) @binding(13) var packed_texture: texture_2d<f32>;
	@group(0) @binding(14) var packed_sampler: sampler;
	@group(0) @binding(15) var roughness_texture: texture_2d<f32>;
	@group(0) @binding(16) var roughness_sampler: sampler;
	@group(0) @binding(17) var metalness_texture: texture_2d<f32>;
	@group(0) @binding(18) var metalness_sampler: sampler;
	@group(0) @binding(19) var ambient_occlusion_texture: texture_2d<f32>;
	@group(0) @binding(20) var ambient_occlusion_sampler: sampler;
	@group(0) @binding(21) var emission_texture: texture_2d<f32>;
	@group(0) @binding(22) var emission_sampler: sampler;
	

	fn waylib_sample_color(uv: vec2f) -> vec4f { return srgb_to_linear(textureSample(color_texture, color_sampler, uv)); }
	fn waylib_sample_cubemap(dir: vec3f) -> vec4f { return srgb_to_linear(textureSample(cubemap_texture, cubemap_sampler, dir)); }
	fn waylib_sample_height(uv: vec2f) -> vec4f { return textureSample(height_texture, height_sampler, uv); }
	fn waylib_sample_normal(uv: vec2f) -> vec4f { return textureSample(normal_texture, normal_sampler, uv); }
	fn waylib_sample_packed(uv: vec2f) -> vec4f { return textureSample(packed_texture, packed_sampler, uv); }
	fn waylib_sample_roughness(uv: vec2f) -> vec4f { return textureSample(roughness_texture, roughness_sampler, uv); }
	fn waylib_sample_metalness(uv: vec2f) -> vec4f { return textureSample(metalness_texture, metalness_sampler, uv); }
	fn waylib_sample_ambient_occlusion(uv: vec2f) -> vec4f { return textureSample(ambient_occlusion_texture, ambient_occlusion_sampler, uv); }
	fn waylib_sample_emission(uv: vec2f) -> vec4f { return srgb_to_linear(textureSample(emission_texture, emission_sampler, uv)); }

	fn waylib_normal_mapping(uv: vec2f, tangent: vec3f, bitangent: vec3f, normal: vec3f, model_matrix: mat4x4f) -> vec3f {
		let normalTangent = waylib_sample_normal(uv) * 2.0 - 1.0;
		return compute_normal_mapping(normalTangent.xyz, tangent, bitangent, normal, model_matrix);
	}
#else
	// @group(1) @binding(0) var color_texture: texture_2d<f32>;
	// @group(1) @binding(1) var color_sampler: sampler;
	// @group(1) @binding(2) var cubemap_texture: texture_cube<f32>;
	// @group(1) @binding(3) var cubemap_sampler: sampler;
	// @group(1) @binding(4) var height_texture: texture_2d<f32>;
	// @group(1) @binding(5) var height_sampler: sampler;
	// @group(1) @binding(6) var normal_texture: texture_2d<f32>;
	// @group(1) @binding(7) var normal_sampler: sampler;
	// @group(1) @binding(8) var packed_texture: texture_2d<f32>;
	// @group(1) @binding(9) var packed_sampler: sampler;
	// @group(1) @binding(10) var roughness_texture: texture_2d<f32>;
	// @group(1) @binding(11) var roughness_sampler: sampler;
	// @group(1) @binding(12) var metalness_texture: texture_2d<f32>;
	// @group(1) @binding(13) var metalness_sampler: sampler;
	// @group(1) @binding(14) var ambient_occlusion_texture: texture_2d<f32>;
	// @group(1) @binding(15) var ambient_occlusion_sampler: sampler;
	// @group(1) @binding(16) var emission_texture: texture_2d<f32>;
	// @group(1) @binding(17) var emission_sampler: sampler;
	@group(1) @binding(0) var color_texture: texture_2d<f32>;
	@group(1) @binding(1) var cubemap_texture: texture_2d<f32>;
	@group(1) @binding(2) var height_texture: texture_2d<f32>;
	@group(1) @binding(3) var normal_texture: texture_2d<f32>;
	@group(1) @binding(4) var packed_texture: texture_2d<f32>;
	@group(1) @binding(5) var roughness_texture: texture_2d<f32>;
	@group(1) @binding(6) var metalness_texture: texture_2d<f32>;
	@group(1) @binding(7) var ambient_occlusion_texture: texture_2d<f32>;
	@group(1) @binding(8) var emission_texture: texture_2d<f32>;
#endif
)"