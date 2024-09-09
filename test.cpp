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

#include <cstring>
#include <glm/geometric.hpp>
#include <iostream>
#include <string_view>

int main() {
	auto window = wl::create_window(800, 600, "waylib"); defer { wl::release_window(window); };
	auto state = wl::create_default_state_from_window(window); defer { wl::release_waylib_state(state); };
	// Attempt to use mailbox present mode (the default) but fall back to immediate mode if not available (and fifo if nothing is available!)
	if(!wl::window_automatically_reconfigure_surface_on_resize(window, state))
		wl::window_automatically_reconfigure_surface_on_resize(window, state, {.presentation_mode = wgpu::PresentMode::Immediate});

	// Load the shader module
	wl::model model = wl::throw_if_null(wl::load_obj_model("resources/suzane_highpoly.obj", state)); defer { wl::release_model(model); };
	wl::shader_preprocessor* p = wl::preprocessor_initialize_virtual_filesystem(wl::create_shader_preprocessor(), state);
	defer { wl::release_shader_preprocessor(p); };
	model.mesh_materials = nullptr;

	// From: https://learnopengl.com/Advanced-OpenGL/Cubemaps
	auto cubemapPaths = std::array<std::string_view, 6>{
		"../resources/skybox/right.jpg", "../resources/skybox/left.jpg",
		"../resources/skybox/top.jpg", "../resources/skybox/bottom.jpg",
		"../resources/skybox/front.jpg", "../resources/skybox/back.jpg"
	};
	wl::image cubemapImg = wl::throw_if_null(wl::load_images_as_frames(cubemapPaths));
	wl::texture sky = wl::throw_if_null(wl::create_texture_from_image(state, cubemapImg, {.cubemap = true}));

	// From: https://ambientcg.com/view?id=Ground068
	std::vector<wl::texture*> textures(WAYLIB_TEXTURE_SLOT_COUNT, nullptr);
	textures[(size_t)wl::texture_slot::Cubemap] = &sky;
	wl::texture dirtCol = wl::throw_if_null(wl::create_texture_from_image(state, wl::load_image("../resources/pbr/Ground068_2K-PNG_Color.png")));
	textures[(size_t)wl::texture_slot::Color] = &dirtCol;
	wl::texture dirtHeight = wl::throw_if_null(wl::create_texture_from_image(state, wl::load_image("../resources/pbr/Ground068_2K-PNG_Displacement.png")));
	textures[(size_t)wl::texture_slot::Height] = &dirtHeight;
	wl::texture dirtNormal = wl::throw_if_null(wl::create_texture_from_image(state, wl::load_image("../resources/pbr/Ground068_2K-PNG_NormalDX.png")));
	textures[(size_t)wl::texture_slot::Normal] = &dirtNormal;
	wl::texture dirtRoughness = wl::throw_if_null(wl::create_texture_from_image(state, wl::load_image("../resources/pbr/Ground068_2K-PNG_Roughness.png")));
	textures[(size_t)wl::texture_slot::Roughness] = &dirtRoughness;
	wl::texture dirtAO = wl::throw_if_null(wl::create_texture_from_image(state, wl::load_image("../resources/pbr/Ground068_2K-PNG_AmbientOcclusion.png")));
	textures[(size_t)wl::texture_slot::AmbientOcclusion] = &dirtAO;

	wl::pbr_material dirt = wl::throw_if_null(wl::create_default_pbr_material(state, textures, {.double_sided = true, .preprocessor = p}));
	dirt.metalness = 0;
	dirt.height_displacement_factor = .05;
	model.materials = &dirt;
	model.material_count = 1;

	// From: https://ambientcg.com/view?id=WoodFloor043
	std::fill(textures.begin(), textures.end(), nullptr);
	textures[(size_t)wl::texture_slot::Cubemap] = &sky;
	wl::texture woodCol = wl::throw_if_null(wl::create_texture_from_image(state, wl::load_image("../resources/pbr/WoodFloor043_2K-PNG_Color.png")));
	textures[(size_t)wl::texture_slot::Color] = &woodCol;
	wl::texture woodNormal = wl::throw_if_null(wl::create_texture_from_image(state, wl::load_image("../resources/pbr/WoodFloor043_2K-PNG_NormalDX.png")));
	textures[(size_t)wl::texture_slot::Normal] = &woodNormal;
	wl::texture woodRoughness = wl::throw_if_null(wl::create_texture_from_image(state, wl::load_image("../resources/pbr/WoodFloor043_2K-PNG_Roughness.png")));
	textures[(size_t)wl::texture_slot::Roughness] = &woodRoughness;
	wl::texture woodMetalness = wl::throw_if_null(wl::create_texture_from_image(state, wl::load_image("../resources/pbr/WoodFloor043_2K-PNG_Metalness.png")));
	textures[(size_t)wl::texture_slot::Metalness] = &woodMetalness;
	wl::texture woodAO = wl::throw_if_null(wl::create_texture_from_image(state, wl::load_image("../resources/pbr/WoodFloor043_2K-PNG_AmbientOcclusion.png")));
	textures[(size_t)wl::texture_slot::AmbientOcclusion] = &woodAO;

	wl::pbr_material wood = wl::throw_if_null(wl::create_default_pbr_material(state, textures, {.preprocessor = p}));
	// model.materials = &wood;
	// model.material_count = 1;
	
	// From: https://ambientcg.com/view?id=Metal041A
	std::fill(textures.begin(), textures.end(), nullptr);
	textures[(size_t)wl::texture_slot::Cubemap] = &sky;
	wl::texture metalCol = wl::throw_if_null(wl::create_texture_from_image(state, wl::load_image("../resources/pbr/Metal041A_2K-PNG_Color.png")));
	textures[(size_t)wl::texture_slot::Color] = &metalCol;
	wl::texture metalNormal = wl::throw_if_null(wl::create_texture_from_image(state, wl::load_image("../resources/pbr/Metal041A_2K-PNG_NormalDX.png")));
	textures[(size_t)wl::texture_slot::Normal] = &metalNormal;
	wl::texture metalRoughness = wl::throw_if_null(wl::create_texture_from_image(state, wl::load_image("../resources/pbr/Metal041A_2K-PNG_Roughness.png")));
	textures[(size_t)wl::texture_slot::Roughness] = &metalRoughness;

	wl::pbr_material metal = wl::throw_if_null(wl::create_default_pbr_material(state, textures, {.preprocessor = p}));
	metal.metalness = 1;
	// model.materials = &metal;
	// model.material_count = 1;

	// From: https://ambientcg.com/view?id=Metal034
	std::fill(textures.begin(), textures.end(), nullptr);
	textures[(size_t)wl::texture_slot::Cubemap] = &sky;
	wl::texture goldCol = wl::throw_if_null(wl::create_texture_from_image(state, wl::load_image("../resources/pbr/Metal034_2K-PNG_Color.png")));
	textures[(size_t)wl::texture_slot::Color] = &goldCol;
	wl::texture goldNormal = wl::throw_if_null(wl::create_texture_from_image(state, wl::load_image("../resources/pbr/Metal034_2K-PNG_NormalDX.png")));
	textures[(size_t)wl::texture_slot::Normal] = &goldNormal;
	wl::texture goldRoughness = wl::throw_if_null(wl::create_texture_from_image(state, wl::load_image("../resources/pbr/Metal034_2K-PNG_Roughness.png")));
	textures[(size_t)wl::texture_slot::Roughness] = &goldRoughness;

	wl::pbr_material gold = wl::throw_if_null(wl::create_default_pbr_material(state, textures, {.preprocessor = p}));
	gold.metalness = 1;
	// model.materials = &gold;
	// model.material_count = 1;

	wl::camera3D camera = {.position={0, 1, -1}, .target={0, 0, 0}};
	std::array<wl::light, 1> lights = {wl::light{
		.type = wl::light_type::Directional,
		.direction = glm::normalize(glm::vec3{1, -1, 1}),
		.color = {.992, .722, .075, 1}, .intensity = 1
	}};

	const char* skyplaneShaderSource = R"(
#include <waylib/vertex>
#include <waylib/textures>

@fragment
fn fragment(vert: waylib_fragment_shader_vertex) -> @location(0) vec4f {
	// Calculate the direction of the pixel
	let clipSpacePos = vec4(vert.position.xy, 1, 1);
	let worldSpacePos = inverse_view_projection_matrix() * clipSpacePos;
	var direction = normalize(worldSpacePos.xyz - camera.settings3D.position);

	// Sample the cubemap (direction defines how it should be sampled!)
	return waylib_sample_cubemap(direction);
})";
	wl::model skyplane = wl::throw_if_null(wl::create_fullscreen_quad(state,
		wl::create_shader(state, skyplaneShaderSource, {.fragment_entry_point = "fragment", .preprocessor = p})
	, p)); defer { wl::release_model(skyplane); };
	skyplane.materials[0].textures[(size_t)wl::texture_slot::Cubemap] = &sky;


	wl::frame_time time = {};
	WAYLIB_MAIN_LOOP(!wl::window_should_close(window), {
	// while(!wl::window_should_close(window)) {
		wl::time_calculations(time);
		// if(!wl::window_should_redraw(window)) WAYLIB_MAIN_LOOP_CONTINUE;

		auto frame = wl::throw_if_null(
			wl::begin_drawing(state, wl::color{0.9, .1,  0.2, 1})
		);
		{
			wl::window_begin_camera_mode3D(frame, window, camera, lights, time);
			{
				wl::model_draw(frame, skyplane);

				wl::model_draw(frame, model);
			}
			// wl::reset_camera_mode(frame);
		}
		wl::end_drawing(frame);

		camera.position = wl::vec3f(2 * wl::cos(time.since_start), wl::sin(time.since_start / 4), 2 * wl::sin(time.since_start));

		wl::present_frame(frame);
	// }
	});
}