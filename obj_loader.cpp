#include "obj_loader.hpp"

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

//////////////////////////////////////////////////////////////////////
// #Model
//////////////////////////////////////////////////////////////////////

#ifdef WAYLIB_NO_AUTOMATIC_UPLOAD
WAYLIB_OPTIONAL(model) load_obj_model(const char* file_path)
#else
WAYLIB_OPTIONAL(model) load_obj_model(const char* file_path, WAYLIB_OPTIONAL(wgpu_state) state)
#endif
{
	auto path = std::filesystem::absolute(file_path);
	tinyobj::ObjReaderConfig reader_config;
	reader_config.triangulate = true;
	reader_config.vertex_color = true;
	reader_config.mtl_search_path = path.parent_path(); // Path to material files

	tinyobj::ObjReader reader;

	if (!reader.ParseFromFile(path, reader_config)) {
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
}

#ifdef WAYLIB_NAMESPACE_NAME
}
#endif