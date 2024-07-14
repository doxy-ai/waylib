R"(#pragma once

@group(0) @binding(1) var color_texture: texture_2d<f32>;
@group(0) @binding(2) var color_sampler: sampler;
@group(0) @binding(3) var height_texture: texture_2d<f32>;
@group(0) @binding(4) var height_sampler: sampler;
@group(0) @binding(5) var normal_texture: texture_2d<f32>;
@group(0) @binding(6) var normal_sampler: sampler;
@group(0) @binding(7) var packed_texture: texture_2d<f32>;
@group(0) @binding(8) var packed_sampler: sampler;
@group(0) @binding(9) var roughness_texture: texture_2d<f32>;
@group(0) @binding(10) var roughness_sampler: sampler;
@group(0) @binding(11) var metalness_texture: texture_2d<f32>;
@group(0) @binding(12) var metalness_sampler: sampler;
@group(0) @binding(13) var ambient_occlusion_texture: texture_2d<f32>;
@group(0) @binding(14) var ambient_occlusion_sampler: sampler;
@group(0) @binding(15) var emission_texture: texture_2d<f32>;
@group(0) @binding(16) var emission_sampler: sampler;

fn waylib_sample_color(uv: vec2f) -> vec4f { return textureSample(color_texture, color_sampler, uv); }
fn waylib_sample_height(uv: vec2f) -> vec4f { return textureSample(height_texture, height_sampler, uv); }
fn waylib_sample_normal(uv: vec2f) -> vec4f { return textureSample(normal_texture, normal_sampler, uv); }
fn waylib_sample_packed(uv: vec2f) -> vec4f { return textureSample(packed_texture, packed_sampler, uv); }
fn waylib_sample_roughness(uv: vec2f) -> vec4f { return textureSample(roughness_texture, roughness_sampler, uv); }
fn waylib_sample_metalness(uv: vec2f) -> vec4f { return textureSample(metalness_texture, metalness_sampler, uv); }
fn waylib_sample_ambient_occlusion(uv: vec2f) -> vec4f { return textureSample(ambient_occlusion_texture, ambient_occlusion_sampler, uv); }
fn waylib_sample_emission(uv: vec2f) -> vec4f { return textureSample(emission_texture, emission_sampler, uv); }
)"