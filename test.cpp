#include "waylib.hpp"
#include "window.hpp"
#include "model.hpp"

#include <iostream>

const char* shaderSource = R"(
struct time_data {
	since_start: f32,
	delta: f32,
	average_delta: f32,
};

struct model_instance_data {
    transform: mat4x4f,
    tint: vec4f,
};

@group(0) @binding(0) var<storage, read> instances: array<model_instance_data>;
@group(1) @binding(0) var<uniform> time: time_data;

struct input_vertex {
	@location(0) position: vec3f,
	@location(1) texcoords: vec2f,
	@location(2) normal: vec3f,
	@location(3) color: vec4f,
	@location(4) tangents: vec4f,
	@location(5) texcoords2: vec2f,
};



fn calculate_barycentric_coordinates(vertex_index: u32) -> vec3f {
	const barycentric_coordinates = array<vec3f, 3>(vec3f(1, 0, 0), vec3f(0, 1, 0), vec3f(0, 0, 1));
	return barycentric_coordinates[vertex_index % 3];
}

struct vert_output {
	@builtin(position) position: vec4f,
	@location(0) barycentric_coordinates: vec3f,
};

@vertex
fn vertex(vertex: input_vertex, @builtin(instance_index) inst_id: u32, @builtin(vertex_index) vert_id: u32) -> vert_output {
	let transform = instances[inst_id].transform;
	return vert_output(transform * vec4f(vertex.position, 1), calculate_barycentric_coordinates(vert_id));
}

// From: https://tchayen.github.io/posts/wireframes-with-barycentric_coordinates-coordinates
fn wireframe_factor(barycentric_coordinates: vec3f, line_width: f32) -> f32 {
  let d = fwidth(barycentric_coordinates);
  let f = step(d * line_width / 2, barycentric_coordinates);
  return 1 - min(min(f.x, f.y), f.z);
}

@fragment
fn fragment(in: vert_output) -> @location(0) vec4f {
	// if(wireframe_factor(in.barycentric_coordinates, 2) < .1) {
	// 	discard;
	// }
	if(wireframe_factor(in.barycentric_coordinates, 2) > .1) {
		return vec4f(vec3f(0), 1);
	}
	let time = time.delta * 1400;
	return vec4f(vec3f(0.0, 0.4, 1.0) * time, 1);
})";

int main() {
	auto window = wl::create_window(800, 600, "waylib");
	auto state = wl::create_default_device_from_window(window);
	wl::window_automatically_reconfigure_surface_on_resize(window, state);

	auto uncapturedErrorCallbackHandle = state.device.setUncapturedErrorCallback([](wgpu::ErrorType type, char const* message) {
		std::cout << "Uncaptured device error: type " << type;
		if (message) std::cout << " (" << message << ")";
		std::cout << std::endl;
	});


	// Load the shader module
	wl::model model = wl::throw_if_null(wl::load_model(state, "../suzane.glb"));
	model.material_count = 1;
	wl::shader_preprocessor* p = wl::create_shader_preprocessor();
	wl::shader shader = wl::throw_if_null(wl::create_shader(
		state, shaderSource, 
		{.vertex_entry_point = "vertex", .fragment_entry_point = "fragment", .name = "Default Shader", .preprocessor = p}
	));
	wl::material mat = wl::create_material(state, shader);
	model.materials = &mat;
	wl::index_t map = 0;
	model.mesh_materials = &map;

	wl::camera3D camera = {{0, 1, -1}, {0, 0, 0}};

	wl::time time = {};
	while(!wl::window_should_close(window)) {
		wl::time_calculations(time);

		camera.position = wl::vec3f(2 * cos(time.since_start), 1, 2 * sin(time.since_start));

		auto frame = wl::throw_if_null(
			wl::begin_drawing(state, wl::color{0.9, .1,  0.2, 1})
		);
		{
			wl::time_upload(frame, time);
			wl::window_begin_camera_mode3D(frame, window, camera);
			{
				auto dbg = frame.current_VP;
				wl::model_draw(frame, model);
			}
			wl::end_camera_mode(frame);
		}
		wl::end_drawing(frame);
	}

	wl::release_shader_preprocessor(p);
	wl::release_webgpu_state(state);
	wl::release_window(window);
}