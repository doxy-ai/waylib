#define WEBGPU_CPP_IMPLEMENTATION
#include "common.hpp"

#include <string>

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
	std::array<wgpu::BindGroupLayoutEntry, 3> bindingLayouts = {wgpu::Default, wgpu::Default, wgpu::Default};
	// G0 B0 == Instance Data
	bindingLayouts[0].binding = 0;
	bindingLayouts[0].visibility = wgpu::ShaderStage::Vertex | wgpu::ShaderStage::Fragment;
	bindingLayouts[0].buffer.type = wgpu::BufferBindingType::ReadOnlyStorage;
	bindingLayouts[0].buffer.minBindingSize = sizeof(model_instance_data);
	bindingLayouts[0].buffer.hasDynamicOffset = false;
	// G1 B0 == Time Data
	bindingLayouts[1].binding = 0;
	bindingLayouts[1].visibility = wgpu::ShaderStage::Vertex | wgpu::ShaderStage::Fragment;
	bindingLayouts[1].buffer.type = wgpu::BufferBindingType::Uniform;
	bindingLayouts[1].buffer.minBindingSize = sizeof(time);
	bindingLayouts[1].buffer.hasDynamicOffset = false;
	// G2 B0 == Camera Data
	bindingLayouts[2].binding = 0;
	bindingLayouts[2].visibility = wgpu::ShaderStage::Vertex | wgpu::ShaderStage::Fragment;
	bindingLayouts[2].buffer.type = wgpu::BufferBindingType::Uniform;
	bindingLayouts[2].buffer.minBindingSize = sizeof(camera_upload_data);
	bindingLayouts[2].buffer.hasDynamicOffset = false;

	// Create a bind group layout
	wgpu::BindGroupLayoutDescriptor bindGroupLayoutDesc{};
	// G0 == Instance Data
	bindGroupLayoutDesc.label = "Waylib Instance Data Bind Group Layout";
	bindGroupLayoutDesc.entryCount = 1;
	bindGroupLayoutDesc.entries = &bindingLayouts[0];
	global.bindGroupLayouts[0] = state.device.createBindGroupLayout(bindGroupLayoutDesc);
	// G1 == Time Data
	bindGroupLayoutDesc.label = "Waylib Time Data Bind Group Layout";
	bindGroupLayoutDesc.entryCount = 1;
	bindGroupLayoutDesc.entries = &bindingLayouts[1];
	global.bindGroupLayouts[1] = state.device.createBindGroupLayout(bindGroupLayoutDesc);
	// G2 == Camera Data
	bindGroupLayoutDesc.label = "Waylib Camera Data Bind Group Layout";
	bindGroupLayoutDesc.entryCount = 1;
	bindGroupLayoutDesc.entries = &bindingLayouts[2];
	global.bindGroupLayouts[2] = state.device.createBindGroupLayout(bindGroupLayoutDesc);

	wgpu::PipelineLayoutDescriptor layoutDesc;
	layoutDesc.bindGroupLayoutCount = global.bindGroupLayouts.size();
	layoutDesc.bindGroupLayouts = global.bindGroupLayouts.data();
	global.layout = state.device.createPipelineLayout(layoutDesc);

	global.created = true;
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