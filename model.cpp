#include "model.hpp"

#ifndef WAYLIB_NO_AUTOMATIC_UPLOAD
#include "waylib.hpp"
#endif

#include <cassert>
#include <filesystem>
#include <glm/ext/matrix_transform.hpp>

#define TINYOBJLOADER_IMPLEMENTATION // define this in only *one* .cc
#define TINYOBJLOADER_USE_MAPBOX_EARCUT
#include "thirdparty/tiny_obj_loader.h"

#ifdef WAYLIB_NAMESPACE_NAME
namespace WAYLIB_NAMESPACE_NAME {
#endif

WAYLIB_OPTIONAL(image) merge_color_and_alpha(const image& color, const image& alpha) {
	if(color.width != alpha.width && color.height != alpha.height
		&& !(alpha.width == 1 && alpha.height == 1)) return {};
	assert(color.frames == 1 && alpha.frames == 1);
	image out = color;
	out.float_data = true;
	out.data32 = new WAYLIB_NAMESPACE_NAME::color[out.width * out.height];
	for(size_t x = 0; x < out.width; ++x)
		for(size_t y = 0; y < out.height; ++y) {
			out.data32[y * out.height + x] = color.data32[y * out.height + x];
			out.data32[y * out.height + x].a = alpha.data32[y * out.height + x].r;
		}
	return out;
}

//////////////////////////////////////////////////////////////////////
// #Misc
//////////////////////////////////////////////////////////////////////

WAYLIB_OPTIONAL(model) create_fullscreen_quad(wgpu_state state, WAYLIB_OPTIONAL(shader) fragment_shader, shader_preprocessor* p) WAYLIB_TRY {
	constexpr const char* vertexShaderSource = R"(
#include <waylib/vertex>

@vertex
fn waylib_fullscreen_vertex_shader(in: waylib_input_vertex, @builtin(instance_index) instance_index: u32, @builtin(vertex_index) vertex_index: u32) -> waylib_output_vertex {
	let tint = instances[instance_index].tint;
	return waylib_output_vertex(
		vec4(in.position, 1),
		in.position,
		in.texcoords,
		in.normal,
		tint * in.color,
		in.tangents,
		in.texcoords2,
#ifdef WAYLIB_VERTEX_SUPPORTS_WIREFRAME
		calculate_barycentric_coordinates(vertex_index)
#endif
	);
})";
	if(!fragment_shader.has_value) return {};

	static mesh mesh = [&state] {
		WAYLIB_NAMESPACE_NAME::mesh mesh = {};
		mesh.heap_allocated = false;
		mesh.vertexCount = 4;
		mesh.triangleCount = 2;
		mesh.positions = new vec3f[4] { vec3f(-1, -1, 0), vec3f(1, -1, 0), vec3f(1, 1, 0), vec3f(-1, 1, 0) };
		mesh.normals = new vec3f[4] { vec3f(0, 0, -1), vec3f(0, 0, -1), vec3f(0, 0, -1), vec3f(0, 0, -1) };
		mesh.texcoords = new vec2f[4] { vec2f(1, 1), vec2f(0, 1), vec2f(0, 0), vec2f(1, 0) };
		mesh.indices = new index_t[6] { 0, 1, 3, 1, 2, 3 };
		mesh_upload(state, mesh);
		return mesh;
	}();

	model model = {};
	model.heap_allocated = false;
	model.mesh_count = 1;
	model.meshes = &mesh;
	
	auto vertexShader = create_shader(state, vertexShaderSource, {.vertex_entry_point = "waylib_fullscreen_vertex_shader", .preprocessor = p});
	if(!vertexShader.has_value) return {};
	shader* shaders = new shader[2]{fragment_shader.value, vertexShader.value};
	shaders[0].heap_allocated = true;
	model.material_count = 1;
	model.materials = new material(create_material(state, shaders, 2, {.depth_function = {}}));
	model.mesh_materials = nullptr;

	return model;
} WAYLIB_CATCH({})

//////////////////////////////////////////////////////////////////////
// #OBJ
//////////////////////////////////////////////////////////////////////

