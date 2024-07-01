#include "model.hpp"
#include "waylib.h"

#include <glm/ext/matrix_transform.hpp>

#include <assimp/Importer.hpp>		// C++ importer interface
#include <assimp/scene.h>			// Output data structure
#include <assimp/postprocess.h>		// Post processing flags
#include <limits>

#ifdef WAYLIB_NAMESPACE_NAME
namespace WAYLIB_NAMESPACE_NAME {
#endif

namespace detail {
	inline vec3f fromAssimp(aiVector3D vec) {
		return vec3f{vec.x, vec.y, vec.z};
	}

	inline color fromAssimp(aiColor4D col) {
		return color{col.r, col.g, col.b, col.a};
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
		if(aiMesh->HasVertexColors(0)) m.colors = new color[m.vertexCount];

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
				assert(face.mNumIndices == 3);
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

	template<typename T>
	wgpu::IndexFormat calculate_index_format() {
		static_assert(sizeof(T) == 2 || sizeof(T) == 4, "Index types must either be u16 or u32 convertable.");

		if constexpr(sizeof(T) == 2)
			return wgpu::IndexFormat::Uint16;
		else return wgpu::IndexFormat::Uint32;
	}

	wgpu::TextureFormat surface_prefered_format(webgpu_state state) {
		wgpu::SurfaceCapabilities capabilities;
		state.surface.getCapabilities(state.device.getAdapter(), &capabilities); // TODO: Always returns error?
		return capabilities.formats[0];
	}

	std::pair<wgpu::RenderPipelineDescriptor, WAYLIB_OPTIONAL(wgpu::FragmentState)> shader_configure_render_pipeline_descriptor(webgpu_state state, shader shader, wgpu::RenderPipelineDescriptor pipelineDesc = {}) {
		static WGPUBlendState blendState = {
			.color = {
				.operation = wgpu::BlendOperation::Add,
				.srcFactor = wgpu::BlendFactor::SrcAlpha,
				.dstFactor = wgpu::BlendFactor::OneMinusSrcAlpha,
			},
			.alpha = {
				.operation = wgpu::BlendOperation::Add,
				.srcFactor = wgpu::BlendFactor::One,
				.dstFactor = wgpu::BlendFactor::Zero,
			},
		};
		thread_local static WGPUColorTargetState colorTarget = {
			.nextInChain = nullptr,
			.format = surface_prefered_format(state),
			.blend = &blendState,
			.writeMask = wgpu::ColorWriteMask::All,
		};

		static WGPUVertexAttribute positionAttribute = {
			.format = wgpu::VertexFormat::Float32x3,
			.offset = 0,
			.shaderLocation = 0,
		};
		static WGPUVertexBufferLayout positionLayout = {
			.arrayStride = sizeof(vec3f),
			.stepMode = wgpu::VertexStepMode::Vertex,
			.attributeCount = 1,
			.attributes = &positionAttribute,
		};

		static WGPUVertexAttribute texcoordAttribute = {
			.format = wgpu::VertexFormat::Float32x2,
			.offset = 0,
			.shaderLocation = 1,
		};
		static WGPUVertexBufferLayout texcoordLayout = {
			.arrayStride = sizeof(vec2f),
			.stepMode = wgpu::VertexStepMode::Vertex,
			.attributeCount = 1,
			.attributes = &texcoordAttribute,
		};

		static WGPUVertexAttribute normalAttribute = {
			.format = wgpu::VertexFormat::Float32x3,
			.offset = 0,
			.shaderLocation = 2,
		};
		static WGPUVertexBufferLayout normalLayout = {
			.arrayStride = sizeof(vec3f),
			.stepMode = wgpu::VertexStepMode::Vertex,
			.attributeCount = 1,
			.attributes = &normalAttribute,
		};

		static WGPUVertexAttribute colorAttribute = {
			.format = wgpu::VertexFormat::Float32x4,
			.offset = 0,
			.shaderLocation = 3,
		};
		static WGPUVertexBufferLayout colorLayout = {
			.arrayStride = sizeof(color),
			.stepMode = wgpu::VertexStepMode::Vertex,
			.attributeCount = 1,
			.attributes = &colorAttribute,
		};

		static WGPUVertexAttribute tangentAttribute = {
			.format = wgpu::VertexFormat::Float32x4,
			.offset = 0,
			.shaderLocation = 4,
		};
		static WGPUVertexBufferLayout tangentLayout = {
			.arrayStride = sizeof(vec4f),
			.stepMode = wgpu::VertexStepMode::Vertex,
			.attributeCount = 1,
			.attributes = &tangentAttribute,
		};

		static WGPUVertexAttribute texcoord2Attribute = {
			.format = wgpu::VertexFormat::Float32x2,
			.offset = 0,
			.shaderLocation = 5,
		};
		static WGPUVertexBufferLayout texcoord2Layout = {
			.arrayStride = sizeof(vec2f),
			.stepMode = wgpu::VertexStepMode::Vertex,
			.attributeCount = 1,
			.attributes = &texcoord2Attribute,
		};
		static std::array<WGPUVertexBufferLayout, 6> buffers = {positionLayout, texcoordLayout, normalLayout, colorLayout, tangentLayout, texcoord2Layout};

		if(shader.vertex_entry_point) {
			pipelineDesc.vertex.bufferCount = buffers.size();
			pipelineDesc.vertex.buffers = buffers.data();
			pipelineDesc.vertex.constantCount = 0;
			pipelineDesc.vertex.constants = nullptr;
			pipelineDesc.vertex.entryPoint = shader.vertex_entry_point;
			pipelineDesc.vertex.module = shader.module;
		}

		if(shader.fragment_entry_point) {
			wgpu::FragmentState fragment;
			fragment.constantCount = 0;
			fragment.constants = nullptr;
			fragment.entryPoint = shader.fragment_entry_point;
			fragment.module = shader.module;
			fragment.targetCount = 1;
			fragment.targets = &colorTarget;
			pipelineDesc.fragment = &fragment;
			return {pipelineDesc, fragment};
		}
		return {pipelineDesc, {}};
	}
}


//////////////////////////////////////////////////////////////////////
// #Mesh
//////////////////////////////////////////////////////////////////////

void mesh_upload(webgpu_state state, mesh& mesh) {
	size_t biggest = std::max(mesh.vertexCount * sizeof(vec4f), mesh.triangleCount * sizeof(index_t) * 3);
	std::vector<std::byte> zeroBuffer(biggest, std::byte{});

	wgpu::BufferDescriptor bufferDesc;
	bufferDesc.label = "Waylib Vertex Buffer";
	bufferDesc.size = mesh.vertexCount * sizeof(vec2f) * 2
		+ mesh.vertexCount * sizeof(vec3f) * 2
		+ mesh.vertexCount * sizeof(vec4f) * 1
		+ mesh.vertexCount * sizeof(color) * 1;
	bufferDesc.usage = wgpu::BufferUsage::CopyDst | wgpu::BufferUsage::Vertex; // Vertex usage here!
	bufferDesc.mappedAtCreation = false;
	if(mesh.buffer) mesh.buffer.release();
	mesh.buffer = state.device.createBuffer(bufferDesc);

	// Upload geometry data to the buffer
	size_t currentOffset = 0;
	wgpu::Queue queue = state.device.getQueue();
	{ // Position
		void* data = mesh.positions ? (void*)mesh.positions : (void*)zeroBuffer.data();
		queue.writeBuffer(mesh.buffer, currentOffset, data, mesh.vertexCount * sizeof(vec3f));
		currentOffset += mesh.vertexCount * sizeof(vec3f);
	}
	{ // Texcoords
		void* data = mesh.texcoords ? (void*)mesh.texcoords : (void*)zeroBuffer.data();
		queue.writeBuffer(mesh.buffer, currentOffset, data, mesh.vertexCount * sizeof(vec2f));
		currentOffset += mesh.vertexCount * sizeof(vec2f);
	}
	{ // Texcoords2
		void* data = mesh.texcoords2 ? (void*)mesh.texcoords2 : (void*)zeroBuffer.data();
		queue.writeBuffer(mesh.buffer, currentOffset, data, mesh.vertexCount * sizeof(vec2f));
		currentOffset += mesh.vertexCount * sizeof(vec2f);
	}
	{ // Normals
		void* data = mesh.normals ? (void*)mesh.normals : (void*)zeroBuffer.data();
		queue.writeBuffer(mesh.buffer, currentOffset, data, mesh.vertexCount * sizeof(vec3f));
		currentOffset += mesh.vertexCount * sizeof(vec3f);
	}
	{ // Tangents
		void* data = mesh.tangents ? (void*)mesh.tangents : (void*)zeroBuffer.data();
		queue.writeBuffer(mesh.buffer, currentOffset, data, mesh.vertexCount * sizeof(vec4f));
		currentOffset += mesh.vertexCount * sizeof(vec4f);
	}
	{ // Colors
		void* data = mesh.colors ? (void*)mesh.colors : (void*)zeroBuffer.data();
		queue.writeBuffer(mesh.buffer, currentOffset, data, mesh.vertexCount * sizeof(color));
		currentOffset += mesh.vertexCount * sizeof(color);
	}
	if(mesh.indices) {
		wgpu::BufferDescriptor bufferDesc;
		bufferDesc.label = "Vertex Position Buffer";
		bufferDesc.size = mesh.triangleCount * sizeof(index_t) * 3;
		bufferDesc.usage = wgpu::BufferUsage::CopyDst | wgpu::BufferUsage::Index;
		bufferDesc.mappedAtCreation = false;
		if(mesh.indexBuffer) mesh.indexBuffer.release();
		mesh.indexBuffer = state.device.createBuffer(bufferDesc);

		queue.writeBuffer(mesh.indexBuffer, 0, mesh.indices, mesh.triangleCount * sizeof(index_t) * 3);
	} else if(mesh.indexBuffer) mesh.indexBuffer.release();
}
void mesh_upload(webgpu_state state, mesh* mesh) {
	mesh_upload(state, *mesh);
}

//////////////////////////////////////////////////////////////////////
// #Material
//////////////////////////////////////////////////////////////////////

pipeline_globals& create_pipeline_globals(webgpu_state state); // Declared in waylib.cpp

void material_upload(webgpu_state state, material& material, material_configuration config /*= {}*/) {
	// Create the render pipeline
	wgpu::RenderPipelineDescriptor pipelineDesc;
	wgpu::FragmentState fragment;
	for(size_t i = material.shaderCount; i--; ) { // Reverse itteration ensures that lower indexed shaders take precedence
		WAYLIB_OPTIONAL(wgpu::FragmentState) fragmentOpt;
		std::tie(pipelineDesc, fragmentOpt) = detail::shader_configure_render_pipeline_descriptor(state, material.shaders[i], pipelineDesc);
		if(fragmentOpt.has_value) {
			fragment = fragmentOpt.value;
			pipelineDesc.fragment = &fragment;
		}
	}

	// Each sequence of 3 vertices is considered as a triangle
	pipelineDesc.primitive.topology = wgpu::PrimitiveTopology::TriangleList;

	// We'll see later how to specify the order in which vertices should be
	// connected. When not specified, vertices are considered sequentially.
	pipelineDesc.primitive.stripIndexFormat = wgpu::IndexFormat::Undefined;

	// The face orientation is defined by assuming that when looking
	// from the front of the face, its corner vertices are enumerated
	// in the counter-clockwise (CCW) order.
	pipelineDesc.primitive.frontFace = wgpu::FrontFace::CCW;

	// But the face orientation does not matter much because we do not
	// cull (i.e. "hide") the faces pointing away from us (which is often
	// used for optimization).
	pipelineDesc.primitive.cullMode = wgpu::CullMode::None; // = wgpu::CullMode::Back;

	// We setup a depth buffer state for the render pipeline
	wgpu::DepthStencilState depthStencilState = Default;
	// Keep a fragment only if its depth is lower than the previously blended one
	depthStencilState.depthCompare = config.depth_function.has_value ? config.depth_function.value : wgpu::CompareFunction::Undefined;
	// Each time a fragment is blended into the target, we update the value of the Z-buffer
	depthStencilState.depthWriteEnabled = config.depth_function.has_value;
	// Store the format in a variable as later parts of the code depend on it
	depthStencilState.format = depth_texture_format;
	// Deactivate the stencil alltogether
	depthStencilState.stencilReadMask = 0;
	depthStencilState.stencilWriteMask = 0;
	pipelineDesc.depthStencil = &depthStencilState;

	// Samples per pixel
	pipelineDesc.multisample.count = 1;
	// Default value for the mask, meaning "all bits on"
	pipelineDesc.multisample.mask = ~0u;
	// Default value as well (irrelevant for count = 1 anyways)
	pipelineDesc.multisample.alphaToCoverageEnabled = false;
	// Associate with the global layout
	pipelineDesc.layout = create_pipeline_globals(state).layout;

	if(material.pipeline) material.pipeline.release();
	material.pipeline = state.device.createRenderPipeline(pipelineDesc);
}
void material_upload(webgpu_state state, material* material, material_configuration config /*= {}*/) {
	material_upload(state, *material, config);
}

material create_material(webgpu_state state, shader* shaders, size_t shader_count, material_configuration config /*= {}*/) {
	material out {.shaderCount = (index_t)shader_count, .shaders = shaders};
	material_upload(state, out, config);
	return out;
}
material create_material(webgpu_state state, std::span<shader> shaders, material_configuration config /*= {}*/) {
	return create_material(state, shaders.data(), shaders.size(), config);
}
material create_material(webgpu_state state, shader& shader, material_configuration config /*= {}*/) {
	return create_material(state, &shader, 1, config);
}

//////////////////////////////////////////////////////////////////////
// #Model
//////////////////////////////////////////////////////////////////////

model_process_configuration default_model_process_configuration() {
	return {}; // TODO: Expand
}

void model_upload(wl::webgpu_state state, wl::model& model) {
	for(size_t i = 0; i < model.mesh_count; ++i)
		wl::mesh_upload(state, model.meshes[i]);
	for(size_t i = 0; i < model.material_count; ++i)
		wl::material_upload(state, model.materials[i]);
}
void model_upload(wl::webgpu_state state, wl::model* model) {
	model_upload(state, *model);
}


WAYLIB_OPTIONAL(model) load_model(webgpu_state state, const char* file_path, model_process_configuration config) {
	// Create an instance of the Importer class
	Assimp::Importer importer;
	importer.SetPropertyInteger(AI_CONFIG_PP_SLM_VERTEX_LIMIT, std::numeric_limits<index_t>::max());
	
	const aiScene* scene = importer.ReadFile(file_path, detail::calculateFlags(config));
	if (scene == nullptr) {
		set_error_message_raw(importer.GetErrorString());
		return {};
	}

	model out = detail::fromAssimp(scene);
	model_upload(state, out);
	return out;
}

WAYLIB_OPTIONAL(model) load_model_from_memory(webgpu_state state, const unsigned char * data, size_t size, model_process_configuration config) {
	// Create an instance of the Importer class
	Assimp::Importer importer;

	const aiScene* scene = importer.ReadFileFromMemory(data, size, detail::calculateFlags(config));
	if (scene == nullptr) {
		set_error_message_raw(importer.GetErrorString());
		return {};
	}

	model out = detail::fromAssimp(scene);
	model_upload(state, out);
	return out;
}
WAYLIB_OPTIONAL(model) load_model_from_memory(webgpu_state state, std::span<std::byte> data, model_process_configuration config) {
	return load_model_from_memory(state, (unsigned char*)data.data(), data.size(), config);
}

void model_draw_instanced(webgpu_frame_state& frame, model& model, std::span<model_instance_data> instances) {
	static WGPUBufferDescriptor bufferDesc = {
		.label = "Waylib Instance Buffer",
		.usage = wgpu::BufferUsage::CopyDst | wgpu::BufferUsage::Storage,
		.size = sizeof(model_instance_data),
		.mappedAtCreation = false,
	};
	static wgpu::Buffer instanceBuffer;
	static wgpu::BindGroup bindGroup;

	if(instances.size() > 0) {
#ifndef WAYLIB_NO_CAMERAS
		for(auto& data: instances)
			data.get_transform() = frame.get_current_view_projection_matrix() * data.get_transform();
#endif // WAYLIB_NO_CAMERAS

		if(!instanceBuffer || instanceBuffer.getSize() < sizeof(model_instance_data) * instances.size()) {
			if(instanceBuffer) instanceBuffer.release();
			bufferDesc.size = sizeof(model_instance_data) * instances.size();
			instanceBuffer = frame.state.device.createBuffer(bufferDesc);
		}
		frame.state.device.getQueue().writeBuffer(instanceBuffer, 0, instances.data(), sizeof(model_instance_data) * instances.size());

		std::array<WGPUBindGroupEntry, 1> bindings = {WGPUBindGroupEntry{
			.binding = 0,
			.buffer = instanceBuffer,
			.offset = 0,
			.size = sizeof(model_instance_data) * instances.size(),
		}};
		wgpu::BindGroupDescriptor bindGroupDesc = wgpu::Default;
		bindGroupDesc.layout = create_pipeline_globals(frame.state).bindGroupLayouts[0]; // Group 0 is instance data
		bindGroupDesc.entryCount = bindings.size();
		bindGroupDesc.entries = bindings.data();
		
		if(bindGroup) bindGroup.release();
		bindGroup = frame.state.device.createBindGroup(bindGroupDesc);
		frame.render_pass.setBindGroup(0, bindGroup, 0, nullptr);
	}

	for(size_t i = 0; i < model.mesh_count; ++i) {
		// Select which render pipeline to use
		frame.render_pass.setPipeline(model.materials[model.mesh_materials[i]].pipeline);

		size_t currentOffset = 0;
		auto& mesh = model.meshes[i];
		{ // Position
			frame.render_pass.setVertexBuffer(0, mesh.buffer, currentOffset, mesh.vertexCount * sizeof(vec3f));
			currentOffset += mesh.vertexCount * sizeof(vec3f);
		}
		{ // Texcoord
			frame.render_pass.setVertexBuffer(1, mesh.buffer, currentOffset, mesh.vertexCount * sizeof(vec2f));
			currentOffset += mesh.vertexCount * sizeof(vec2f);
		}
		{ // Texcoord 2
			frame.render_pass.setVertexBuffer(5, mesh.buffer, currentOffset, mesh.vertexCount * sizeof(vec2f));
			currentOffset += mesh.vertexCount * sizeof(vec2f);
		}
		{ // Normals
			frame.render_pass.setVertexBuffer(2, mesh.buffer, currentOffset, mesh.vertexCount * sizeof(vec3f));
			currentOffset += mesh.vertexCount * sizeof(vec3f);
		}
		{ // Tangents
			frame.render_pass.setVertexBuffer(4, mesh.buffer, currentOffset, mesh.vertexCount * sizeof(vec4f));
			currentOffset += mesh.vertexCount * sizeof(vec4f);
		}
		{ // Colors
			frame.render_pass.setVertexBuffer(3, mesh.buffer, currentOffset, mesh.vertexCount * sizeof(color));
			currentOffset += mesh.vertexCount * sizeof(color);
		}

		if(mesh.indexBuffer) {
			frame.render_pass.setIndexBuffer(mesh.indexBuffer, detail::calculate_index_format<index_t>(), 0, mesh.indexBuffer.getSize());
			frame.render_pass.drawIndexed(mesh.triangleCount * 3, std::max<size_t>(instances.size(), 1), 0, 0, 0);
		} else
			frame.render_pass.draw(model.meshes[i].vertexCount, std::max<size_t>(instances.size(), 1), 0, 0);
	}
}
void model_draw_instanced(webgpu_frame_state* frame, model* model, model_instance_data* instances, size_t instance_count) {
	model_draw_instanced(*frame, *model, {instances, instance_count});
}

void model_draw(webgpu_frame_state& frame, model& model) {
	model_instance_data instance = {model.transform, {1, 1, 1, 1}};
	model_draw_instanced(frame, model, {&instance, 1});
}
void model_draw(webgpu_frame_state* frame, model* model) {
	model_draw(*frame, *model);
}

#ifdef WAYLIB_NAMESPACE_NAME
}
#endif