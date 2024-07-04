#include "waylib.hpp"
#include "window.hpp"
#include "model.hpp"

#include "waylib.h" // Here so it won't keep getting auto added at the top!
#include "window.h"

#include <iostream>

const char* shaderSource = R"(
#include <waylib/time>
#include <waylib/vertex>
#include <waylib/instance>
#include <waylib/wireframe>

struct vertex_output {
	@builtin(position) position: vec4f,
	@location(0) barycentric_coordinates: vec3f,
};

@vertex
fn vertex(vertex: waylib_input_vertex, @builtin(instance_index) inst_id: u32, @builtin(vertex_index) vert_id: u32) -> vertex_output {
	let transform = instances[inst_id].transform;
	return vertex_output(transform * vec4f(vertex.position, 1), calculate_barycentric_coordinates(vert_id));
}

@fragment
fn fragment(vert: vertex_output) -> @location(0) vec4f {
	// if(wireframe_factor(vert.barycentric_coordinates, 2) < .1) {
	// 	discard;
	// }
	if(wireframe_factor(vert.barycentric_coordinates, 2) > .1) {
		return vec4f(vec3f(0), 1);
	}
	let time = time.delta * 1400;
#if WGPU_BACKEND_TYPE == WGPU_BACKEND_TYPE_VULKAN
	return vec4f(vec3f(0.0, 0.4, 1.0) * time, 1);
#else
	return vec4f(vec3f(0.4, 0, 1.0) * time, 1);
#endif
})";

int main() {
	auto window = wl::create_window(800, 600, "waylib");
	auto state = wl::create_default_device_from_window(window);
	wl::window_automatically_reconfigure_surface_on_resize(window, state);

	auto uncapturedErrorCallbackHandle = state.device.setUncapturedErrorCallback([](wgpu::ErrorType type, char const* message) {
		std::cerr << "[WAYLIB] Uncaptured device error: type " << type;
		if (message) std::cerr << " (" << message << ")";
		std::cerr << std::endl;
	});

	// Load the shader module
	wl::model model = wl::throw_if_null(wl::load_model(state, "../suzane.gltf"));
	model.material_count = 1;
	wl::shader_preprocessor* p = wl::preprocessor_initialize_virtual_filesystem(wl::create_shader_preprocessor(), state);
	wl::shader shader = wl::throw_if_null(wl::create_shader(
		state, shaderSource,
		{.vertex_entry_point = "vertex", .fragment_entry_point = "fragment", .name = "Default Shader", .preprocessor = p}
	));
	wl::material mat = wl::create_material(state, shader);
	model.materials = &mat;
	model.mesh_materials = nullptr;

	wl::camera3D camera = {{0, 1, -1}, {0, 0, 0}};

	wl::time time = {};
	while(!wl::window_should_close(window)) {
		wl::time_calculations(time);

		camera.position = wl::vec3f(2 * wl::cos(time.since_start), wl::sin(time.since_start / 4), 2 * wl::sin(time.since_start));

		auto frame = wl::throw_if_null(
			wl::begin_drawing(state, wl::color{0.9, .1,  0.2, 1})
		);
		{
			wl::time_upload(frame, time);
			wl::window_begin_camera_mode3D(frame, window, camera);
			{
				wl::model_draw(frame, model);
			}
			wl::end_camera_mode(frame);
		}
		wl::end_drawing(frame);
	}

	wl::release_shader_preprocessor(p);
	wl::release_wgpu_state(state);
	wl::release_window(window);
}