#ifdef WAYLIB_NO_AUTOMATIC_UPLOAD
WAYLIB_OPTIONAL(model) load_obj_model(const char* file_path)
#else
WAYLIB_OPTIONAL(model) load_obj_model(const char* file_path, WAYLIB_OPTIONAL(wgpu_state) state)
#endif
WAYLIB_TRY {
	auto path = std::filesystem::absolute(file_path);
	tinyobj::ObjReaderConfig reader_config;
	reader_config.triangulate = true;
	reader_config.vertex_color = true;
	reader_config.mtl_search_path = path.parent_path().string(); // Path to material files

	tinyobj::ObjReader reader;

	if (!reader.ParseFromFile(path.string(), reader_config)) {
		if (!reader.Error().empty())
			set_error_message(reader.Error());
		return {};
	}

	if (!reader.Warning().empty())
		std::cout << "[TinyObjReader] " << reader.Warning();

	auto& attrib = reader.GetAttrib();
	auto& shapes = reader.GetShapes();
	auto& materials = reader.GetMaterials();

	model out;
	out.get_transform() = glm::identity<glm::mat4x4>();
	out.mesh_count = shapes.size();
	out.meshes = new mesh[shapes.size()];
	// TODO: Materials

	// Loop over shapes
	for (size_t s = 0; s < shapes.size(); s++) {
		// Loop over faces(polygon)
		size_t index_offset = 0;
		out.meshes[s] = {}; // Default initialize the mesh
		out.meshes[s].vertexCount = shapes[s].mesh.num_face_vertices.size() * 3;
		out.meshes[s].triangleCount = out.meshes[s].vertexCount / 3;
		for (size_t f = 0; f < shapes[s].mesh.num_face_vertices.size(); f++) {
			size_t fv = size_t(shapes[s].mesh.num_face_vertices[f]);
			assert(fv == 3); // We only support triangles

			// per-face material
			// shapes[s].mesh.material_ids[f];
			// TODO: Different material faces should go into different meshes

			// Loop over vertices in the face.
			for (size_t v = 0; v < fv; v++) {
				// access to vertex
				tinyobj::index_t idx = shapes[s].mesh.indices[index_offset + v];
				tinyobj::real_t vx = attrib.vertices[3*size_t(idx.vertex_index)+0];
				tinyobj::real_t vy = attrib.vertices[3*size_t(idx.vertex_index)+1];
				tinyobj::real_t vz = attrib.vertices[3*size_t(idx.vertex_index)+2];
				if(!out.meshes[s].positions) out.meshes[s].positions = new vec3f[out.meshes[s].vertexCount];
				out.meshes[s].positions[3 * f + v] = vec3f(vx, vy, vz);

				// Check if `normal_index` is zero or positive. negative = no normal data
				if (idx.normal_index >= 0) {
					tinyobj::real_t nx = attrib.normals[3*size_t(idx.normal_index)+0];
					tinyobj::real_t ny = attrib.normals[3*size_t(idx.normal_index)+1];
					tinyobj::real_t nz = attrib.normals[3*size_t(idx.normal_index)+2];
					if(!out.meshes[s].normals) out.meshes[s].normals = new vec3f[out.meshes[s].vertexCount];
					out.meshes[s].normals[3 * f + v] = vec3f(nx, ny, nz);
				}

				// Check if `texcoord_index` is zero or positive. negative = no texcoord data
				if (idx.texcoord_index >= 0) {
					tinyobj::real_t tx = attrib.texcoords[2*size_t(idx.texcoord_index)+0];
					tinyobj::real_t ty = attrib.texcoords[2*size_t(idx.texcoord_index)+1];
					if(!out.meshes[s].texcoords) out.meshes[s].texcoords = new vec2f[out.meshes[s].vertexCount];
					out.meshes[s].texcoords[3 * f + v] = vec2f(tx, ty);
				}

				// Optional: vertex colors
				tinyobj::real_t red   = attrib.colors[3*size_t(idx.vertex_index)+0];
				tinyobj::real_t green = attrib.colors[3*size_t(idx.vertex_index)+1];
				tinyobj::real_t blue  = attrib.colors[3*size_t(idx.vertex_index)+2];
				if(!out.meshes[s].colors) out.meshes[s].colors = new color[out.meshes[s].vertexCount];
					out.meshes[s].colors[3 * f + v] = color(red, green, blue, 1);
			}
			index_offset += fv;
		}
	}

	out.material_count = 0;
	out.materials = nullptr;
	out.mesh_materials = nullptr;
#ifndef WAYLIB_NO_AUTOMATIC_UPLOAD
	if(state.has_value)
		model_upload(state.value, out);
#endif
	return out;
} WAYLIB_CATCH({})

#ifdef WAYLIB_NAMESPACE_NAME
}
#endif