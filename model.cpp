#include "model.hpp"

#include <glm/ext/matrix_transform.hpp>

#include <assimp/Importer.hpp>		// C++ importer interface
#include <assimp/scene.h>			// Output data structure
#include <assimp/postprocess.h>		// Post processing flags

#ifdef WAYLIB_NAMESPACE_NAME
namespace WAYLIB_NAMESPACE_NAME {
#endif

namespace detail {
	inline vec3f fromAssimp(aiVector3D vec) {
		return vec3f{vec.x, vec.y, vec.z};
	}

	inline color8bit fromAssimp(aiColor4D col) {
		uint8_t r = std::round(col.r * 255);
		uint8_t g = std::round(col.g * 255);
		uint8_t b = std::round(col.b * 255);
		uint8_t a = std::round(col.a * 255);
		return color8bit{r, g, b, a};
	}

	mesh fromAssimp(const aiMesh* aiMesh) {
		assert(aiMesh != nullptr);
		mesh m;

		m.vertexCount = aiMesh->mNumVertices;
		if(aiMesh->HasPositions()) m.positions = new vec3f[m.vertexCount];
		if(aiMesh->HasNormals()) m.normals = new vec3f[m.vertexCount];
		if(aiMesh->HasTangentsAndBitangents()) m.tangents = new vec4f[m.vertexCount];
		if(aiMesh->HasTextureCoords(0)) m.texcoords = new vec2f[m.vertexCount];
		if(aiMesh->HasTextureCoords(1)) m.texcoords2 = new vec2f[m.vertexCount];
		if(aiMesh->HasVertexColors(0)) m.colors = new color8bit[m.vertexCount];

		for (unsigned int j = 0; j < aiMesh->mNumVertices; ++j) {
			if(m.positions) m.positions[j] = fromAssimp(aiMesh->mVertices[j]);
			if(m.normals) m.normals[j] = fromAssimp(aiMesh->mNormals[j]);
			if(m.tangents) m.tangents[j] = vec4f(fromAssimp(aiMesh->mTangents[j]), 0);
			if(m.texcoords) m.texcoords[j] = fromAssimp(aiMesh->mTextureCoords[0][j]).xy();
			if(m.texcoords2) m.texcoords2[j] = fromAssimp(aiMesh->mTextureCoords[1][j]).xy();
			if(m.colors) m.colors[j] = fromAssimp(aiMesh->mColors[0][j]);
		}

		// Indices (if present)
		if (aiMesh->HasFaces()) {
			m.triangleCount = aiMesh->mNumFaces;
			m.indices = new index_t[m.triangleCount * 3]; // Assuming triangles, adjust as necessary

			for (unsigned int j = 0; j < aiMesh->mNumFaces; ++j) {
				const aiFace& face = aiMesh->mFaces[j];
				// Assuming triangles
				m.indices[j * 3 + 0] = face.mIndices[0];
				m.indices[j * 3 + 1] = face.mIndices[1];
				m.indices[j * 3 + 2] = face.mIndices[2];
			}
		}

		return m;
	}

	model fromAssimp(const aiScene* scene) {
		assert(scene != nullptr);
		model resultModel;

		resultModel.get_transform() = glm::identity<glm::mat4>(); // TODO: Initialize with identity

		resultModel.mesh_count = scene->mNumMeshes;
		resultModel.meshes = new mesh[resultModel.mesh_count];
		for (unsigned int i = 0; i < scene->mNumMeshes; ++i)
			resultModel.meshes[i] = fromAssimp(scene->mMeshes[i]);

		resultModel.material_count = 0;
		resultModel.materials = nullptr;
		resultModel.mesh_materials = nullptr;
		// TODO: Populate materials

		resultModel.bone_count = 0;
		resultModel.bones = nullptr;
		// TODO: Populate bones

		return resultModel;
	}

	unsigned int calculateFlags(model_process_configuration config) {
		unsigned int flags = aiProcess_ConvertToLeftHanded |
			aiProcess_LimitBoneWeights | // Only 4 bones!
			aiProcess_RemoveRedundantMaterials |
			aiProcess_PopulateArmatureData |
			aiProcess_SortByPType |
			aiProcess_GenUVCoords |
			aiProcess_OptimizeGraph;
		if(config.triangulate) flags |= aiProcess_Triangulate;
		if(config.join_identical_vertecies) flags |= aiProcess_JoinIdenticalVertices;
		if(config.generate_normals_if_not_present) flags |= aiProcess_GenNormals;
		if(config.smooth_generated_normals && config.generate_normals_if_not_present) flags |= aiProcess_GenSmoothNormals;
		if(config.calculate_tangets_from_normals) flags |= aiProcess_CalcTangentSpace;
		if(config.remove_normals) flags |= aiProcess_DropNormals;
		if(config.split_large_meshes) flags |= aiProcess_SplitLargeMeshes;
		if(config.split_large_bone_count) flags |= aiProcess_SplitByBoneCount;
		if(config.fix_infacing_normals) flags |= aiProcess_FixInfacingNormals;
		if(config.fix_invalid_data) flags |= aiProcess_FindInvalidData;
		if(config.fix_invalid_texture_paths) flags |= aiProcess_EmbedTextures;
		if(config.validate_model) flags |= aiProcess_ValidateDataStructure;
		if(config.optimize.vertex_cache_locality) flags |= aiProcess_ImproveCacheLocality;
		if(config.optimize.find_instances) flags |= aiProcess_FindInstances;
		if(config.optimize.meshes) flags |= aiProcess_OptimizeMeshes;
		return flags;
	}
}



model_process_configuration default_model_process_configuration() {
	return {}; // TODO: Expand
}

WAYLIB_OPTIONAL(model) load_model(const char* file_path, model_process_configuration config) {
	// Create an instance of the Importer class
	Assimp::Importer importer;

	const aiScene* scene = importer.ReadFile(file_path, detail::calculateFlags(config));
	if (scene == nullptr) {
		std::cerr << "Error: " << importer.GetErrorString() << std::endl;
		return {};
	}

	return detail::fromAssimp(scene);
}

WAYLIB_OPTIONAL(model) load_model_from_memory(const unsigned char * data, size_t size, model_process_configuration config) {
	// Create an instance of the Importer class
	Assimp::Importer importer;

	const aiScene* scene = importer.ReadFileFromMemory(data, size, detail::calculateFlags(config));
	if (scene == nullptr) {
		std::cerr << "Error: " << importer.GetErrorString() << std::endl;
		return {};
	}

	return detail::fromAssimp(scene);
}
WAYLIB_OPTIONAL(model) load_model_from_memory(std::span<std::byte> data, model_process_configuration config) {
	return load_model_from_memory((unsigned char*)data.data(), data.size(), config);
}

// WAYLIB_OPTIONAL(model) load_model(std::string_view file_path, model_process_configuration config /*= {}*/) {
// 	return load_model((const char*)file_path.data(), config);
// }

#ifdef WAYLIB_NAMESPACE_NAME
}
#endif