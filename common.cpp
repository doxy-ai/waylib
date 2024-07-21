#define WEBGPU_CPP_IMPLEMENTATION
#include "common.hpp"

#include <string>
#include <algorithm>
#include <cstring>
#include <numeric>

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
	std::array<wgpu::BindGroupLayoutEntry, 19> modelTimeBindingLayouts; modelTimeBindingLayouts.fill(wgpu::Default);
	std::array<wgpu::BindGroupLayoutEntry, 3> cameraTimeBindingLayouts; cameraTimeBindingLayouts.fill(wgpu::Default);

	// G0 B0 == Instance Data
	modelTimeBindingLayouts[0].binding = 0;
	modelTimeBindingLayouts[0].visibility = wgpu::ShaderStage::Vertex | wgpu::ShaderStage::Fragment;
	modelTimeBindingLayouts[0].buffer.type = wgpu::BufferBindingType::ReadOnlyStorage;
	modelTimeBindingLayouts[0].buffer.minBindingSize = sizeof(model_instance_data);
	modelTimeBindingLayouts[0].buffer.hasDynamicOffset = false;
	// G0 B1 == Color Texture Data
	modelTimeBindingLayouts[1].binding = 1;
	modelTimeBindingLayouts[1].visibility = wgpu::ShaderStage::Fragment;
	modelTimeBindingLayouts[1].texture.sampleType = wgpu::TextureSampleType::Float;
	modelTimeBindingLayouts[1].texture.viewDimension = wgpu::TextureViewDimension::_2D;
	// G0 B2 == Color Sampler
	modelTimeBindingLayouts[2].binding = 2;
	modelTimeBindingLayouts[2].visibility = wgpu::ShaderStage::Fragment;
	modelTimeBindingLayouts[2].sampler.type = wgpu::SamplerBindingType::Filtering;
	// G0 B3 == Height Texture Data
	modelTimeBindingLayouts[3].binding = 3;
	modelTimeBindingLayouts[3].visibility = wgpu::ShaderStage::Fragment;
	modelTimeBindingLayouts[3].texture.sampleType = wgpu::TextureSampleType::Float;
	modelTimeBindingLayouts[3].texture.viewDimension = wgpu::TextureViewDimension::_2D;
	// G0 B4 == Height Sampler
	modelTimeBindingLayouts[4].binding = 4;
	modelTimeBindingLayouts[4].visibility = wgpu::ShaderStage::Fragment;
	modelTimeBindingLayouts[4].sampler.type = wgpu::SamplerBindingType::Filtering;
	// G0 B5 == Normal Texture Data
	modelTimeBindingLayouts[5].binding = 5;
	modelTimeBindingLayouts[5].visibility = wgpu::ShaderStage::Fragment;
	modelTimeBindingLayouts[5].texture.sampleType = wgpu::TextureSampleType::Float;
	modelTimeBindingLayouts[5].texture.viewDimension = wgpu::TextureViewDimension::_2D;
	// G0 B6 == Normal Sampler
	modelTimeBindingLayouts[6].binding = 6;
	modelTimeBindingLayouts[6].visibility = wgpu::ShaderStage::Fragment;
	modelTimeBindingLayouts[6].sampler.type = wgpu::SamplerBindingType::Filtering;
	// G0 B7 == PackedMap Texture Data
	modelTimeBindingLayouts[7].binding = 7;
	modelTimeBindingLayouts[7].visibility = wgpu::ShaderStage::Fragment;
	modelTimeBindingLayouts[7].texture.sampleType = wgpu::TextureSampleType::Float;
	modelTimeBindingLayouts[7].texture.viewDimension = wgpu::TextureViewDimension::_2D;
	// G0 B8 == Packed Sampler
	modelTimeBindingLayouts[8].binding = 8;
	modelTimeBindingLayouts[8].visibility = wgpu::ShaderStage::Fragment;
	modelTimeBindingLayouts[8].sampler.type = wgpu::SamplerBindingType::Filtering;
	// G0 B9 == Roughness Texture Data
	modelTimeBindingLayouts[9].binding = 9;
	modelTimeBindingLayouts[9].visibility = wgpu::ShaderStage::Fragment;
	modelTimeBindingLayouts[9].texture.sampleType = wgpu::TextureSampleType::Float;
	modelTimeBindingLayouts[9].texture.viewDimension = wgpu::TextureViewDimension::_2D;
	// G0 B10 == Roughness Sampler
	modelTimeBindingLayouts[10].binding = 10;
	modelTimeBindingLayouts[10].visibility = wgpu::ShaderStage::Fragment;
	modelTimeBindingLayouts[10].sampler.type = wgpu::SamplerBindingType::Filtering;
	// G0 B11 == Metalness Texture Data
	modelTimeBindingLayouts[11].binding = 11;
	modelTimeBindingLayouts[11].visibility = wgpu::ShaderStage::Fragment;
	modelTimeBindingLayouts[11].texture.sampleType = wgpu::TextureSampleType::Float;
	modelTimeBindingLayouts[11].texture.viewDimension = wgpu::TextureViewDimension::_2D;
	// G0 B12 == Metalness Sampler
	modelTimeBindingLayouts[12].binding = 12;
	modelTimeBindingLayouts[12].visibility = wgpu::ShaderStage::Fragment;
	modelTimeBindingLayouts[12].sampler.type = wgpu::SamplerBindingType::Filtering;
	// G0 B13 == AmbientOcclusion Texture Data
	modelTimeBindingLayouts[13].binding = 13;
	modelTimeBindingLayouts[13].visibility = wgpu::ShaderStage::Fragment;
	modelTimeBindingLayouts[13].texture.sampleType = wgpu::TextureSampleType::Float;
	modelTimeBindingLayouts[13].texture.viewDimension = wgpu::TextureViewDimension::_2D;
	// G0 B14 == AO Sampler
	modelTimeBindingLayouts[14].binding = 14;
	modelTimeBindingLayouts[14].visibility = wgpu::ShaderStage::Fragment;
	modelTimeBindingLayouts[14].sampler.type = wgpu::SamplerBindingType::Filtering;
	// G0 B15 == Emission Texture Data
	modelTimeBindingLayouts[15].binding = 15;
	modelTimeBindingLayouts[15].visibility = wgpu::ShaderStage::Fragment;
	modelTimeBindingLayouts[15].texture.sampleType = wgpu::TextureSampleType::Float;
	modelTimeBindingLayouts[15].texture.viewDimension = wgpu::TextureViewDimension::_2D;
	// G0 B16 == Emmission Sampler
	modelTimeBindingLayouts[16].binding = 16;
	modelTimeBindingLayouts[16].visibility = wgpu::ShaderStage::Fragment;
	modelTimeBindingLayouts[16].sampler.type = wgpu::SamplerBindingType::Filtering;
	// G0 B17 == Cubemap Texture Data
	modelTimeBindingLayouts[17].binding = 17;
	modelTimeBindingLayouts[17].visibility = wgpu::ShaderStage::Fragment;
	modelTimeBindingLayouts[17].texture.sampleType = wgpu::TextureSampleType::Float;
	modelTimeBindingLayouts[17].texture.viewDimension = wgpu::TextureViewDimension::Cube;
	// G0 B18 == Cubemap Sampler
	modelTimeBindingLayouts[18].binding = 18;
	modelTimeBindingLayouts[18].visibility = wgpu::ShaderStage::Fragment;
	modelTimeBindingLayouts[18].sampler.type = wgpu::SamplerBindingType::Filtering;

	// G1 B0 == Camera Data
	cameraTimeBindingLayouts[0].binding = 0;
	cameraTimeBindingLayouts[0].visibility = wgpu::ShaderStage::Vertex | wgpu::ShaderStage::Fragment;
	cameraTimeBindingLayouts[0].buffer.type = wgpu::BufferBindingType::Uniform;
	cameraTimeBindingLayouts[0].buffer.minBindingSize = sizeof(camera_upload_data);
	cameraTimeBindingLayouts[0].buffer.hasDynamicOffset = false;
	// G1 B1 == Light Data
	cameraTimeBindingLayouts[1].binding = 1;
	cameraTimeBindingLayouts[1].visibility = wgpu::ShaderStage::Vertex | wgpu::ShaderStage::Fragment;
	cameraTimeBindingLayouts[1].buffer.type = wgpu::BufferBindingType::ReadOnlyStorage;
	cameraTimeBindingLayouts[1].buffer.minBindingSize = sizeof(light);
	cameraTimeBindingLayouts[1].buffer.hasDynamicOffset = false;
	// G1 B2 == Time Data
	cameraTimeBindingLayouts[2].binding = 2;
	cameraTimeBindingLayouts[2].visibility = wgpu::ShaderStage::Vertex | wgpu::ShaderStage::Fragment;
	cameraTimeBindingLayouts[2].buffer.type = wgpu::BufferBindingType::Uniform;
	cameraTimeBindingLayouts[2].buffer.minBindingSize = sizeof(time);
	cameraTimeBindingLayouts[2].buffer.hasDynamicOffset = false;

	// Create a bind group layout
	wgpu::BindGroupLayoutDescriptor bindGroupLayoutDesc{};
	// G0 == Instance (B0)/Texture Data
	bindGroupLayoutDesc.label = "Waylib Instance Data Bind Group Layout";
	bindGroupLayoutDesc.entryCount = modelTimeBindingLayouts.size();
	bindGroupLayoutDesc.entries = modelTimeBindingLayouts.data();
	global.bindGroupLayouts[0] = state.device.createBindGroupLayout(bindGroupLayoutDesc);
	// G1 == Utility Camera (B0) / Light (B1) / Time (B2) Data
	bindGroupLayoutDesc.label = "Waylib Utility Data Bind Group Layout";
	bindGroupLayoutDesc.entryCount = cameraTimeBindingLayouts.size();
	bindGroupLayoutDesc.entries = cameraTimeBindingLayouts.data();
	global.bindGroupLayouts[1] = state.device.createBindGroupLayout(bindGroupLayoutDesc);

	wgpu::PipelineLayoutDescriptor layoutDesc;
	layoutDesc.bindGroupLayoutCount = global.bindGroupLayouts.size();
	layoutDesc.bindGroupLayouts = global.bindGroupLayouts.data();
	global.layout = state.device.createPipelineLayout(layoutDesc);

	global.created = true;
	global.min_buffer_size = std::max({
		cameraTimeBindingLayouts[0].buffer.minBindingSize,
		cameraTimeBindingLayouts[1].buffer.minBindingSize,
		cameraTimeBindingLayouts[2].buffer.minBindingSize,
		modelTimeBindingLayouts[0].buffer.minBindingSize
	});
	return global;
}

