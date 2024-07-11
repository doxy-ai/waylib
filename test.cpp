#include "waylib.hpp"
#include "window.hpp"
#include "obj_loader.hpp"

#include "waylib.h" // Here so it won't keep getting auto added at the top!
#include "window.h"

#include <glm/geometric.hpp>
#include <iostream>

const char* shaderSource = R"(
#include <waylib/time>
#define WAYLIB_USE_DEFAULT_VERTEX_SHADER_AS_ENTRY_POINT
#include <waylib/vertex>
#include <waylib/light>

@fragment
fn fragment(vert: waylib_output_vertex) -> @location(0) vec4f {
	let time = time.delta * 1400;
	let light = lights[0];
	return vec4f(light.color.rgb * time, 1);
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
	wl::model model = wl::throw_if_null(wl::load_obj_model("../suzane.obj", state));
	model.material_count = 1;
	wl::shader_preprocessor* p = wl::preprocessor_initialize_virtual_filesystem(wl::create_shader_preprocessor(), state);
	wl::shader shader = wl::throw_if_null(wl::create_shader(
		state, shaderSource,
		{.vertex_entry_point = "waylib_default_vertex_shader", .fragment_entry_point = "fragment", .name = "Default Shader", .preprocessor = p}
	));
	wl::material mat = wl::create_material(state, shader);
	model.materials = &mat;
	model.mesh_materials = nullptr;

	wl::camera3D camera = {.position={0, 1, -1}, .target={0, 0, 0}};
	std::array<wl::light, 1> lights = {wl::light{
		.type = wl::light_type::Directional,
		.direction = glm::normalize(glm::vec3{-1, -1, 0}),
		.color = {1, 1, 1, 1}, .intensity = 1
	}};
	// wl::camera3D camera = {{1, 2, 3}, 4, {5, 6, 7}, 8, {9, 10, 11}, 12, 13, 14, true};
	// std::array<wl::light, 1> lights = {wl::light{
	// 	(wl::light_type)1, 
	// 	{2, 3, 4}, {5, 6, 7}, 
	// 	8, {9, 10, 11}, 
	// 	12, {13, 14, 15, 16}, 17, 18, 19, 20, 21, 22, {23, 24}}};	

	wl::time time = {};
	while(!wl::window_should_close(window)) {
		wl::time_calculations(time);

		camera.position = wl::vec3f(2 * wl::cos(time.since_start), wl::sin(time.since_start / 4), 2 * wl::sin(time.since_start));

		auto frame = wl::throw_if_null(
			wl::begin_drawing(state, wl::color{0.9, .1,  0.2, 1})
		);
		{
			wl::time_upload(frame, time);
			wl::light_upload(frame, lights);
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