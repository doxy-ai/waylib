#define WEBGPU_CPP_IMPLEMENTATION
#include "common.hpp"

#include <string>
#include <algorithm>

#ifdef WAYLIB_NAMESPACE_NAME
namespace WAYLIB_NAMESPACE_NAME {
#endif

//////////////////////////////////////////////////////////////////////
// #Pipeline Globals
//////////////////////////////////////////////////////////////////////

pipeline_globals& create_pipeline_globals(wgpu_state state) {
	static pipeline_globals global;
	if(global.created) return global;

	// Create binding layout (don't forget to = Default)
	std::array<wgpu::BindGroupLayoutEntry, 4> bufferBindingLayouts; bufferBindingLayouts.fill(wgpu::Default);
	std::array<wgpu::BindGroupLayoutEntry, 16> textureBindingLayouts; textureBindingLayouts.fill(wgpu::Default);
	// G0 B0 == Instance Data
	bufferBindingLayouts[0].binding = 0;
	bufferBindingLayouts[0].visibility = wgpu::ShaderStage::Vertex | wgpu::ShaderStage::Fragment;
	bufferBindingLayouts[0].buffer.type = wgpu::BufferBindingType::ReadOnlyStorage;
	bufferBindingLayouts[0].buffer.minBindingSize = sizeof(model_instance_data);
	bufferBindingLayouts[0].buffer.hasDynamicOffset = false;
	// G2 B0 == Camera Data
	bufferBindingLayouts[1].binding = 0;
	bufferBindingLayouts[1].visibility = wgpu::ShaderStage::Vertex | wgpu::ShaderStage::Fragment;
	bufferBindingLayouts[1].buffer.type = wgpu::BufferBindingType::Uniform;
	bufferBindingLayouts[1].buffer.minBindingSize = sizeof(camera_upload_data);
	bufferBindingLayouts[1].buffer.hasDynamicOffset = false;
	// G2 B1 == Light Data
	bufferBindingLayouts[2].binding = 1;
	bufferBindingLayouts[2].visibility = wgpu::ShaderStage::Vertex | wgpu::ShaderStage::Fragment;
	bufferBindingLayouts[2].buffer.type = wgpu::BufferBindingType::ReadOnlyStorage;
	bufferBindingLayouts[2].buffer.minBindingSize = sizeof(light);
	bufferBindingLayouts[2].buffer.hasDynamicOffset = false;
	// G2 B2 == Time Data
	bufferBindingLayouts[3].binding = 2;
	bufferBindingLayouts[3].visibility = wgpu::ShaderStage::Vertex | wgpu::ShaderStage::Fragment;
	bufferBindingLayouts[3].buffer.type = wgpu::BufferBindingType::Uniform;
	bufferBindingLayouts[3].buffer.minBindingSize = sizeof(time);
	bufferBindingLayouts[3].buffer.hasDynamicOffset = false;

	// G1 B0 == Color Texture Data
	textureBindingLayouts[0].binding = 0;
	textureBindingLayouts[0].visibility = wgpu::ShaderStage::Fragment;
	textureBindingLayouts[0].texture.sampleType = wgpu::TextureSampleType::Float;
	textureBindingLayouts[0].texture.viewDimension = wgpu::TextureViewDimension::_2D;
	// G1 B1 == Color Sampler
	textureBindingLayouts[1].binding = 1;
	textureBindingLayouts[1].visibility = wgpu::ShaderStage::Fragment;
	textureBindingLayouts[1].sampler.type = wgpu::SamplerBindingType::Filtering;
	// G1 B2 == Height Texture Data
	textureBindingLayouts[2].binding = 2;
	textureBindingLayouts[2].visibility = wgpu::ShaderStage::Fragment;
	textureBindingLayouts[2].texture.sampleType = wgpu::TextureSampleType::Float;
	textureBindingLayouts[2].texture.viewDimension = wgpu::TextureViewDimension::_2D;
	// G1 B3 == Height Sampler
	textureBindingLayouts[3].binding = 3;
	textureBindingLayouts[3].visibility = wgpu::ShaderStage::Fragment;
	textureBindingLayouts[3].sampler.type = wgpu::SamplerBindingType::Filtering;
	// G1 B4 == Normal Texture Data
	textureBindingLayouts[4].binding = 4;
	textureBindingLayouts[4].visibility = wgpu::ShaderStage::Fragment;
	textureBindingLayouts[4].texture.sampleType = wgpu::TextureSampleType::Float;
	textureBindingLayouts[4].texture.viewDimension = wgpu::TextureViewDimension::_2D;
	// G1 B5 == Normal Sampler
	textureBindingLayouts[5].binding = 5;
	textureBindingLayouts[5].visibility = wgpu::ShaderStage::Fragment;
	textureBindingLayouts[5].sampler.type = wgpu::SamplerBindingType::Filtering;
	// G1 B6 == PackedMap Texture Data
	textureBindingLayouts[6].binding = 6;
	textureBindingLayouts[6].visibility = wgpu::ShaderStage::Fragment;
	textureBindingLayouts[6].texture.sampleType = wgpu::TextureSampleType::Float;
	textureBindingLayouts[6].texture.viewDimension = wgpu::TextureViewDimension::_2D;
	// G1 B7 == Packed Sampler
	textureBindingLayouts[7].binding = 7;
	textureBindingLayouts[7].visibility = wgpu::ShaderStage::Fragment;
	textureBindingLayouts[7].sampler.type = wgpu::SamplerBindingType::Filtering;
	// G1 B8 == Roughness Texture Data
	textureBindingLayouts[8].binding = 8;
	textureBindingLayouts[8].visibility = wgpu::ShaderStage::Fragment;
	textureBindingLayouts[8].texture.sampleType = wgpu::TextureSampleType::Float;
	textureBindingLayouts[8].texture.viewDimension = wgpu::TextureViewDimension::_2D;
	// G1 B9 == Roughness Sampler
	textureBindingLayouts[9].binding = 9;
	textureBindingLayouts[9].visibility = wgpu::ShaderStage::Fragment;
	textureBindingLayouts[9].sampler.type = wgpu::SamplerBindingType::Filtering;
	// G1 B10 == Metalness Texture Data
	textureBindingLayouts[10].binding = 10;
	textureBindingLayouts[10].visibility = wgpu::ShaderStage::Fragment;
	textureBindingLayouts[10].texture.sampleType = wgpu::TextureSampleType::Float;
	textureBindingLayouts[10].texture.viewDimension = wgpu::TextureViewDimension::_2D;
	// G1 B11 == Metalness Sampler
	textureBindingLayouts[11].binding = 11;
	textureBindingLayouts[11].visibility = wgpu::ShaderStage::Fragment;
	textureBindingLayouts[11].sampler.type = wgpu::SamplerBindingType::Filtering;
	// G1 B12 == AmbientOcclusion Texture Data
	textureBindingLayouts[12].binding = 12;
	textureBindingLayouts[12].visibility = wgpu::ShaderStage::Fragment;
	textureBindingLayouts[12].texture.sampleType = wgpu::TextureSampleType::Float;
	textureBindingLayouts[12].texture.viewDimension = wgpu::TextureViewDimension::_2D;
	// G1 B13 == AO Sampler
	textureBindingLayouts[13].binding = 13;
	textureBindingLayouts[13].visibility = wgpu::ShaderStage::Fragment;
	textureBindingLayouts[13].sampler.type = wgpu::SamplerBindingType::Filtering;
	// G1 B14 == Emmision Texture Data
	textureBindingLayouts[14].binding = 14;
	textureBindingLayouts[14].visibility = wgpu::ShaderStage::Fragment;
	textureBindingLayouts[14].texture.sampleType = wgpu::TextureSampleType::Float;
	textureBindingLayouts[14].texture.viewDimension = wgpu::TextureViewDimension::_2D;
	// G1 B15 == Emmission Sampler
	textureBindingLayouts[15].binding = 15;
	textureBindingLayouts[15].visibility = wgpu::ShaderStage::Fragment;
	textureBindingLayouts[15].sampler.type = wgpu::SamplerBindingType::Filtering;

	// Create a bind group layout
	wgpu::BindGroupLayoutDescriptor bindGroupLayoutDesc{};
	// G0 == Instance Data
	bindGroupLayoutDesc.label = "Waylib Instance Data Bind Group Layout";
	bindGroupLayoutDesc.entryCount = 1;
	bindGroupLayoutDesc.entries = &bufferBindingLayouts[0];
	global.bindGroupLayouts[0] = state.device.createBindGroupLayout(bindGroupLayoutDesc);
	// G1 == Texture Data
	bindGroupLayoutDesc.label = "Waylib Texture Bind Group Layout";
	bindGroupLayoutDesc.entryCount = textureBindingLayouts.size();
	bindGroupLayoutDesc.entries = textureBindingLayouts.data();
	global.bindGroupLayouts[1] = state.device.createBindGroupLayout(bindGroupLayoutDesc);
	// G2 == Utility Camera (B0) / Light (B1) / Time (B2) Data
	bindGroupLayoutDesc.label = "Waylib Utility Data Bind Group Layout";
	bindGroupLayoutDesc.entryCount = 3;
	bindGroupLayoutDesc.entries = &bufferBindingLayouts[1];
	global.bindGroupLayouts[2] = state.device.createBindGroupLayout(bindGroupLayoutDesc);

	wgpu::PipelineLayoutDescriptor layoutDesc;
	layoutDesc.bindGroupLayoutCount = global.bindGroupLayouts.size();
	layoutDesc.bindGroupLayouts = global.bindGroupLayouts.data();
	global.layout = state.device.createPipelineLayout(layoutDesc);

	global.created = true;
	global.min_buffer_size = std::max({
		bufferBindingLayouts[0].buffer.minBindingSize,
		bufferBindingLayouts[1].buffer.minBindingSize,
		bufferBindingLayouts[2].buffer.minBindingSize,
		bufferBindingLayouts[3].buffer.minBindingSize
	});
	return global;
}

//////////////////////////////////////////////////////////////////////
// #Miscelanious
//////////////////////////////////////////////////////////////////////

static std::string error_message = "";

const char* get_error_message() {
	if(error_message.empty()) return nullptr;
	return error_message.c_str();
}

void set_error_message_raw(const char* message) {
	error_message = message;
}
void set_error_message(const std::string_view view) {
	error_message = std::string(view);
}
void set_error_message(const std::string& str) {
	error_message = str;
}

void clear_error_message() {
	set_error_message_raw("");
}
std::string get_error_message_and_clear() {
	const char* msg = get_error_message();
	if(!msg) return "";
	std::string out = msg;
	clear_error_message();
	return out;
}

#ifdef WAYLIB_NAMESPACE_NAME
}
#endif