#pragma once

#include <stylizer/mesh_data>
#include <stylizer/utility_data>
#include <stylizer/vertex_data>
#include <stylizer/default_gbuffer_data>

@group(0) @binding(0) var<storage, read> utility : utility_data;
@group(0) @binding(1) var previous_color_texture: texture_2d<f32>;
@group(0) @binding(2) var previous_color_sampler: sampler;
@group(0) @binding(3) var previous_depth_texture: texture_depth_2d;
@group(0) @binding(4) var previous_depth_sampler: sampler;
@group(0) @binding(5) var previous_normal_texture: texture_2d<f32>;
@group(0) @binding(6) var previous_normal_sampler: sampler;

@group(1) @binding(0) var<storage, read> mesh : mesh_meta_and_data;
@group(1) @binding(1) var<storage, read> mesh_indices : array<u32>;
@group(1) @binding(2) var<storage, read> instances : array<instance_data>;

fn ensure_gbuffer_layout() {
	var trash = mesh.vertex_count;
	trash = mesh_indices[0];
	var trashF = utility.time.delta;
	trashF = instances[0].tint.r;
}

fn ensure_gbuffer_fragment_layout() {
	var trash = textureSample(previous_color_texture, previous_color_sampler, vec2(0));
	let trashF = textureSample(previous_depth_texture, previous_depth_sampler, vec2(0));
	trash = textureSample(previous_normal_texture, previous_normal_sampler, vec2(0));
}