R"(#pragma once

struct camera3D {
    position: vec3f,
    target_position: vec3f,
	up: vec3f,
	field_of_view: f32,
	near_clip_distance: f32,
    far_clip_distance: f32,
	orthographic: u32,
};

struct camera2D {
    offset: vec3f,
	target_position: vec3f,
	rotation: f32,
	near_clip_distance: f32,
    far_clip_distance: f32,
	zoom: f32,
};

struct waylib_camera_data {
	is_3D: u32,
	settings3D: camera3D,
	settings2D: camera2D,
    window_dimension: vec2i,
	current_VP: mat4x4f,
};
@group(2) @binding(0) var<uniform> camera: waylib_camera_data;
)"