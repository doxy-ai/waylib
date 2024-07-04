#include "waylib.hpp"

#include <chrono>
#include <limits>
#include <glm/ext/matrix_transform.hpp>
#include <glm/ext/matrix_clip_space.hpp>

#define OPEN_URL_NAMESPACE wl_detail
#include "thirdparty/open.hpp"

#ifdef WAYLIB_NAMESPACE_NAME
namespace WAYLIB_NAMESPACE_NAME {
#endif

// Defined in common.cpp
pipeline_globals& create_pipeline_globals(wgpu_state state);

//////////////////////////////////////////////////////////////////////
// #Miscelanious
//////////////////////////////////////////////////////////////////////

wgpu::Color to_webgpu(const color& color) {
	return {color.r, color.g, color.b, color.a};
}

void time_calculations(time& time) WAYLIB_TRY {
	constexpr static float alpha = .9;
	static std::chrono::system_clock::time_point last = std::chrono::system_clock::now();

	auto now = std::chrono::system_clock::now();
	time.delta = std::chrono::duration_cast<std::chrono::microseconds>(now - last).count() / 1000000.0;
	time.since_start += time.delta;

	if(std::abs(time.average_delta) < 2 * std::numeric_limits<float>::epsilon()) time.average_delta = time.delta;
	time.average_delta = time.average_delta * alpha + time.delta * (1 - alpha);

	last = now;
} WAYLIB_CATCH()
void time_calculations(time* time) { time_calculations(*time); }

void time_upload(wgpu_frame_state& frame, time& time) WAYLIB_TRY {
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
} WAYLIB_CATCH()
void time_upload(wgpu_frame_state* frame, time* time) {
	time_upload(*frame, *time);
}

bool open_url(const char* url) WAYLIB_TRY {
	return wl_detail::open_url(url);
} WAYLIB_CATCH(false)

void process_wgpu_events([[maybe_unused]] Device device, [[maybe_unused]] bool yieldToWebBrowser = true) WAYLIB_TRY {
#if defined(WEBGPU_BACKEND_DAWN)
    device.tick();
// #elif defined(WEBGPU_BACKEND_WGPU)
//     device.poll(false);
#elif defined(WEBGPU_BACKEND_EMSCRIPTEN)
    if (yieldToWebBrowser) {
        emscripten_sleep(10);
    }
#endif
} WAYLIB_CATCH()

//////////////////////////////////////////////////////////////////////
// #WebGPU Defaults
//////////////////////////////////////////////////////////////////////

wgpu::Device create_default_device_from_instance(WGPUInstance instance, WGPUSurface surface /*= nullptr*/, bool prefer_low_power /*= false*/) WAYLIB_TRY {
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
} WAYLIB_CATCH(nullptr)

wgpu::Device create_default_device(WGPUSurface surface /*= nullptr*/, bool prefer_low_power /*= false*/) {
	return create_default_device_from_instance(wgpuCreateInstance({}), surface, prefer_low_power);
}

void release_device(WGPUDevice device, bool also_release_adapter /*= true*/, bool also_release_instance /*= true*/) WAYLIB_TRY {
	wgpu::Adapter adapter = WAYLIB_C_TO_CPP_CONVERSION(wgpu::Device, device).getAdapter();
	wgpu::Instance instance = adapter.getInstance();
	wgpuDeviceRelease(device);
	if(also_release_adapter) wgpuAdapterRelease(adapter);
	if(also_release_instance) wgpuInstanceRelease(instance);
} WAYLIB_CATCH()

void release_wgpu_state(wgpu_state state) WAYLIB_TRY {
	state.device.getQueue().release();
	state.surface.release();
	release_device(state.device, true, true);
} WAYLIB_CATCH()

bool configure_surface(wgpu_state state, vec2i size, surface_configuration config /*= {}*/) WAYLIB_TRY {
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
} WAYLIB_CATCH(false)

//////////////////////////////////////////////////////////////////////
// #Shader (Preprocessor)
//////////////////////////////////////////////////////////////////////

namespace detail {
	// Defined in shader_preprocessor.cpp
	std::optional<std::string> preprocess_shader_from_memory(shader_preprocessor* processor, const std::string& _data, const preprocess_shader_config& config);
	std::optional<std::string> preprocess_shader_from_memory_and_cache(shader_preprocessor* processor, const std::string& _data, const std::filesystem::path& path, preprocess_shader_config config);
	std::optional<std::string> preprocess_shader(shader_preprocessor* processor, const std::filesystem::path& path, const preprocess_shader_config& config);
	void preprocessor_initalize_platform_defines(shader_preprocessor* preprocessor, wgpu::Adapter adapter);
}

shader_preprocessor* create_shader_preprocessor() WAYLIB_TRY {
	return new shader_preprocessor();
} WAYLIB_CATCH(nullptr)

void release_shader_preprocessor(shader_preprocessor* processor) WAYLIB_TRY {
	delete processor;
} WAYLIB_CATCH()

shader_preprocessor* preprocessor_initialize_virtual_filesystem(shader_preprocessor* processor, wgpu_state state, preprocess_shader_config config /*= {}*/) WAYLIB_TRY {
	if(processor->defines.find("WGPU_ADAPTER_TYPE") == std::string::npos)
		preprocessor_initalize_platform_defines(processor, state);

	preprocess_shader_from_memory_and_cache(processor,
#include "shaders/waylib/time.wgsl"
		, "waylib/time", config);
	preprocess_shader_from_memory_and_cache(processor,
#include "shaders/waylib/instance.wgsl"
		, "waylib/instance", config);
	preprocess_shader_from_memory_and_cache(processor,
#include "shaders/waylib/vertex.wgsl"
		, "waylib/vertex", config);
	preprocess_shader_from_memory_and_cache(processor,
#include "shaders/waylib/wireframe.wgsl"
		, "waylib/wireframe", config);
	return processor;
} WAYLIB_CATCH(nullptr)

shader_preprocessor* preprocessor_add_define(shader_preprocessor* processor, const char* name, const char* value) WAYLIB_TRY {
	processor->defines += "#define " + std::string(name) + " " + value + "\n";
	return processor;
} WAYLIB_CATCH(nullptr)

bool preprocessor_add_search_path(shader_preprocessor* processor, const char* _path) WAYLIB_TRY {
	auto path = std::filesystem::absolute(_path);
	if(!std::filesystem::exists(path)) return false;

	processor->search_paths.emplace(path);
	return true;
} WAYLIB_CATCH(false)

shader_preprocessor* preprocessor_initalize_platform_defines(shader_preprocessor* processor, wgpu_state state) WAYLIB_TRY {
	detail::preprocessor_initalize_platform_defines(processor, state.device.getAdapter());
	return processor;
} WAYLIB_CATCH(nullptr)

const char* preprocessor_get_cached_file(shader_preprocessor* processor, const char* path) WAYLIB_TRY {
	static std::string cache;
	if(processor->file_cache.find(path) != processor->file_cache.end())
		return (cache = processor->file_cache[path]).c_str();
	return nullptr;
} WAYLIB_CATCH(nullptr)

const char* preprocess_shader_from_memory(shader_preprocessor* processor, const char* data, preprocess_shader_config config /*= {}*/) WAYLIB_TRY {
	static std::string cache;
	if(auto res = detail::preprocess_shader_from_memory(processor, data, config); res)
		return (cache = std::move(*res)).c_str();
	return nullptr;
} WAYLIB_CATCH(nullptr)

const char* preprocess_shader_from_memory_and_cache(shader_preprocessor* processor, const char* data, const char* path, preprocess_shader_config config /*= {}*/) WAYLIB_TRY {
	static std::string cache;
	if(auto res = detail::preprocess_shader_from_memory_and_cache(processor, data, path, config); res)
		return (cache = std::move(*res)).c_str();
	return nullptr;
} WAYLIB_CATCH(nullptr)

const char* preprocess_shader(shader_preprocessor* processor, const char* path, preprocess_shader_config config /*= {}*/) WAYLIB_TRY {
	static std::string cache;
	if(auto res = detail::preprocess_shader(processor, path, config); res)
		return (cache = std::move(*res)).c_str();
	return nullptr;
} WAYLIB_CATCH(nullptr)

WAYLIB_OPTIONAL(shader) create_shader(wgpu_state state, const char* wgsl_source_code, create_shader_configuration config /*= {}*/) WAYLIB_TRY {
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
} WAYLIB_CATCH({})

//////////////////////////////////////////////////////////////////////
// #Begin/End Drawing
//////////////////////////////////////////////////////////////////////

WAYLIB_OPTIONAL(wgpu_frame_state) begin_drawing_render_texture(wgpu_state state, WGPUTextureView render_texture, vec2i render_texture_dimensions, WAYLIB_OPTIONAL(color) clear_color /*= {}*/) WAYLIB_TRY {
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

	wgpu_frame_state out = {state, render_texture, depthTexture, depthTextureView, encoder, renderPass};
	out.get_current_view_projection_matrix() = glm::identity<glm::mat4x4>();
	return out;
} WAYLIB_CATCH({})

WAYLIB_OPTIONAL(wgpu_frame_state) begin_drawing(wgpu_state state, WAYLIB_OPTIONAL(color) clear_color /*= {}*/) WAYLIB_TRY {
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
} WAYLIB_CATCH({})

void end_drawing(wgpu_frame_state& frame) WAYLIB_TRY {
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
} WAYLIB_CATCH()

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

void begin_camera_mode3D(wgpu_frame_state& frame, camera3D& camera, vec2i window_dimensions) {
	frame.get_current_view_projection_matrix() = camera3D_get_matrix(camera, window_dimensions);
}
void begin_camera_mode3D(wgpu_frame_state* frame, camera3D* camera, vec2i window_dimensions) {
	begin_camera_mode3D(*frame, *camera, window_dimensions);
}

void begin_camera_mode2D(wgpu_frame_state& frame, camera2D& camera, vec2i window_dimensions) {
	frame.get_current_view_projection_matrix() = camera2D_get_matrix(camera, window_dimensions);
}
void begin_camera_mode2D(wgpu_frame_state* frame, camera2D* camera, vec2i window_dimensions) {
	begin_camera_mode2D(*frame, *camera, window_dimensions);
}

void end_camera_mode(wgpu_frame_state& frame) {
	frame.get_current_view_projection_matrix() = glm::identity<glm::mat4x4>();
}
void end_camera_mode(wgpu_frame_state* frame) {
	end_camera_mode(*frame);
}
#endif // WAYLIB_NO_CAMERAS

#ifdef WAYLIB_NAMESPACE_NAME
}
#endif