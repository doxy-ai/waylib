#define WEBGPU_CPP_IMPLEMENTATION
#include "waylib.hpp"

#include <chrono>
#include <fstream>

#define TCPP_IMPLEMENTATION
#include "thirdparty/tcppLibrary.hpp"

#ifdef WAYLIB_NAMESPACE_NAME
namespace WAYLIB_NAMESPACE_NAME {
#endif

wgpu::Color to_webgpu(const color8bit& color) {
	wgpu::Color out;
	out.r = float(color.r) / 255;
	out.g = float(color.g) / 255;
	out.b = float(color.b) / 255;
	out.a = float(color.a) / 255;
	return out;
}

wgpu::Color to_webgpu(const color32bit& color) {
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

	if(std::abs(time.average_delta) < .001) time.average_delta = time.delta;
	time.average_delta = time.average_delta * alpha + time.delta * (1 - alpha);

	last = now;
}
void time_calculations(time* time) { time_calculations(*time); }

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

WAYLIB_OPTIONAL(webgpu_frame_state) begin_drawing_render_texture(webgpu_state state, WGPUTextureView render_texture, WAYLIB_OPTIONAL(color8bit) clear_color /*= {}*/) {
	// Create a command encoder for the draw call
	wgpu::CommandEncoderDescriptor encoderDesc = {};
	encoderDesc.label = "Waylib Command Encoder";
	wgpu::CommandEncoder encoder = state.device.createCommandEncoder(encoderDesc);

	// Create the render pass that clears the screen with our color
	wgpu::RenderPassDescriptor renderPassDesc = {};

	// The attachment part of the render pass descriptor describes the target texture of the pass
	wgpu::RenderPassColorAttachment renderPassColorAttachment = {};
	renderPassColorAttachment.view = render_texture;
	renderPassColorAttachment.resolveTarget = nullptr;
	renderPassColorAttachment.loadOp = clear_color.has_value ? wgpu::LoadOp::Clear : wgpu::LoadOp::Load;
	renderPassColorAttachment.storeOp = wgpu::StoreOp::Store;
	renderPassColorAttachment.clearValue = to_webgpu(clear_color.has_value ? clear_color.value : color8bit{0, 0, 0, 0});
	renderPassColorAttachment.depthSlice = WGPU_DEPTH_SLICE_UNDEFINED;

	renderPassDesc.colorAttachmentCount = 1;
	renderPassDesc.colorAttachments = &renderPassColorAttachment;
	renderPassDesc.depthStencilAttachment = nullptr;
	renderPassDesc.timestampWrites = nullptr;

	// Create the render pass and end it immediately (we only clear the screen but do not draw anything)
	wgpu::RenderPassEncoder renderPass = encoder.beginRenderPass(renderPassDesc);
	return webgpu_frame_state{render_texture, encoder, renderPass};
}

WAYLIB_OPTIONAL(webgpu_frame_state) begin_drawing(webgpu_state state, WAYLIB_OPTIONAL(color8bit) clear_color /*= {}*/) {
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

	return begin_drawing_render_texture(state, texture.createView(viewDescriptor), clear_color);
}

void end_drawing(webgpu_state state, webgpu_frame_state frame) {
	frame.render_pass.end();
	frame.render_pass.release();

	// Finally encode and submit the render pass
	wgpu::CommandBufferDescriptor cmdBufferDescriptor = {};
	cmdBufferDescriptor.label = "Waylib Frame Command buffer";
	wgpu::CommandBuffer command = frame.encoder.finish(cmdBufferDescriptor);
	frame.encoder.release();

	// std::cout << "Submitting command..." << std::endl;
	state.device.getQueue().submit(1, &command);
	command.release();
	// std::cout << "Command submitted." << std::endl;

#ifndef __EMSCRIPTEN__
	state.surface.present();
#endif

	// Process callbacks
	state.device.tick();
}

#ifdef WAYLIB_NAMESPACE_NAME
}
#endif