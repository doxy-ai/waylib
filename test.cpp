#include <iostream>

#include "stylizer/stylizer.hpp"
#include "stylizer/pbr/pbr.hpp"

int main() {
	sl::auto_release window = sl::window::create({800, 600});
	sl::auto_release state = window.create_default_state();
	window.reconfigure_surface_on_resize(state);

	// GBuffer that gets its color format from the window
	sl::auto_release gbuffer = sl::pbr::geometry_buffer::create_default(state, window.get_size(), {{
		.color_format = wgpu::TextureFormat::Undefined
	}});
	window.auto_resize_gbuffer(state, gbuffer);

	// Secondary layer of gbuffer used to combine results
	sl::auto_release presentGbuffer = gbuffer.clone(state);
	window.auto_resize_gbuffer(state, presentGbuffer);
	presentGbuffer.previous = &gbuffer;

	auto p = sl::pbr::initialize_shader_preprocessor(sl::shader_preprocessor{}.initialize(state));
	sl::auto_release shader = sl::shader::from_wgsl(state, R"_(
#define STYLIZER_CAMERA_DATA_IS_3D
#include <stylizer/pbr/gbuffer>
#include <stylizer/pbr/material>

@vertex
fn vertex(in: vertex_input, @builtin(vertex_index) vertex_index: u32, @builtin(instance_index) instance_index: u32) -> vertex_output {
	if false { ensure_gbuffer_layout(); }
	return unpack_vertex_input_full(in, vertex_index, instance_index, instances[instance_index].transform, camera_view_projection_matrix(utility.camera));
}

@fragment
fn fragment(vert: vertex_output) -> fragment_output {
	if false { ensure_gbuffer_fragment_layout(); }
	let mat = build_default_pbr_material(material_settings, vert);
	return fragment_output(
		mat.albedo,
		vec4f(mat.normal, f32(material_settings.material_id)),
		packed_from_pbr_material(mat, 0),
	);
}
	)_", {.vertex_entry_point = "vertex", .fragment_entry_point = "fragment", .preprocessor = &p});

	sl::auto_release albedoTexture = std::move(sl::img::load("../resources/pbr/Ground068_2K-PNG_Color.png")
		.upload(state, {.sampler_type = sl::texture_create_sampler_type::Trilinear}, {"Albedo Texture"})
		.generate_mipmaps(state));
	// sl::auto_release normalTexture = std::move(sl::img::load("../resources/pbr/Ground068_2K-PNG_NormalDX.png")
	// 	.upload(state, {.sampler_type = sl::texture_create_sampler_type::Trilinear}, {"Normal Texture"})
	// 	.generate_mipmaps(state));
	sl::auto_release packedTexture = std::move(sl::img::load("../resources/pbr/Ground068_2K-PNG_Packed.png")
		.upload(state, {.sampler_type = sl::texture_create_sampler_type::Trilinear}, {"Packed Texture"})
		.generate_mipmaps(state));

	sl::auto_release model = sl::obj::load("../resources/suzane_highpoly.obj");
	model.meshes()[0].indices = nullptr;
	model.upload(state, gbuffer);
	sl::auto_release material = sl::pbr::material({sl::materialC{
		.shader_count = 1,
		.shaders = &shader,
	}});
	material.albedo_texture = &albedoTexture;
	// material.normal_texture = &normalTexture; material.normal_strength = 1;
	material.packed_texture = &packedTexture;
	material.material_id = 1;
	material.upload(state, gbuffer, {}, {.double_sided = true});
	model.c().material_count = 1;
	model.c().materials = &material;
	model.c().mesh_materials = nullptr;
	model.transform = glm::identity<glm::mat4x4>();

	// From: https://learnopengl.com/Advanced-OpenGL/Cubemaps

	sl::auto_release skyTexture = sl::img::load_frames(std::array<std::filesystem::path, 6>{
		"../resources/skybox/right.jpg", "../resources/skybox/left.jpg",
		"../resources/skybox/top.jpg", "../resources/skybox/bottom.jpg",
		"../resources/skybox/front.jpg", "../resources/skybox/back.jpg"
	}).upload_frames_as_cube(state, {.sampler_type = sl::texture_create_sampler_type::Trilinear}, {"Sky Cubemap"});
	sl::auto_release skyShader = sl::shader::from_wgsl(state, R"_(
#define STYLIZER_CAMERA_DATA_IS_3D
#include <stylizer/pbr/gbuffer>
#include <stylizer/inverse>

@group(2) @binding(0) var sky_texture: texture_cube<f32>;
@group(2) @binding(1) var sky_sampler: sampler;

@vertex
fn vertex(in: vertex_input, @builtin(vertex_index) vertex_index: u32, @builtin(instance_index) instance_index: u32) -> vertex_output {
	if false { ensure_gbuffer_layout(); }
	return unpack_vertex_input(in, vertex_index, instance_index);
}

@fragment
fn fragment(vert: vertex_output) -> fragment_output {
	if false { ensure_gbuffer_fragment_layout(); }

	// Calculate the direction of the pixel
	let clipSpacePos = vec4(vert.position.xy, 1, 1);
	let V = inverse_view_matrix(utility.camera.view_matrix);
	let P = inverse4x4(utility.camera.projection_matrix);
	let worldSpacePos = V * P * clipSpacePos;
	let direction = normalize(worldSpacePos.xyz - utility.camera.position);

	return fragment_output(
		textureSample(sky_texture, sky_sampler, direction),
		vec4f(0),
		vec4f(0),
	);
}
	)_", {.vertex_entry_point = "vertex", .fragment_entry_point = "fragment", .preprocessor = &p});
	sl::auto_release<sl::material> skyMat = {sl::materialC{
		.texture_count = 1,
		.textures = &skyTexture,
		.shader_count = 1,
		.shaders = &skyShader,
	}};
	skyMat.upload(state, gbuffer, {}, {.depth_function = {}});
	sl::auto_release<sl::model> sky = sl::model::fullscreen_mesh(state, skyMat);

	sl::auto_release combineShader = sl::shader::from_wgsl(state, R"_(
#define STYLIZER_CAMERA_DATA_IS_3D
#include <stylizer/pbr/gbuffer>
#include <stylizer/pbr/combiners>

@vertex
fn vertex(in: vertex_input, @builtin(vertex_index) vertex_index: u32, @builtin(instance_index) instance_index: u32) -> vertex_output {
	if false { ensure_gbuffer_layout(); }
	return unpack_vertex_input(in, vertex_index, instance_index);
}

const light = light_data(
	0,
	vec3(0),
	normalize(vec3f(1, -1, 1)),
	vec4f(.992, .937, .914, 1),
	1,
	0, 0, 0, 0, 0
);

@fragment
fn fragment(vert: vertex_output) -> fragment_output_with_depth {
	if false { ensure_gbuffer_fragment_layout(); }

	var uv = vert.position.xy * .5 + .5;
	uv.y = 1 - uv.y;

	let color = textureSample(previous_color_texture, previous_color_sampler, uv);
	let depth = textureSample(previous_depth_texture, previous_depth_sampler, uv);
	let normal_and_id = textureSample(previous_normal_texture, previous_normal_sampler, uv);
	let packed = textureSample(previous_packed_texture, previous_packed_sampler, uv);

	let mat = pbr_material(
		color,
		vec4f(0),
		normal_and_id.xyz,
		vec3f(0),
		packed.r,
		packed.g,
		packed.b
	);

	var out = color.rgb;
	if u32(normal_and_id.a) > 0 { // Don't apply shading to the skybox (matID = 0 in previous shaders)
		out = pbr_metalrough(light, mat, vert).rgb + pbr_simple_ambient(light, mat, vert).rgb;
	}

	return fragment_output_with_depth(
		vec4(out, 1),
		depth,
		normal_and_id,
		packed,
	);
}
	)_", {.vertex_entry_point = "vertex", .fragment_entry_point = "fragment", .preprocessor = &p});
	sl::auto_release<sl::material> combineMat = {sl::materialC{
		.shader_count = 1,
		.shaders = &combineShader,
	}};
	combineMat.upload(state, gbuffer, {}, {.depth_function = {}});
	sl::auto_release combiner = sl::model::fullscreen_mesh(state, combineMat);

	sl::auto_release<sl::gpu_buffer> utility_buffer;
	sl::time time{};
	sl::camera3D camera = sl::camera3DC{.position = {0, 1, -1}, .target_position = sl::vec3f(0)};

	STYLIZER_MAIN_LOOP(!window.should_close(),
	// while(!window.should_close()) {
		utility_buffer = time.calculate().update_utility_buffer(state, utility_buffer);

		camera.position = sl::vec3f(2 * cos(time.since_start), sin(time.since_start / 4), 2 * sin(time.since_start));
		utility_buffer = camera.calculate_matrices(window.get_size()).update_utility_buffer(state, utility_buffer);

		sl::auto_release draw = gbuffer.begin_drawing(state, {{.1, .2, .7, 1}}, utility_buffer);
		{
			sky.draw(draw);

			model.draw(draw);
		}
		draw.draw();

		draw = presentGbuffer.begin_drawing(state, {}, utility_buffer);
			combiner.draw(draw);
		draw.draw();

		// Present gbuffer's color
		window.present(state, presentGbuffer.color());
	// }
	);
}