#include "waylib.hpp"
#include "window.hpp"
#include "model.hpp"
#include "texture.hpp"

#include "waylib.h" // Here so it won't keep getting auto added at the top!
#include "window.h"
#include "common.h"
#include "common.hpp"
#include "config.h"
#include "texture.h"

#include <glm/geometric.hpp>
#include <iostream>
#include <string_view>

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

int main() {
	auto window = wl::create_window(800, 600, "waylib");
	auto state = wl::create_default_state_from_window(window);
	// Attempt to use mailbox present mode (the default) but fall back to immediate mode if not available (and fifo if nothing is available!)
	if(!wl::window_automatically_reconfigure_surface_on_resize(window, state))
		wl::window_automatically_reconfigure_surface_on_resize(window, state, {.presentation_mode = wgpu::PresentMode::Immediate});

	// Load the shader module
	wl::model model = wl::throw_if_null(wl::load_obj_model("resources/suzane_highpoly.obj", state));
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

	const char* skyplaneShaderSource = R"(
#include <waylib/vertex>
#include <waylib/textures>

@fragment
fn fragment(vert: waylib_output_vertex) -> @location(0) vec4f {
	// Calculate the direction of the pixel
	let clipSpacePos = vec4(vert.position.xy, 1, 1);
	let worldSpacePos = inverse_view_projection_matrix() * clipSpacePos;
	var direction = normalize(worldSpacePos.xyz - camera.settings3D.position);
	direction.z *= -1;

	// Sample the cubemap (direction defines how it should be sampled!)
	return waylib_sample_cubemap(direction);
})";
	wl::model skyplane = wl::throw_if_null(wl::create_fullscreen_quad(state, 
		wl::create_shader(state, skyplaneShaderSource, {.fragment_entry_point = "fragment", .preprocessor = p})
	, p));
	// From: https://learnopengl.com/Advanced-OpenGL/Cubemaps
	auto cubemapPaths = std::array<std::string_view, 6>{
		"../skybox/right.jpg", "../skybox/left.jpg",
		"../skybox/top.jpg", "../skybox/bottom.jpg",
		"../skybox/front.jpg", "../skybox/back.jpg"
	};
	wl::image cubemapImg = wl::throw_if_null(wl::load_images_as_frames(cubemapPaths));
	wl::texture sky = wl::throw_if_null(wl::create_texture_from_image(state, cubemapImg, {.cubemap = true}));
	skyplane.materials[0].textures[(size_t)wl::texture_slot::Cubemap] = &sky;


	wl::frame_time time = {};
	WAYLIB_MAIN_LOOP(!wl::window_should_close(window), {
		wl::time_calculations(time);

		camera.position = wl::vec3f(2 * std::cos(time.since_start), std::sin(time.since_start / 4), 2 * std::sin(time.since_start));

		if(!wl::window_should_redraw(window)) WAYLIB_MAIN_LOOP_CONTINUE;

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

		wl::present_frame(frame);
	});

	wl::release_shader_preprocessor(p);
	wl::release_wgpu_state(state);
	wl::release_window(window);
}