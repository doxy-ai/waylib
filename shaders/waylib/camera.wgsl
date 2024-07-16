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
	pixel_perfect: u32,
};

struct waylib_camera_data {
	is_3D: u32,
	settings3D: camera3D,
	settings2D: camera2D,
	window_dimensions: vec2i,
	view_matrix: mat4x4f,
	projection_matrix: mat4x4f,
};
@group(1) @binding(0) var<uniform> camera: waylib_camera_data;

fn current_view_projection_matrix() -> mat4x4f {
	return camera.projection_matrix * camera.view_matrix;
}

fn inverse_view_matrix_impl(view_matrix: mat4x4f) -> mat4x4f {
	let rot = transpose(mat3x3f(
		view_matrix[0].xyz, view_matrix[1].xyz, view_matrix[2].xyz
	));
	let trans = view_matrix[3];
	return mat4x4f(
		vec4f(rot[0], 0),
		vec4f(rot[1], 0),
		vec4f(rot[2], 0),
		vec4f(-trans.x, -trans.y, -trans.z, 1)
	);
}
fn inverse_view_matrix() -> mat4x4f {
	return inverse_view_matrix_impl(camera.view_matrix);
}

#include <waylib/inverse>

// Warning! Expensive
fn inverse_projection_matrix() -> mat4x4f {
	return inverse4x4(camera.projection_matrix);
}

// Warning! Expensive!
fn inverse_view_projection_matrix() -> mat4x4f {
	return inverse_view_matrix() * inverse_projection_matrix();
}
)"