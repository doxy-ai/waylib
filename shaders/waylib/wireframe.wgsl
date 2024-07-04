R"(
#pragma once

fn calculate_barycentric_coordinates(vertex_index: u32) -> vec3f {
	const barycentric_coordinates = array<vec3f, 3>(vec3f(1, 0, 0), vec3f(0, 1, 0), vec3f(0, 0, 1));
	return barycentric_coordinates[vertex_index % 3];
}

// From: https://tchayen.github.io/posts/wireframes-with-barycentric_coordinates-coordinates
fn wireframe_factor(barycentric_coordinates: vec3f, line_width: f32) -> f32 {
  let d = fwidth(barycentric_coordinates);
  let f = step(d * line_width / 2, barycentric_coordinates);
  return 1 - min(min(f.x, f.y), f.z);
}
)"