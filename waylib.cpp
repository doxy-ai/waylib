#define WEBGPU_CPP_IMPLEMENTATION
#include "waylib.hpp"

#include <chrono>
#include <fstream>
#include <limits>
#include <glm/ext/matrix_transform.hpp>
#include <glm/ext/matrix_clip_space.hpp>

#define TCPP_IMPLEMENTATION
#include "thirdparty/tcppLibrary.hpp"

#define OPEN_URL_NAMESPACE wl_detail
#include "thirdparty/open.hpp"

#ifdef WAYLIB_NAMESPACE_NAME
namespace WAYLIB_NAMESPACE_NAME {
#endif

//////////////////////////////////////////////////////////////////////
// #Pipeline Globals
//////////////////////////////////////////////////////////////////////

pipeline_globals& create_pipeline_globals(webgpu_state state) {
	static pipeline_globals global;
	if(global.created) return global;

	// Create binding layout (don't forget to = Default)
	std::array<wgpu::BindGroupLayoutEntry, 2> bindingLayouts = {Default, Default};
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

	// Create a bind group layout
	wgpu::BindGroupLayoutDescriptor bindGroupLayoutDesc{};
	bindGroupLayoutDesc.label = "Waylib Instance Data Bind Group Layout";
	bindGroupLayoutDesc.entryCount = 1;
	bindGroupLayoutDesc.entries = &bindingLayouts[0];
	// G0 == Instance Data
	global.bindGroupLayouts[0] = state.device.createBindGroupLayout(bindGroupLayoutDesc);
	bindGroupLayoutDesc.label = "Waylib Time Data Bind Group Layout";
	bindGroupLayoutDesc.entryCount = 1;
	bindGroupLayoutDesc.entries = &bindingLayouts[1];
	// G1 == Time Data
	global.bindGroupLayouts[1] = state.device.createBindGroupLayout(bindGroupLayoutDesc);

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

wgpu::Color to_webgpu(const color& color) {
	return {color.r, color.g, color.b, color.a};
}

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

void time_calculations(time& time) {
	constexpr static float alpha = .9;
	static std::chrono::system_clock::time_point last = std::chrono::system_clock::now();

	auto now = std::chrono::system_clock::now();
	time.delta = std::chrono::duration_cast<std::chrono::microseconds>(now - last).count() / 1000000.0;
	time.since_start += time.delta;

	if(std::abs(time.average_delta) < 2 * std::numeric_limits<float>::epsilon()) time.average_delta = time.delta;
	time.average_delta = time.average_delta * alpha + time.delta * (1 - alpha);

	last = now;
}
void time_calculations(time* time) { time_calculations(*time); }

void time_upload(webgpu_frame_state& frame, time& time) {
	static wgpu::Buffer timeBuffer = [&frame] {
		WGPUBufferDescriptor bufferDesc = {
			.label = "Waylib Time Buffer",
			.usage = wgpu::BufferUsage::CopyDst | wgpu::BufferUsage::Uniform,
			.size = sizeof(time),
			.mappedAtCreation = false,
		};
		return frame.state.device.createBuffer(bufferDesc);
	}();
	static wgpu::BindGroup bindGroup = [&frame] {
		std::array<WGPUBindGroupEntry, 1> bindings = {WGPUBindGroupEntry{
			.binding = 0,
			.buffer = timeBuffer,
			.offset = 0,
			.size = sizeof(time)
		}};
		wgpu::BindGroupDescriptor bindGroupDesc = wgpu::Default;
		bindGroupDesc.layout = create_pipeline_globals(frame.state).bindGroupLayouts[1]; // Group 1 is time data
		bindGroupDesc.entryCount = bindings.size();
		bindGroupDesc.entries = bindings.data();
		return frame.state.device.createBindGroup(bindGroupDesc);
	}();

	frame.state.device.getQueue().writeBuffer(timeBuffer, 0, &time, sizeof(time));
	frame.render_pass.setBindGroup(1, bindGroup, 0, nullptr);
}
void time_upload(webgpu_frame_state* frame, time* time) {
	time_upload(*frame, *time);
}

bool open_url(const char* url) {
	return wl_detail::open_url(url);
}

void process_wgpu_events([[maybe_unused]] Device device, [[maybe_unused]] bool yieldToWebBrowser = true) {
#if defined(WEBGPU_BACKEND_DAWN)
    device.tick();
// #elif defined(WEBGPU_BACKEND_WGPU)
//     device.poll(false);
#elif defined(WEBGPU_BACKEND_EMSCRIPTEN)
    if (yieldToWebBrowser) {
        emscripten_sleep(10);
    }
#endif
}

//////////////////////////////////////////////////////////////////////
// #WebGPU Defaults
//////////////////////////////////////////////////////////////////////

wgpu::Device create_default_device_from_instance(WGPUInstance instance, WGPUSurface surface /*= nullptr*/, bool prefer_low_power /*= false*/) {
	wgpu::RequestAdapterOptions adapterOpts = {};
	adapterOpts.compatibleSurface = surface;
	adapterOpts.powerPreference = prefer_low_power ? wgpu::PowerPreference::LowPower : wgpu::PowerPreference::HighPerformance;
	wgpu::Adapter adapter = WAYLIB_C_TO_CPP_CONVERSION(wgpu::instance, instance).requestAdapter(adapterOpts);
	// std::cout << "Got adapter: " << adapter << std::endl;

	// std::cout << "Requesting device..." << std::endl;
	wgpu::DeviceDescriptor deviceDesc = {};
	deviceDesc.label = "Waylib Device";
	deviceDesc.requiredFeatureCount = 0;
	deviceDesc.requiredLimits = nullptr;
	deviceDesc.defaultQueue.nextInChain = nullptr;
	deviceDesc.defaultQueue.label = "Waylib Queue";
	deviceDesc.deviceLostCallback = [](WGPUDeviceLostReason reason, char const* message, void* /* pUserData */) {
		std::cout << "Device lost: reason " << reason;
		if (message) std::cout << " (" << message << ")";
		std::cout << std::endl;
	};
	return adapter.requestDevice(deviceDesc);
}

wgpu::Device create_default_device(WGPUSurface surface /*= nullptr*/, bool prefer_low_power /*= false*/) {
	return create_default_device_from_instance(wgpuCreateInstance({}), surface, prefer_low_power);
}

void release_device(WGPUDevice device, bool also_release_adapter /*= true*/, bool also_release_instance /*= true*/) {
	wgpu::Adapter adapter = WAYLIB_C_TO_CPP_CONVERSION(wgpu::Device, device).getAdapter();
	wgpu::Instance instance = adapter.getInstance();
	wgpuDeviceRelease(device);
	if(also_release_adapter) wgpuAdapterRelease(adapter);
	if(also_release_instance) wgpuInstanceRelease(instance);
}

void release_webgpu_state(webgpu_state state) {
	state.device.getQueue().release();
	state.surface.release();
	release_device(state.device, true, true);
}

bool configure_surface(webgpu_state state, vec2i size, surface_configuration config /*= {}*/) {
	// Configure the surface
	wgpu::SurfaceConfiguration descriptor = {};

	wgpu::SurfaceCapabilities capabilities;
	state.surface.getCapabilities(state.device.getAdapter(), &capabilities); // TODO: Always returns error?

	bool found = false;
	for(size_t i = 0; i < capabilities.presentModeCount; ++i)
		if(capabilities.presentModes[i] == config.presentaion_mode) {
			found = true;
			break;
		}
	if(!found) {
		set_error_message_raw("Desired present mode not available... Falling back to First in First out.");
		config.presentaion_mode = wgpu::PresentMode::Fifo; // Fifo
	}

	// Configuration of the textures created for the underlying swap chain
	descriptor.width = size.x;
	descriptor.height = size.y;
	descriptor.usage = wgpu::TextureUsage::RenderAttachment;
	descriptor.format = capabilities.formats[0];

	// And we do not need any particular view format:
	descriptor.viewFormatCount = 0;
	descriptor.viewFormats = nullptr;
	descriptor.device = state.device;
	descriptor.presentMode = config.presentaion_mode;
	descriptor.alphaMode = config.alpha_mode;

	state.surface.configure(descriptor);
	return true;
}

//////////////////////////////////////////////////////////////////////
// #Shader (Preprocessor)
//////////////////////////////////////////////////////////////////////

namespace detail {
	std::string remove_whitespace(const std::string& in) {
		std::string out;
		for(char c: in)
			if(!std::isspace(c))
				out += c;
		return out;
	}

	std::string process_pragma_once(std::string data, const std::filesystem::path& path) {
		if(auto once = data.find("#pragma once"); once != std::string::npos) {
			std::string guard = path;
			std::transform(guard.begin(), guard.end(), guard.begin(), [](char c) -> char {
				if(!std::isalnum(c)) return '_';
				return std::toupper(c);
			});
			guard = "__" + guard + "_GUARD__";
			data.replace(once, 12, "#ifndef " + guard + "\n#define " + guard + "\n");
			data += "#endif //" + guard + "\n";
		}
		return data;
	}

	std::optional<std::string> read_entire_file(std::filesystem::path path, const preprocess_shader_config& config = {}) {
		std::ifstream fin(path);
		if(!fin) {
			set_error_message("Failed to open file `" + std::string(path) + "`... does it exist?");
			return {};
		}
		fin.seekg(0, std::ios::end);
		size_t size = fin.tellg();
		fin.seekg(0, std::ios::beg);

		std::string data(size + 1, '\n');
		fin.read(data.data(), size);
		if(config.support_pragma_once)
			return process_pragma_once(data, path);
		return data;
	}

	std::optional<std::string> preprocess_shader(shader_preprocessor* processor, const std::filesystem::path& path, const preprocess_shader_config& config);

	std::optional<std::string> preprocess_shader_from_memory(shader_preprocessor* processor, const std::string& _data, const preprocess_shader_config& config) {
		struct PreprocessFailed {};

		auto data = std::make_unique<tcpp::StringInputStream>(processor->defines + _data + "\n");
		tcpp::Lexer lexer(std::move(data));
		try {
			tcpp::Preprocessor preprocessor(lexer, {[](const tcpp::TErrorInfo& info){
				auto error = tcpp::ErrorTypeToString(info.mType);
				if (error.empty()) error = "Unknown error";
				error += " on line: " + std::to_string(info.mLine);
				// TODO: Include that line of the input!
				set_error_message(error);
				throw PreprocessFailed{};
			}, [&](const std::string& _path, bool isSystem){
				std::filesystem::path path = _path;
				if(processor->file_cache.find(path) != processor->file_cache.end())
					return std::make_unique<tcpp::StringInputStream>(processor->file_cache[path]);

				#define WAYLIB_NON_SYSTEM_PATHS {\
					if(config.path) {\
						auto relativeToConfig = std::filesystem::absolute(std::filesystem::path(config.path).parent_path() / path);\
						if(std::filesystem::exists(relativeToConfig)){\
							if(auto res = read_entire_file(relativeToConfig, config); res)\
								return std::make_unique<tcpp::StringInputStream>(*res);\
							else throw PreprocessFailed{};\
						}\
					}\
					auto absolute = std::filesystem::absolute(path);\
					if(std::filesystem::exists(absolute)) {\
						if(auto res = read_entire_file(absolute, config); res)\
							return std::make_unique<tcpp::StringInputStream>(*res);\
						else throw PreprocessFailed{};\
					}\
				}
				if(!isSystem) WAYLIB_NON_SYSTEM_PATHS

				for(auto system: processor->search_paths) {
					system = system / path;
					if(std::filesystem::exists(system)) {
						if(auto res = read_entire_file(system, config); res)
							return std::make_unique<tcpp::StringInputStream>(*res);
						else throw PreprocessFailed{};
					}
				}

				if(isSystem) WAYLIB_NON_SYSTEM_PATHS
				#undef WAYLIB_NON_SYSTEM_PATHS

				set_error_message("Included file `" + std::string(path) + "` could not be found!");
				throw PreprocessFailed{};
			}, config.remove_comments});

			if(config.remove_whitespace) return remove_whitespace(preprocessor.Process());
			return preprocessor.Process();
		} catch(PreprocessFailed) {
			return {};
		}
	}

	std::optional<std::string> preprocess_shader_from_memory_and_cache(shader_preprocessor* processor, const std::string& _data, const std::filesystem::path& path, preprocess_shader_config config) {
		std::string data;
		if(config.support_pragma_once) data = process_pragma_once(_data, path);
		else data = _data;

		config.path = path.c_str();
		if(auto res = preprocess_shader_from_memory(processor, data, config); res) {
			processor->file_cache[path] = data;
			return *res;
		}
		return {};
	}

	std::optional<std::string> preprocess_shader(shader_preprocessor* processor, const std::filesystem::path& path, const preprocess_shader_config& config) {
		if(processor->file_cache.find(path) != processor->file_cache.end())
			return processor->file_cache[path];

		auto data = read_entire_file(path, config);
		if(!data) return {};
		return preprocess_shader_from_memory_and_cache(processor, *data, path, config);
	}
}

shader_preprocessor* create_shader_preprocessor() {
	return new shader_preprocessor();
}

void release_shader_preprocessor(shader_preprocessor* processor) {
	delete processor;
}

void preprocessor_add_define(shader_preprocessor* processor, const char* name, const char* value) {
	processor->defines += "#define " + std::string(name) + " " + value + "\n";
}

bool preprocessor_add_search_path(shader_preprocessor* processor, const char* _path) {
	auto path = std::filesystem::absolute(_path);
	if(!std::filesystem::exists(path)) return false;

	processor->search_paths.emplace(path);
	return true;
}

const char* preprocessor_get_cached_file(shader_preprocessor* processor, const char* path) {
	static std::string cache;
	if(processor->file_cache.find(path) != processor->file_cache.end())
		return (cache = processor->file_cache[path]).c_str();
	return nullptr;
}

const char* preprocess_shader_from_memory(shader_preprocessor* processor, const char* data, preprocess_shader_config config /*= {}*/) {
	static std::string cache;
	if(auto res = detail::preprocess_shader_from_memory(processor, data, config); res)
		return (cache = std::move(*res)).c_str();
	return nullptr;
}

const char* preprocess_shader_from_memory_and_cache(shader_preprocessor* processor, const char* data, const char* path, preprocess_shader_config config /*= {}*/) {
	static std::string cache;
	if(auto res = detail::preprocess_shader_from_memory_and_cache(processor, data, path, config); res)
		return (cache = std::move(*res)).c_str();
	return nullptr;
}

const char* preprocess_shader(shader_preprocessor* processor, const char* path, preprocess_shader_config config /*= {}*/) {
	static std::string cache;
	if(auto res = detail::preprocess_shader(processor, path, config); res)
		return (cache = std::move(*res)).c_str();
	return nullptr;
}

WAYLIB_OPTIONAL(shader) create_shader(webgpu_state state, const char* wgsl_source_code, create_shader_configuration config /*= {}*/) {
	wgpu::ShaderModuleDescriptor shaderDesc;
	shaderDesc.label = config.name;
#ifdef WEBGPU_BACKEND_WGPU
	shaderDesc.hintCount = 0;
	shaderDesc.hints = nullptr;
#endif

	// We use the extension mechanism to specify the WGSL part of the shader module descriptor
	wgpu::ShaderModuleWGSLDescriptor shaderCodeDesc;
	// Set the chained struct's header
	shaderCodeDesc.chain.next = nullptr;
	shaderCodeDesc.chain.sType = wgpu::SType::ShaderModuleWGSLDescriptor;
	// Connect the chain
	shaderDesc.nextInChain = &shaderCodeDesc.chain;
	if(config.preprocessor) {
		shaderCodeDesc.code = preprocess_shader_from_memory(config.preprocessor, wgsl_source_code, config.preprocess_config);
		if(!shaderCodeDesc.code) return {};
	} else shaderCodeDesc.code = wgsl_source_code;

	return {{
		.compute_entry_point = config.compute_entry_point,
		.vertex_entry_point = config.vertex_entry_point,
		.fragment_entry_point = config.fragment_entry_point,
		.module = state.device.createShaderModule(shaderDesc)
	}};
}

//////////////////////////////////////////////////////////////////////
// #Begin/End Drawing
//////////////////////////////////////////////////////////////////////

WAYLIB_OPTIONAL(webgpu_frame_state) begin_drawing_render_texture(webgpu_state state, WGPUTextureView render_texture, vec2i render_texture_dimensions, WAYLIB_OPTIONAL(color) clear_color /*= {}*/) {
	static WGPUTextureViewDescriptor depthTextureViewDesc = {
		.format = depth_texture_format,
		.dimension = wgpu::TextureViewDimension::_2D,
		.baseMipLevel = 0,
		.mipLevelCount = 1,
		.baseArrayLayer = 0,
		.arrayLayerCount = 1,
		.aspect = wgpu::TextureAspect::DepthOnly,
	};
	static WGPUTextureDescriptor depthTextureDesc = {
		.usage = TextureUsage::RenderAttachment,
		.dimension = wgpu::TextureDimension::_2D,
		.format = depth_texture_format,
		.mipLevelCount = 1,
		.sampleCount = 1,
		.viewFormatCount = 1,
		.viewFormats = (WGPUTextureFormat*)&depth_texture_format,
	};

	static wgpu::Texture depthTexture;
	static wgpu::TextureView depthTextureView;

	// Create a command encoder for the draw call
	wgpu::CommandEncoderDescriptor encoderDesc = {};
	encoderDesc.label = "Waylib Command Encoder";
	wgpu::CommandEncoder encoder = state.device.createCommandEncoder(encoderDesc);

	// Update the depth texture (if nessicary)
	if(!depthTexture || depthTextureDesc.size.width != render_texture_dimensions.x || depthTextureDesc.size.height != render_texture_dimensions.y) {
		depthTextureDesc.size = {(uint32_t)render_texture_dimensions.x, (uint32_t)render_texture_dimensions.y, 1};
		if(depthTextureView) depthTextureView.release();
		if(depthTexture) {
			depthTexture.destroy();
			depthTexture.release();
		}
		depthTexture = state.device.createTexture(depthTextureDesc);
		depthTextureView = depthTexture.createView(depthTextureViewDesc);
	}

	// Create the render pass that clears the screen with our color
	wgpu::RenderPassDescriptor renderPassDesc = {};

	// The attachment part of the render pass descriptor describes the target texture of the pass
	wgpu::RenderPassColorAttachment renderPassColorAttachment = {};
	renderPassColorAttachment.view = render_texture;
	renderPassColorAttachment.resolveTarget = nullptr;
	renderPassColorAttachment.loadOp = clear_color.has_value ? wgpu::LoadOp::Clear : wgpu::LoadOp::Load;
	renderPassColorAttachment.storeOp = wgpu::StoreOp::Store;
	renderPassColorAttachment.clearValue = to_webgpu(clear_color.has_value ? clear_color.value : color{0, 0, 0, 0});
	renderPassColorAttachment.depthSlice = WGPU_DEPTH_SLICE_UNDEFINED;

	// We now add a depth/stencil attachment:
	wgpu::RenderPassDepthStencilAttachment depthStencilAttachment;
	depthStencilAttachment.view = depthTextureView;
	// The initial value of the depth buffer, meaning "far"
	depthStencilAttachment.depthClearValue = 1.0f;
	// Operation settings comparable to the color attachment
	depthStencilAttachment.depthLoadOp = clear_color.has_value ? wgpu::LoadOp::Clear : wgpu::LoadOp::Load;
	depthStencilAttachment.depthStoreOp = wgpu::StoreOp::Store;
	// we could turn off writing to the depth buffer globally here
	depthStencilAttachment.depthReadOnly = false;

	// Stencil setup, mandatory but unused
	depthStencilAttachment.stencilClearValue = 0;
	// depthStencilAttachment.stencilLoadOp = clear_color.has_value ? wgpu::LoadOp::Clear : wgpu::LoadOp::Load;
	// depthStencilAttachment.stencilStoreOp = wgpu::StoreOp::Store;
	depthStencilAttachment.stencilLoadOp = wgpu::LoadOp::Undefined;
	depthStencilAttachment.stencilStoreOp = wgpu::StoreOp::Undefined;
	depthStencilAttachment.stencilReadOnly = true;

	renderPassDesc.colorAttachmentCount = 1;
	renderPassDesc.colorAttachments = &renderPassColorAttachment;
	renderPassDesc.depthStencilAttachment = &depthStencilAttachment;
	renderPassDesc.timestampWrites = nullptr;

	// Create the render pass and end it immediately (we only clear the screen but do not draw anything)
	wgpu::RenderPassEncoder renderPass = encoder.beginRenderPass(renderPassDesc);

	webgpu_frame_state out = {state, render_texture, depthTexture, depthTextureView, encoder, renderPass};
	out.get_current_view_projection_matrix() = glm::identity<glm::mat4x4>();
	return out;
}

WAYLIB_OPTIONAL(webgpu_frame_state) begin_drawing(webgpu_state state, WAYLIB_OPTIONAL(color) clear_color /*= {}*/) {
	// Get the surface texture
	wgpu::SurfaceTexture surfaceTexture;
	state.surface.getCurrentTexture(&surfaceTexture);
	if (surfaceTexture.status != wgpu::SurfaceGetCurrentTextureStatus::Success)
		return {};
	wgpu::Texture texture = surfaceTexture.texture;

	// Create a view for this surface texture
	wgpu::TextureViewDescriptor viewDescriptor;
	viewDescriptor.label = "Waylib Surface Texture View";
	viewDescriptor.format = texture.getFormat();
	viewDescriptor.dimension = wgpu::TextureViewDimension::_2D;
	viewDescriptor.baseMipLevel = 0;
	viewDescriptor.mipLevelCount = 1;
	viewDescriptor.baseArrayLayer = 0;
	viewDescriptor.arrayLayerCount = 1;
	viewDescriptor.aspect = wgpu::TextureAspect::All;

	return begin_drawing_render_texture(state, texture.createView(viewDescriptor), {texture.getWidth(), texture.getHeight()}, clear_color);
}

void end_drawing(webgpu_frame_state& frame) {
	frame.render_pass.end();
	frame.render_pass.release();

	// Finally encode and submit the render pass
	wgpu::CommandBufferDescriptor cmdBufferDescriptor = {};
	cmdBufferDescriptor.label = "Waylib Frame Command buffer";
	wgpu::CommandBuffer command = frame.encoder.finish(cmdBufferDescriptor);
	frame.encoder.release();

	// std::cout << "Submitting command..." << std::endl;
	frame.state.device.getQueue().submit(1, &command);
	command.release();
	// std::cout << "Command submitted." << std::endl;

#ifndef __EMSCRIPTEN__
	frame.state.surface.present();
#endif

	// Process callbacks
	frame.state.device.tick();
}

#ifndef WAYLIB_NO_CAMERAS
mat4x4f camera3D_get_matrix(camera3D& camera, vec2i window_dimensions) {
	mat4x4f V = glm::lookAt(camera.position, camera.target, camera.up);
	mat4x4f P;
	if(camera.orthographic) {
		float right = camera.field_of_view / 2;
		float top = right * window_dimensions.y / window_dimensions.x;
		P = glm::ortho<float>(-right, right, -top, top, camera.near_clip_distance, camera.far_clip_distance);
	} else
		P = glm::perspectiveFov<float>(camera.field_of_view, window_dimensions.x, window_dimensions.y, camera.near_clip_distance, camera.far_clip_distance);
	return P * V;
}
mat4x4f camera3D_get_matrix(camera3D* camera, vec2i window_dimensions) { return camera3D_get_matrix(*camera, window_dimensions); }

mat4x4f camera2D_get_matrix(camera2D& camera, vec2i window_dimensions) {
	window_dimensions /= std::abs(camera.zoom) < .01 ? 2 : 2 * camera.zoom;
	vec3f position = {camera.target.x, camera.target.y, -.1};
	vec4f up = {0, 1, 0, 0};
	up = glm::rotate(glm::identity<glm::mat4x4>(), camera.rotation.radian_value(), vec3f{0, 0, 1}) * up;
	auto P = glm::ortho<float>(-window_dimensions.x, window_dimensions.x, -window_dimensions.y, window_dimensions.y, camera.near_clip_distance, camera.far_clip_distance);
	auto V = glm::lookAt(position, position + vec3f{0, 0, 1}, up.xyz());
	return P * V;
}
mat4x4f camera2D_get_matrix(camera2D* camera, vec2i window_dimensions) { return camera2D_get_matrix(*camera, window_dimensions); }

void begin_camera_mode3D(webgpu_frame_state& frame, camera3D& camera, vec2i window_dimensions) {
	frame.get_current_view_projection_matrix() = camera3D_get_matrix(camera, window_dimensions);
}
void begin_camera_mode3D(webgpu_frame_state* frame, camera3D* camera, vec2i window_dimensions) {
	begin_camera_mode3D(*frame, *camera, window_dimensions);
}

void begin_camera_mode2D(webgpu_frame_state& frame, camera2D& camera, vec2i window_dimensions) {
	frame.get_current_view_projection_matrix() = camera2D_get_matrix(camera, window_dimensions);
}
void begin_camera_mode2D(webgpu_frame_state* frame, camera2D* camera, vec2i window_dimensions) {
	begin_camera_mode2D(*frame, *camera, window_dimensions);
}

void end_camera_mode(webgpu_frame_state& frame) {
	frame.get_current_view_projection_matrix() = glm::identity<glm::mat4x4>();
}
void end_camera_mode(webgpu_frame_state* frame) {
	end_camera_mode(*frame);
}
#endif // WAYLIB_NO_CAMERAS

#ifdef WAYLIB_NAMESPACE_NAME
}
#endif