//////////////////////////////////////////////////////////////////////
// #Errors
//////////////////////////////////////////////////////////////////////

static std::string error_message = "";

WAYLIB_NULLABLE(const char*) get_error_message() {
	if(error_message.empty()) return nullptr;
	return error_message.c_str();
}

void set_error_message_raw(WAYLIB_NULLABLE(const char*) message) {
	if(!message) error_message = "";
	else error_message = message;
}
void set_error_message(const std::string_view view) {
	error_message = view;
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

//////////////////////////////////////////////////////////////////////
// #Image
//////////////////////////////////////////////////////////////////////

color convert(const color8& c) {
	return {c.r / 255.f, c.g / 255.f, c.b / 255.f, c.a / 255.f};
}
color convert_to_color(const color8 c) { return convert(c); }
color8 convert(const color& c) {
	return {
		std::clamp<unsigned char>(c.r * 255, 0, 255),
		std::clamp<unsigned char>(c.g * 255, 0, 255),
		std::clamp<unsigned char>(c.b * 255, 0, 255),
		std::clamp<unsigned char>(c.a * 255, 0, 255)
	};
}
color8 convert_to_color8(const color c) { return convert(c); }

WAYLIB_NULLABLE(image_data_span<>) image_get_frame(image& image, size_t frame, size_t mip_level /*= 0*/) {
	if(frame > image.frames) return {.data = {(color8*)nullptr, 0}};
	if(mip_level > image.mipmaps) return {.data = {(color8*)nullptr, 0}};

	// TODO: Mipmaps
	size_t size = image.width * image.height;

	if(image.float_data)
		return {.data32 = {image.data32 + size * frame, size}};
	else return {.data = {image.data + size * frame, size}};
}
WAYLIB_NULLABLE(void*) image_get_frame_and_size(size_t* out_size, image* image, size_t frame, size_t mip_level /*= 0*/) {
	auto span = image_get_frame(*image, frame, mip_level);
	if(!span.data.size()) return nullptr;
	*out_size = span.data.size();
	return span.data.data();
}
WAYLIB_NULLABLE(void*) image_get_frame(image* image, size_t frame, size_t mip_level /*= 0*/) {
	auto span = image_get_frame(*image, frame, mip_level);
	if(!span.data.size()) return nullptr;
	return span.data.data();
}

WAYLIB_OPTIONAL(image) merge_images(std::span<image> images, bool free_incoming /*= true*/) {
	for(auto& image: images)
		if( !(image.width == images[0].width && image.height == images[0].height) ) {
			set_error_message_raw("Image dimensions don't match!");
			return {};
		}

	image out;
	out.frames = std::accumulate(images.begin(), images.end(), 0, [](size_t frames, const image& img) {
		return frames + img.frames;
	});
	out.width = images[0].width;
	out.height = images[0].height;
	out.mipmaps = 1;
	out.heap_allocated = true;

	size_t size = out.frames * out.width * out.height;
	if((out.float_data = images[0].float_data))
		out.data32 = new color[size];
	else out.data = new color8[size];
	size_t matchingSize = out.float_data ? sizeof(color) : sizeof(color8);
	size = out.width * out.height;

	size_t frame = 0;
	for(auto& image: images)
		for(size_t f = 0; f < image.frames; ++f, ++frame) {
			image_data_span data = image_get_frame(image, f);
			if(image.float_data == out.float_data) {
				char* dbg = (char*)out.data + size * matchingSize * frame;
				memcpy(dbg, data.data.data(), size * matchingSize);
			}
			else for(size_t i = size; i--; )
				if(out.float_data) out.data32[size * frame + i] = convert(data.data[i]);
				else out.data[size * frame + i] = convert(data.data32[i]);
		}

	if(free_incoming) {
		// TODO: Free
	}

	return out;
}
WAYLIB_OPTIONAL(image) merge_images(image* images, size_t images_size, bool free_incoming /*= true*/) {
	return merge_images({images, images_size}, free_incoming);
}

#ifdef WAYLIB_NAMESPACE_NAME
}
#endif