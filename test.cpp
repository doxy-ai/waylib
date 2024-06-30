#include "waylib.hpp"
#include "window.hpp"
#include "model.hpp"

#include <iostream>

const char* shaderSource = R"(
@vertex
fn vertex(@location(0) in_vertex_position: vec3f) -> @builtin(position) vec4f {
    return vec4f(in_vertex_position.xzy, 1.0);
}

@fragment
fn fragment() -> @location(0) vec4f {
	return vec4f(0.0, 0.4, 1.0, 1.0);
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

	// Load the shader module
	wl::model model = wl::throw_if_null(wl::load_model(state, "../tri.obj"));
	model.material_count = 1;
	wl::shader shader = wl::create_shader(
		state, shaderSource, 
		{.vertex_entry_point = "vertex", .fragment_entry_point = "fragment", .name = "Default Shader"}
	);
	wl::material mat = wl::create_material(state, shader);
	model.materials = &mat;
	wl::index_t map = 0;
	model.mesh_materials = &map;

	wl::time time = {};
	while(!wl::window_should_close(window)) {
		wl::time_calculations(time);
		auto frame = wl::throw_if_null(
			wl::begin_drawing(state, wl::color8bit{229, 25, 51, 255})
		);
		{
			wl::model_draw(frame, model);
		}
		wl::end_drawing(state, frame);
	}

	wl::release_webgpu_state(state);
	wl::release_window(window);
}