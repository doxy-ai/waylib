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

@vertex
fn vertex(vertex: input_vertex, @builtin(instance_index) inst_id: u32) -> @builtin(position) vec4f {
	let transform = instances[inst_id].transform;
	return transform * vec4f(vertex.position, 1);
	// return vec4f(vertex.position.xzy, 1);
}

@fragment
fn fragment() -> @location(0) vec4f {
	let time = time.delta * 1400;
	return vec4f(vec3f(0.0, 0.4, 1.0) * time, 1);
})";

int main() {
	constexpr auto y = wl::vec3f(5);
	auto x = y.xx();
	// y.xy();
	std::cout << x.x << ", " << x.y << std::endl;

	auto window = wl::create_window(800, 600, "waylib");
	auto state = wl::create_default_device_from_window(window);

	auto uncapturedErrorCallbackHandle = state.device.setUncapturedErrorCallback([](wgpu::ErrorType type, char const* message) {
		std::cout << "Uncaptured device error: type " << type;
		if (message) std::cout << " (" << message << ")";
		std::cout << std::endl;
	});

	wl::window_automatically_reconfigure_surface_on_resize(window, state);
	auto queue = state.device.getQueue();

	std::cout << "Window: " << window << "\n"
		<< "Instance: " << state.device.getAdapter().getInstance() << "\n"
		<< "Adapter: " << state.device.getAdapter() << "\n"
		<< "Device: " << state.device << "\n"
		<< "Surface: " << state.surface << "\n"
		<< "Queue: " << queue << std::endl;
	std::cout << state.device.getQueue() << std::endl;

	// auto onQueueWorkDone = [](WGPUQueueWorkDoneStatus status, void* /* pUserData */) {
	// 	std::cout << "Queued work finished with status: " << status << std::endl;
	// };
	// wgpuQueueOnSubmittedWorkDone(queue, onQueueWorkDone, nullptr /* pUserData */);

	wl::shader_preprocessor* p = wl::create_shader_preprocessor();
	// wl::preprocessor_add_define(p, "flip", "1");
	auto texture = wl::load_image("../test.png");

	// wl::open_url("https://google.com");

	// Load the shader module
	wl::model model = wl::throw_if_null(wl::load_model(state, "../tri.obj"));
	model.material_count = 1;
	wl::shader shader = wl::throw_if_null(wl::create_shader(
		state, shaderSource, 
		{.vertex_entry_point = "vertex", .fragment_entry_point = "fragment", .name = "Default Shader", .preprocessor = p}
	));
	wl::material mat = wl::create_material(state, shader);
	model.materials = &mat;
	wl::index_t map = 0;
	model.mesh_materials = &map;

	wl::preprocess_shader_from_memory_and_cache(p, "#pragma once\n#define bob 5\n", "/virtual/bob.wgsl", {.support_pragma_once=true});
	std::cout << wl::get_error_message_and_clear() << std::endl;
	auto res = wl::preprocess_shader_from_memory(p, R"(
// #include "/virtual/bob.wgsl"

#if bob == 5
	is_bob = true
#else
	is_bob = false
#endif
	)", {true, true, false});
	std::cout << res << std::endl;
	std::cout << wl::preprocessor_get_cached_file(p, "/virtual/bob.wgsl") << std::endl;

	wl::camera3D camera = {{0, 3, -5}, {0, 0, 0}};

	wl::time time = {};
	while(!wl::window_should_close(window)) {
		wl::time_calculations(time);
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