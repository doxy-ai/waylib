R"(
#define WAYLIB_USE_DEFAULT_GEOMETRY_SHADER_AS_ENTRY_POINT
#include <waylib/geometry_shader>
#include <waylib/pbr_data>

fn per_vertex(vertex_id: u32, vertex: waylib_vertex_shader_vertex) -> waylib_vertex_shader_vertex {
	let size = textureDimensions(height_texture);
	let uv = vec2u(
		u32((vertex.uvs.x - f32(u32(vertex.uvs.x))) * f32(size.x)),
		u32((vertex.uvs.y - f32(u32(vertex.uvs.x))) * f32(size.y))
	);
	let displace = (textureLoad(height_texture, uv, 0).r * 2.0 - 1.0) * pbr_data.height_displacement_factor;

	var res = vertex;
	res.position = vertex.position + vertex.normal * displace;
	return res; // Displacement
}
)"