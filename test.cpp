#include "waylib.hpp"
#include "window.hpp"
#include "model.hpp"
#include "texture.hpp"

#include "waylib.h" // Here so it won't keep getting auto added at the top!
#include "window.h"
#include "common.h"
#include "config.h"

#include <glm/geometric.hpp>
#include <iostream>

const char* shaderSource = R"(
#include <waylib/time>
#define WAYLIB_USE_DEFAULT_VERTEX_SHADER_AS_ENTRY_POINT
#include <waylib/vertex>
#include <waylib/light>
#include <waylib/textures>

@fragment
fn fragment(vert: waylib_output_vertex) -> @location(0) vec4f {
	let color = waylib_sample_color(vert.texcoords);
	let time = time.delta * 800;
	return vec4f(color.rgb * time, 1);
})";

	const char* skyplaneShaderSource = R"(
#include <waylib/vertex>
#include <waylib/textures>

@fragment
fn fragment(vert: waylib_output_vertex) -> @location(0) vec4f {
	// Calculate the direction of the pixel
	let clipSpacePos = vec4(vert.position.xy, 1, 1);
	let worldSpacePos = inverse_view_projection_matrix() * clipSpacePos;
	let direction = normalize(worldSpacePos.xyz - camera.settings3D.position);

	// Equirectangularly project the direction
	let longitude = degrees(atan2(direction.z, direction.x)) / 360;
	let latitude = (degrees(acos(direction.y)) + 90) / 360;

	// Sample the projection
	return waylib_sample_color(vec2f(longitude, latitude));
}
)";

int main() {
	auto window = wl::create_window(800, 600, "waylib");
	auto state = wl::create_default_device_from_window(window);
	wl::window_automatically_reconfigure_surface_on_resize(window, state);

	// Load the shader module
	wl::model model = wl::throw_if_null(wl::load_obj_model("../suzane.obj", state));
	wl::shader_preprocessor* p = wl::preprocessor_initialize_virtual_filesystem(wl::create_shader_preprocessor(), state);
	wl::shader shader = wl::throw_if_null(wl::create_shader(
		state, shaderSource,
		{.vertex_entry_point = "waylib_default_vertex_shader", .fragment_entry_point = "fragment", .name = "Default Shader", .preprocessor = p}
	));
	wl::material mat = wl::create_material(state, shader);
	// mat.textures[(size_t)wl::texture_slot::Color] = &color;
	model.materials = &mat;
	model.material_count = 1;
	model.mesh_materials = nullptr;

	wl::camera3D camera = {.position={0, 1, -1}, .target={0, 0, 0}};
	std::array<wl::light, 1> lights = {wl::light{
		.type = wl::light_type::Directional,
		.direction = glm::normalize(glm::vec3{-1, -1, 0}),
		.color = {1, 1, 1, 1}, .intensity = 1
	}};

	wl::model skyplane = wl::throw_if_null(wl::create_fullscreen_quad(state, 
		wl::create_shader(state, skyplaneShaderSource, {.fragment_entry_point = "fragment", .preprocessor = p})
	, p));
	// from: https://polyhaven.com/a/symmetrical_garden_02 
	wl::texture sky = wl::throw_if_null(wl::create_texture_from_image(state, wl::load_image("../symmetrical_garden_02_1k.exr")));
	skyplane.materials[0].textures[(size_t)wl::texture_slot::Color] = &sky;


	wl::time time = {};
	while(!wl::window_should_close(window)) {
		wl::time_calculations(time);

		camera.position = wl::vec3f(2 * wl::cos(time.since_start), wl::sin(time.since_start / 4), 2 * wl::sin(time.since_start));

		auto frame = wl::throw_if_null(
			wl::begin_drawing(state, wl::color{0.9, .1,  0.2, 1})
		);
		{
			wl::window_begin_camera_mode3D(frame, window, camera, lights, time);
			{
				wl::model_draw(frame, skyplane);

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