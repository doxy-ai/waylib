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

fn light_intensity_attenuation(light: waylib_light_data, world_position: vec3f) -> f32 {
	return light.intensity; // TODO: Attenuate
}

fn light_direction(light: waylib_light_data, world_position: vec3f) -> vec3f {
	if light.light_type == WAYLIB_LIGHT_TYPE_POINT {
		return normalize(light.position - world_position);
	}
	return normalize(-light.direction);
}
)"