R"(#pragma once

struct waylib_light_data {
	light_type: u32,
	position: vec3f,
	direction: vec3f,
	
	color: vec4f,
	intensity: f32,
	
	cutoff_start_angle: f32,
	cutoff_end_angle: f32,
	
	constant: f32,
	linear: f32,
	quadratic: f32,
};
@group(1) @binding(1) var<storage, read> lights: array<waylib_light_data>;
)"