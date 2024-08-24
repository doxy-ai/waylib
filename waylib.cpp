#include "waylib.hpp"
#include "common.h"
#include "config.h"
#include "waylib.h"

#include <cassert>
#include <chrono>
#include <cstring>
#include <glm/ext/quaternion_common.hpp>
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

void time_calculations(frame_time& frame_time) WAYLIB_TRY {
	constexpr static float alpha = .9;
	static std::chrono::system_clock::time_point last = std::chrono::system_clock::now();

	auto now = std::chrono::system_clock::now();
	frame_time.delta = std::chrono::duration_cast<std::chrono::microseconds>(now - last).count() / 1000000.0;
	frame_time.since_start += frame_time.delta;

	if(std::abs(frame_time.average_delta) < 2 * std::numeric_limits<float>::epsilon()) frame_time.average_delta = frame_time.delta;
	frame_time.average_delta = frame_time.average_delta * alpha + frame_time.delta * (1 - alpha);

	last = now;
} WAYLIB_CATCH()
void time_calculations(frame_time* frame_time) { time_calculations(*frame_time); }

bool open_url(const char* url) WAYLIB_TRY {
	return wl_detail::open_url(url);
} WAYLIB_CATCH(false)

wgpu::Buffer get_zero_buffer(wgpu_state state, size_t size) {
	static WGPUBufferDescriptor bufferDesc = {
		.label = "Waylib Zero Buffer",
		.usage = wgpu::BufferUsage::CopyDst | wgpu::BufferUsage::Storage | wgpu::BufferUsage::Uniform,
		.size = 10,
		.mappedAtCreation = false,
	};
	static wgpu::Buffer zeroBuffer;

	if(!zeroBuffer || size > bufferDesc.size) {
		bufferDesc.size = size;
		std::vector<std::byte> zeroData(bufferDesc.size, std::byte{0});
		if(zeroBuffer) {
			zeroBuffer.destroy();
			zeroBuffer.release();
		}
		zeroBuffer = state.device.createBuffer(bufferDesc);
		state.device.getQueue().writeBuffer(zeroBuffer, 0, zeroData.data(), zeroData.size());
	}
	return zeroBuffer;
};

void upload_utility_data(wgpu_frame_state& frame, WAYLIB_OPTIONAL(camera_upload_data&) camera, std::span<light> lights, WAYLIB_OPTIONAL(frame_time) frame_time) WAYLIB_TRY {
	static WGPUBufferDescriptor cameraBufferDesc = {
		.label = "Waylib Camera Buffer",
		.usage = wgpu::BufferUsage::CopyDst | wgpu::BufferUsage::Uniform,
		.size = sizeof(camera_upload_data),
		.mappedAtCreation = false,
	};
	static WGPUBufferDescriptor lightBufferDesc = {
		.label = "Waylib Light Buffer",
		.usage = wgpu::BufferUsage::CopyDst | wgpu::BufferUsage::Storage,
		// .size = sizeof(light),
		.mappedAtCreation = false,
	};
	static WGPUBufferDescriptor timeBufferDesc = {
		.label = "Waylib Time Buffer",
		.usage = /*wgpu::BufferUsage::CopyDst |*/ wgpu::BufferUsage::Uniform,
		.size = sizeof(frame_time),
		.mappedAtCreation = true,
	};

	static std::array<WGPUBindGroupEntry, 3> bindings = {WGPUBindGroupEntry{
		.binding = 0,
		// .buffer = cameraBuffer,
		.offset = 0,
		.size = sizeof(camera_upload_data)
	}, WGPUBindGroupEntry{
		.binding = 1,
		// .buffer = lightBuffer,
		.offset = 0,
		// .size = sizeof(light)
	}, WGPUBindGroupEntry{
		.binding = 2,
		// .buffer = timeBuffer,
		.offset = 0,
		.size = sizeof(frame_time)
	}};
	static wgpu::BindGroupDescriptor bindGroupDesc = [&frame] {
		wgpu::BindGroupDescriptor bindGroupDesc = wgpu::Default;
		bindGroupDesc.layout = create_pipeline_globals(frame.state).bindGroupLayouts[1]; // Group 1 is utility data
		bindGroupDesc.entryCount = bindings.size();
		bindGroupDesc.entries = bindings.data();
		return bindGroupDesc;
	}();

	if(camera.has_value) {
		wgpu::Buffer cameraBuffer = frame.state.device.createBuffer(cameraBufferDesc); WAYLIB_RELEASE_BUFFER_AT_FRAME_END(frame, cameraBuffer);
		frame.state.device.getQueue().writeBuffer(cameraBuffer, 0, &camera.value, sizeof(camera_upload_data));
		bindings[0].buffer = cameraBuffer;
	} else bindings[0].buffer = get_zero_buffer(frame.state, sizeof(camera_upload_data));

	if(!lights.empty()) {
		lightBufferDesc.size = sizeof(light) * lights.size();
		wgpu::Buffer lightBuffer = frame.state.device.createBuffer(lightBufferDesc); WAYLIB_RELEASE_BUFFER_AT_FRAME_END(frame, lightBuffer);
		frame.state.device.getQueue().writeBuffer(lightBuffer, 0, lights.data(), lightBufferDesc.size);
		bindings[1].buffer = lightBuffer;
		bindings[1].size = lightBufferDesc.size;
	} else {
		bindings[1].buffer = get_zero_buffer(frame.state, sizeof(light));
		bindings[1].size = sizeof(light);
	}

	if(frame_time.has_value) {
		wgpu::Buffer timeBuffer = frame.state.device.createBuffer(timeBufferDesc); WAYLIB_RELEASE_BUFFER_AT_FRAME_END(frame, timeBuffer);
		// frame.state.device.getQueue().writeBuffer(frame_timeBuffer, 0, &frame_time.value, sizeof(frame_time));
		std::memcpy(timeBuffer.getMappedRange(0, sizeof(frame_time)), &frame_time.value, sizeof(frame_time)); timeBuffer.unmap();
		bindings[2].buffer = timeBuffer;
	} else bindings[2].buffer = get_zero_buffer(frame.state, sizeof(frame_time));

	auto bindGroup = frame.state.device.createBindGroup(bindGroupDesc); WAYLIB_RELEASE_AT_FRAME_END(frame, bindGroup);
	frame.render_pass.setBindGroup(1, bindGroup, 0, nullptr);
} WAYLIB_CATCH()
void upload_utility_data(wgpu_frame_state* frame, WAYLIB_OPTIONAL(camera_upload_data*) data, light* lights, size_t light_count, WAYLIB_OPTIONAL(frame_time) frame_time) {
	WAYLIB_OPTIONAL(camera_upload_data&) camera = data.has_value ? *data.value : WAYLIB_OPTIONAL(camera_upload_data&){};
	upload_utility_data(*frame, camera, {lights, light_count}, frame_time);
}

//////////////////////////////////////////////////////////////////////
// #WebGPU Defaults
//////////////////////////////////////////////////////////////////////

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

template<typename T>
wgpu::IndexFormat calculate_index_format() {
	static_assert(sizeof(T) == 2 || sizeof(T) == 4, "Index types must either be u16 or u32 convertible.");

	if constexpr(sizeof(T) == 2)
		return wgpu::IndexFormat::Uint16;
	else return wgpu::IndexFormat::Uint32;
}

wgpu::TextureFormat surface_preferred_format(wgpu_state state) {
	wgpu::SurfaceCapabilities capabilities;
	state.surface.getCapabilities(state.adapter, &capabilities); // TODO: Always returns error?
	return capabilities.formats[0];
}

wgpu_state create_default_state_from_instance(WGPUInstance instance, WGPUSurface surface /*= nullptr*/, bool prefer_low_power /*= false*/) WAYLIB_TRY {
	wgpu::RequestAdapterOptions adapterOpts = {};
	adapterOpts.compatibleSurface = surface;
	adapterOpts.powerPreference = prefer_low_power ? wgpu::PowerPreference::LowPower : wgpu::PowerPreference::HighPerformance;
	wgpu::Adapter adapter = WAYLIB_C_TO_CPP_CONVERSION(wgpu::instance, instance).requestAdapter(adapterOpts);
	// std::cout << "Got adapter: " << adapter << std::endl;

	// std::cout << "Requesting device..." << std::endl;
	WGPUFeatureName float32filterable = WGPUFeatureName_Float32Filterable;
	wgpu::DeviceDescriptor deviceDesc = {};
	deviceDesc.label = "Waylib Device";
	deviceDesc.requiredFeatureCount = 1;
	deviceDesc.requiredFeatures = &float32filterable;
	deviceDesc.requiredLimits = nullptr;
	deviceDesc.defaultQueue.nextInChain = nullptr;
	deviceDesc.defaultQueue.label = "Waylib Queue";
#ifndef __EMSCRIPTEN__
	deviceDesc.deviceLostCallbackInfo.callback = [](WGPUDevice const* device, WGPUDeviceLostReason reason, char const* message, void* userdata) {
		std::cerr << "[WAYLIB] Device " << wgpu::Device{*device} << " lost: reason " << wgpu::DeviceLostReason{reason};
		if (message) std::cerr << " (" << message << ")";
		std::cerr << std::endl;
	};
	deviceDesc.uncapturedErrorCallbackInfo.callback = [](WGPUErrorType type, char const* message, void* userdata) {
		std::cerr << "[WAYLIB] Uncaptured device error: type " << wgpu::ErrorType{type};
		if (message) std::cerr << " (" << message << ")";
		std::cerr << std::endl;
	};
#endif
	return {
		.instance = instance,
		.adapter = adapter,
		.device = adapter.requestDevice(deviceDesc),
		.surface = surface
	};
} WAYLIB_CATCH({})

wgpu_state create_default_state(WGPUSurface surface /*= nullptr*/, bool prefer_low_power /*= false*/) WAYLIB_TRY {
	return create_default_state_from_instance(wgpuCreateInstance({}), surface, prefer_low_power);
} WAYLIB_CATCH({})

void release_wgpu_state(wgpu_state state, bool release_adapter /*= true*/, bool release_instance /*= true*/) WAYLIB_TRY {
	state.device.getQueue().release();
	state.surface.release();
	state.device.release();
	if(release_adapter) state.adapter.release();
	if(release_instance) state.instance.release();
} WAYLIB_CATCH()

// Returns true if the request present_mode is supported... returns false if we fall back fifo (can be confirmed its not just a memory allocation failure by making sure the first word of the error message is "desired")
bool configure_surface(wgpu_state state, vec2i size, surface_configuration config /*= {}*/) WAYLIB_TRY {
	// Configure the surface
	wgpu::SurfaceConfiguration descriptor = {};

	wgpu::SurfaceCapabilities capabilities;
	state.surface.getCapabilities(state.adapter, &capabilities); // TODO: Always returns error?

	bool found = false;
	for(size_t i = 0; i < capabilities.presentModeCount; ++i)
		if(capabilities.presentModes[i] == config.presentation_mode) {
			found = true;
			break;
		}
	if(!found) {
		set_error_message_raw("Desired present mode not available... Falling back to First in First out.");
		config.presentation_mode = wgpu::PresentMode::Fifo; // Fifo
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
	descriptor.presentMode = config.presentation_mode;
	descriptor.alphaMode = config.alpha_mode;

	state.surface.configure(descriptor);
	return found;
} WAYLIB_CATCH(false)

//////////////////////////////////////////////////////////////////////
// #Shader (Preprocessor)
//////////////////////////////////////////////////////////////////////

namespace detail {
	// Defined in shader_preprocessor.cpp
	std::optional<std::string> preprocess_shader_from_memory(shader_preprocessor* processor, const std::string& _data, const preprocess_shader_config& config);
	std::optional<std::string> preprocess_shader_from_memory_and_cache(shader_preprocessor* processor, const std::string& _data, const std::filesystem::path& path, preprocess_shader_config config);
	std::optional<std::string> preprocess_shader(shader_preprocessor* processor, const std::filesystem::path& path, const preprocess_shader_config& config);
	void preprocessor_initialize_platform_defines(shader_preprocessor* preprocessor, wgpu::Adapter adapter);
}

shader_preprocessor* create_shader_preprocessor() WAYLIB_TRY {
	return new shader_preprocessor();
} WAYLIB_CATCH(nullptr)

void release_shader_preprocessor(shader_preprocessor* processor) WAYLIB_TRY {
	delete processor;
} WAYLIB_CATCH()

shader_preprocessor* preprocessor_initialize_virtual_filesystem(shader_preprocessor* processor, wgpu_state state, preprocess_shader_config config /*= {}*/) WAYLIB_TRY {
	if(processor->defines.find("WGPU_ADAPTER_TYPE") == std::string::npos)
		preprocessor_initialize_platform_defines(processor, state);

	preprocess_shader_from_memory_and_cache(processor,
#include "shaders/waylib/inverse.wgsl"
		, "waylib/inverse", config);
	preprocess_shader_from_memory_and_cache(processor,
#include "shaders/waylib/time.wgsl"
		, "waylib/time", config);
	preprocess_shader_from_memory_and_cache(processor,
#include "shaders/waylib/instance.wgsl"
		, "waylib/instance", config);
	preprocess_shader_from_memory_and_cache(processor,
#include "shaders/waylib/textures.wgsl"
		, "waylib/textures", config);
	preprocess_shader_from_memory_and_cache(processor,
#include "shaders/waylib/camera.wgsl"
		, "waylib/camera", config);
	preprocess_shader_from_memory_and_cache(processor,
#include "shaders/waylib/light.wgsl"
		, "waylib/light", config);
	preprocess_shader_from_memory_and_cache(processor,
#include "shaders/waylib/wireframe.wgsl"
		, "waylib/wireframe", config);
	preprocess_shader_from_memory_and_cache(processor,
#include "shaders/waylib/vertex.wgsl"
		, "waylib/vertex", config);
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

shader_preprocessor* preprocessor_initialize_platform_defines(shader_preprocessor* processor, wgpu_state state) WAYLIB_TRY {
	detail::preprocessor_initialize_platform_defines(processor, state.adapter);
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

namespace detail{
	std::pair<wgpu::RenderPipelineDescriptor, WAYLIB_OPTIONAL(wgpu::FragmentState)> shader_configure_render_pipeline_descriptor(wgpu_state state, shader& shader, wgpu::RenderPipelineDescriptor pipelineDesc = {}) {
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
			.format = surface_preferred_format(state),
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
		static std::array<WGPUVertexBufferLayout, 6> bufferLayouts = {positionLayout, texcoordLayout, normalLayout, colorLayout, tangentLayout, texcoord2Layout};

		if(shader.vertex_entry_point) {
			pipelineDesc.vertex.bufferCount = bufferLayouts.size();
			pipelineDesc.vertex.buffers = bufferLayouts.data();
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
// #Begin/End Drawing
//////////////////////////////////////////////////////////////////////

std::array<WGPUBindGroupEntry, 18> enumerate_texture_bindings(wgpu_frame_state& frame, std::span<WAYLIB_NULLABLE(texture*), WAYLIB_TEXTURE_SLOT_COUNT> textures) {
	static const texture& default_texture = *throw_if_null(get_default_texture(frame.state));
	static const texture& default_cube_texture = *throw_if_null(get_default_cube_texture(frame.state));
	static std::array<WGPUBindGroupEntry, 18> bindings = [] {
		std::array<WGPUBindGroupEntry, 18> bindings = {WGPUBindGroupEntry{
			.binding = 1,
			.textureView = default_texture.view,
		}, WGPUBindGroupEntry{
			.binding = 2,
			.sampler = default_texture.sampler,
		}};
		for(size_t i = 2; i < bindings.size(); i += 2) {
			bindings[i + 0] = bindings[0];
			bindings[i + 0].binding = i + 1;
			bindings[i + 1] = bindings[1];
			bindings[i + 1].binding = i + 2;
		}
		return bindings;
	}();

	for(size_t i = 0; i < textures.size(); ++i)
		if(textures[i]) {
			auto& texture = *textures[i];
			bindings[2 * i + 0].textureView = texture.view;
			bindings[2 * i + 1].sampler = texture.sampler;
		} else {
			bindings[2 * i + 0].textureView = i == (size_t)texture_slot::Cubemap ? default_cube_texture.view : default_texture.view;
			// bindings[2 * i + 1].sampler = i == (size_t)texture_slot::Cubemap ? default_cube_texture.sampler : default_texture.sampler;
			bindings[2 * i + 1].sampler = default_texture.sampler;
		}

	return bindings;
}

void setup_default_bindings(wgpu_frame_state& frame) {
	static std::array<WGPUBindGroupEntry, 19> bindings = [&frame]{
		size_t size = create_pipeline_globals(frame.state).min_buffer_size;
		std::array<WGPUBindGroupEntry, 19> bindings{WGPUBindGroupEntry{
			.binding = 0,
			.buffer = get_zero_buffer(frame.state, size),
			.offset = 0,
			.size = size,
		}};
		auto null = std::array<WAYLIB_NULLABLE(texture*), WAYLIB_TEXTURE_SLOT_COUNT>{}; null.fill(nullptr);
		std::array<WGPUBindGroupEntry, 18> textureBindings = enumerate_texture_bindings(frame, null);
		std::move(textureBindings.begin(), textureBindings.end(), bindings.begin() + 1);
		return bindings;
	}();
	static wgpu::BindGroup defaultBindGroup = [&frame] {
		wgpu::BindGroupDescriptor bindGroupDesc = wgpu::Default;
		bindGroupDesc.layout = create_pipeline_globals(frame.state).bindGroupLayouts[0];
		bindGroupDesc.entryCount = bindings.size();
		bindGroupDesc.entries = bindings.data();
		return frame.state.device.createBindGroup(bindGroupDesc);
	}();

	// Bind instance and texture data
	frame.render_pass.setBindGroup(0, defaultBindGroup, 0, nullptr);

	// Passing nothing to upload_utility_data binds everything to zero!
	upload_utility_data(frame, {}, {}, {});
}

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
	static wgpu_frame_finalizers finalizers;

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

	wgpu_frame_state out = {state, render_texture, depthTexture, depthTextureView, encoder, renderPass, &finalizers};
	setup_default_bindings(out);
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
} WAYLIB_CATCH()
void end_drawing(wgpu_frame_state* frame) { end_drawing(*frame); }

void present_frame(wgpu_frame_state& frame) WAYLIB_TRY {
#ifndef __EMSCRIPTEN__
	frame.state.surface.present();
#endif

	// Call all of the finalizers
	for(auto& finalizer: *frame.finalizers)
		finalizer();
	frame.finalizers->clear();

	// Process callbacks
	process_wgpu_events(frame.state.device);
} WAYLIB_CATCH()
void present_frame(wgpu_frame_state* frame) { present_frame(*frame); }

//////////////////////////////////////////////////////////////////////
// #Camera
//////////////////////////////////////////////////////////////////////

#ifndef WAYLIB_NO_CAMERAS
std::pair<mat4x4f, mat4x4f> camera3D_get_matrix_impl(camera3D& camera, vec2i window_dimensions) {
	mat4x4f V = glm::lookAt(camera.position, camera.target, camera.up);
	mat4x4f P;
	if(camera.orthographic) {
		float right = camera.field_of_view / 2;
		float top = right * window_dimensions.y / window_dimensions.x;
		P = glm::ortho<float>(-right, right, -top, top, camera.near_clip_distance, camera.far_clip_distance);
	} else
		P = glm::perspectiveFov<float>(camera.field_of_view, window_dimensions.x, window_dimensions.y, camera.near_clip_distance, camera.far_clip_distance);
	return {V, P};
}
mat4x4f camera3D_get_matrix(camera3D& camera, vec2i window_dimensions) {
	auto [V, P] = camera3D_get_matrix_impl(camera, window_dimensions);
	return P * V;
}
mat4x4f camera3D_get_matrix(camera3D* camera, vec2i window_dimensions) { return camera3D_get_matrix(*camera, window_dimensions); }

std::pair<mat4x4f, mat4x4f> camera2D_get_matrix_impl(camera2D& camera, vec2i window_dimensions) {
	window_dimensions /= std::abs(camera.zoom) < .01 ? 1 : 1 * camera.zoom;
	vec3f position = {camera.target.x, camera.target.y, -1};
	vec4f up = {0, 1, 0, 0};
	up = glm::rotate(glm::identity<glm::mat4x4>(), camera.rotation.radian_value(), vec3f{0, 0, 1}) * up;
	auto P = glm::ortho<float>(0, window_dimensions.x, window_dimensions.y, 0, camera.near_clip_distance, camera.far_clip_distance);
	if(!camera.pixel_perfect) P = glm::ortho<float>(0, 1/camera.zoom, 1/camera.zoom, 0, camera.near_clip_distance, camera.far_clip_distance);
	auto V = glm::lookAt(position, position + vec3f{0, 0, 1}, up.xyz());
	return {V, P};
}
mat4x4f camera2D_get_matrix(camera2D& camera, vec2i window_dimensions) {
	auto [V, P] = camera2D_get_matrix_impl(camera, window_dimensions);
	return P * V;
}
mat4x4f camera2D_get_matrix(camera2D* camera, vec2i window_dimensions) { return camera2D_get_matrix(*camera, window_dimensions); }

void begin_camera_mode3D(wgpu_frame_state& frame, camera3D& camera, vec2i window_dimensions, std::span<light> lights /*={}*/, WAYLIB_OPTIONAL(frame_time) frame_time /*={}*/) WAYLIB_TRY {
	camera_upload_data data {
		.is_3D = true,
		.settings3D = camera,
		.window_dimensions = window_dimensions,
	};
	std::tie(data.get_view_matrix(), data.get_projection_matrix()) = camera3D_get_matrix_impl(camera, window_dimensions);
	upload_utility_data(frame, data, lights, frame_time);
} WAYLIB_CATCH()
void begin_camera_mode3D(wgpu_frame_state* frame, camera3D* camera, vec2i window_dimensions, light* lights /*= nullptr*/, size_t light_count /*=0*/, WAYLIB_OPTIONAL(frame_time) frame_time /*={}*/) {
	begin_camera_mode3D(*frame, *camera, window_dimensions, {lights, light_count}, frame_time);
}

void begin_camera_mode2D(wgpu_frame_state& frame, camera2D& camera, vec2i window_dimensions, std::span<light> lights /*={}*/, WAYLIB_OPTIONAL(frame_time) frame_time /*={}*/) WAYLIB_TRY {
	camera_upload_data data {
		.is_3D = true,
		.settings2D = camera,
		.window_dimensions = window_dimensions,
	};
	std::tie(data.get_view_matrix(), data.get_projection_matrix()) = camera2D_get_matrix_impl(camera, window_dimensions);
	upload_utility_data(frame, data, lights, frame_time);
} WAYLIB_CATCH()
void begin_camera_mode2D(wgpu_frame_state* frame, camera2D* camera, vec2i window_dimensions, light* lights /*= nullptr*/, size_t light_count /*=0*/, WAYLIB_OPTIONAL(frame_time) frame_time /*={}*/) {
	begin_camera_mode2D(*frame, *camera, window_dimensions, {lights, light_count}, frame_time);
}

void begin_camera_mode_identity(wgpu_frame_state& frame, vec2i window_dimensions, std::span<light> lights /*={}*/, WAYLIB_OPTIONAL(frame_time) frame_time /*={}*/) WAYLIB_TRY {
	camera_upload_data data {
		.is_3D = false,
		.settings3D = {},
		.settings2D = {},
		.window_dimensions = window_dimensions,
	};
	data.get_projection_matrix() = data.get_view_matrix() = glm::identity<glm::mat4x4>();
	upload_utility_data(frame, data, lights, frame_time);
} WAYLIB_CATCH()
void begin_camera_mode_identity(wgpu_frame_state* frame, vec2i window_dimensions, light* lights /*= nullptr*/, size_t light_count /*=0*/, WAYLIB_OPTIONAL(frame_time) frame_time /*={}*/) {
	begin_camera_mode_identity(*frame, window_dimensions);
}

void end_camera_mode(wgpu_frame_state& frame) {
	begin_camera_mode_identity(frame, vec2i(0), {}, {});
}
void end_camera_mode(wgpu_frame_state* frame) {
	end_camera_mode(*frame);
}
#endif // WAYLIB_NO_CAMERAS

//////////////////////////////////////////////////////////////////////
// #Mesh
//////////////////////////////////////////////////////////////////////

void mesh_upload(wgpu_state state, mesh& mesh) WAYLIB_TRY {
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
} WAYLIB_CATCH()
void mesh_upload(wgpu_state state, mesh* mesh) {
	mesh_upload(state, *mesh);
}

//////////////////////////////////////////////////////////////////////
// #Material
//////////////////////////////////////////////////////////////////////

pipeline_globals& create_pipeline_globals(wgpu_state state); // Declared in waylib.cpp

void material_upload(wgpu_state state, material& material, material_configuration config /*= {}*/) WAYLIB_TRY {
	// Create the render pipeline
	wgpu::RenderPipelineDescriptor pipelineDesc;
	wgpu::FragmentState fragment;
	for(size_t i = material.shaderCount; i--; ) { // Reverse iteration ensures that lower indexed shaders take precedence
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
	wgpu::DepthStencilState depthStencilState = wgpu::Default;
	// Keep a fragment only if its depth is lower than the previously blended one
	depthStencilState.depthCompare = config.depth_function.has_value ? config.depth_function.value : wgpu::CompareFunction::Undefined;
	// Each frame_time a fragment is blended into the target, we update the value of the Z-buffer
	depthStencilState.depthWriteEnabled = config.depth_function.has_value;
	// Store the format in a variable as later parts of the code depend on it
	depthStencilState.format = depth_texture_format;
	// Deactivate the stencil altogether
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
} WAYLIB_CATCH()
void material_upload(wgpu_state state, material* material, material_configuration config /*= {}*/) {
	material_upload(state, *material, config);
}

material create_material(wgpu_state state, shader* shaders, size_t shader_count, material_configuration config /*= {}*/) {
	material out {.shaderCount = (index_t)shader_count, .shaders = shaders};
	material_upload(state, out, config);
	return out;
}
material create_material(wgpu_state state, std::span<shader> shaders, material_configuration config /*= {}*/) {
	return create_material(state, shaders.data(), shaders.size(), config);
}
material create_material(wgpu_state state, shader& shader, material_configuration config /*= {}*/) {
	return create_material(state, &shader, 1, config);
}

//////////////////////////////////////////////////////////////////////
// #Model
//////////////////////////////////////////////////////////////////////

void model_upload(wgpu_state state, model& model) {
	for(size_t i = 0; i < model.mesh_count; ++i)
		mesh_upload(state, model.meshes[i]);
	// for(size_t i = 0; i < model.material_count; ++i) // TODO: How do we handle material configurations?
	// 	material_upload(state, model.materials[i]);
}
void model_upload(wgpu_state state, model* model) {
	model_upload(state, *model);
}

void model_draw_instanced(wgpu_frame_state& frame, model& model, std::span<model_instance_data> instances) WAYLIB_TRY {
	static WGPUBufferDescriptor bufferDesc = {
		.label = "Waylib Instance Buffer",
		.usage = wgpu::BufferUsage::CopyDst | wgpu::BufferUsage::Storage,
		.size = sizeof(model_instance_data),
		.mappedAtCreation = false,
	};
	static std::array<WGPUBindGroupEntry, 19> bindings = {WGPUBindGroupEntry{
		.binding = 0,
		.buffer = get_zero_buffer(frame.state, bufferDesc.size),
		.offset = 0,
		.size = sizeof(model_instance_data) * instances.size(),
	}};
	static WGPUBindGroupDescriptor bindGroupDesc {
		.label = "Waylib Per Model Bind Group",
		.layout = create_pipeline_globals(frame.state).bindGroupLayouts[0], // Group 0 is instance/texture data
		.entryCount = bindings.size(),
		.entries = bindings.data(),
	};

	if(instances.size() > 0) {
		bufferDesc.size = sizeof(model_instance_data) * instances.size();
		wgpu::Buffer instanceBuffer = frame.state.device.createBuffer(bufferDesc);
		frame.state.device.getQueue().writeBuffer(instanceBuffer, 0, instances.data(), sizeof(model_instance_data) * instances.size());
		WAYLIB_RELEASE_BUFFER_AT_FRAME_END(frame, instanceBuffer);

		bindings[0].buffer = instanceBuffer;
		bindings[0].size = sizeof(model_instance_data) * instances.size();
	} else {
		bindings[0].buffer = get_zero_buffer(frame.state, sizeof(model_instance_data));
		bindings[0].size = sizeof(model_instance_data);
	}

	for(size_t i = 0; i < model.mesh_count; ++i) {
		// Select which render pipeline to use
		auto& mat = model.materials[model.get_material_index_for_mesh(i)];
		frame.render_pass.setPipeline(mat.pipeline);
		// Figure out which textures to use from the material
		auto tex = enumerate_texture_bindings(frame, mat.get_textures());
		std::move(tex.begin(), tex.end(), bindings.begin() + 1);

		// Bind the instance and texture buffers
		wgpu::BindGroup bindGroup = frame.state.device.createBindGroup(bindGroupDesc); WAYLIB_RELEASE_AT_FRAME_END(frame, bindGroup);
		frame.render_pass.setBindGroup(0, bindGroup, 0, nullptr);

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
			frame.render_pass.setIndexBuffer(mesh.indexBuffer, calculate_index_format<index_t>(), 0, mesh.indexBuffer.getSize());
			frame.render_pass.drawIndexed(mesh.triangleCount * 3, std::max<size_t>(instances.size(), 1), 0, 0, 0);
		} else
			frame.render_pass.draw(model.meshes[i].vertexCount, std::max<size_t>(instances.size(), 1), 0, 0);
	}
} WAYLIB_CATCH()
void model_draw_instanced(wgpu_frame_state* frame, model* model, model_instance_data* instances, size_t instance_count) {
	model_draw_instanced(*frame, *model, {instances, instance_count});
}

void model_draw(wgpu_frame_state& frame, model& model) {
	model_instance_data instance = {model.transform, {}, {1, 1, 1, 1}};
	instance.get_inverse_transform() = glm::inverse(model.get_transform());
	model_draw_instanced(frame, model, {&instance, 1});
}
void model_draw(wgpu_frame_state* frame, model* model) {
	model_draw(*frame, *model);
}

//////////////////////////////////////////////////////////////////////
// #Texture
//////////////////////////////////////////////////////////////////////

WAYLIB_OPTIONAL(texture) create_texture(wgpu_state state, vec2i dimensions, texture_config config /*= {}*/) {
	if(config.cubemap) assert(config.frames % 6 == 0); // Cubemaps need to have a multiple of 6 images

	texture out{.cpu_data = nullptr, .heap_allocated = false};
	// Create the color texture
	wgpu::TextureDescriptor textureDesc = wgpu::Default;
	textureDesc.dimension = wgpu::TextureDimension::_2D;
	textureDesc.size = { static_cast<uint32_t>(dimensions.x), static_cast<uint32_t>(dimensions.y), static_cast<uint32_t>(config.frames) };
	textureDesc.mipLevelCount = config.mipmaps;
	textureDesc.sampleCount = 1;
	textureDesc.format = config.float_data ? wgpu::TextureFormat::RGBA32Float : wgpu::TextureFormat::RGBA8Unorm;
	textureDesc.usage = wgpu::TextureUsage::CopyDst | wgpu::TextureUsage::TextureBinding;
	textureDesc.viewFormatCount = 1;
	textureDesc.viewFormats = &textureDesc.format;
	out.gpu_data = state.device.createTexture(textureDesc);
	// std::cout << "Texture: " << out.gpu_data << std::endl;

	wgpu::TextureViewDescriptor textureViewDesc = wgpu::Default;
	textureViewDesc.aspect = wgpu::TextureAspect::All;
	textureViewDesc.baseArrayLayer = 0;
	textureViewDesc.arrayLayerCount = config.frames;
	textureViewDesc.baseMipLevel = 0;
	textureViewDesc.mipLevelCount = config.mipmaps;
	textureViewDesc.dimension = config.cubemap ? wgpu::TextureViewDimension::Cube : wgpu::TextureViewDimension::_2D;
	textureViewDesc.format = textureDesc.format;
	out.view = out.gpu_data.createView(textureViewDesc);
	// std::cout << "Texture view: " << out.view << std::endl;

	// Create a sampler
	wgpu::SamplerDescriptor samplerDesc;
	samplerDesc.addressModeU = config.address_mode;
	samplerDesc.addressModeV = config.address_mode;
	samplerDesc.addressModeW = config.address_mode;
	samplerDesc.magFilter = config.color_filter;
	samplerDesc.minFilter = config.color_filter;
	samplerDesc.mipmapFilter = config.mipmap_filter;
	samplerDesc.lodMinClamp = 0.0f;
	samplerDesc.lodMaxClamp = 8.0f;
	samplerDesc.compare = wgpu::CompareFunction::Undefined;
	samplerDesc.maxAnisotropy = 1;
	out.sampler = state.device.createSampler(samplerDesc);

	return out;
}

WAYLIB_OPTIONAL(texture) create_texture_from_image(wgpu_state state, image& image, texture_config config /*= {}*/) {
	config.float_data = image.float_data;
	config.frames = image.frames;
	config.mipmaps = image.mipmaps;
	auto _out = create_texture(state, {image.width, image.height}, config);
	if(!_out.has_value) return _out; auto out = _out.value;
	out.cpu_data = &image;

	// Upload texture data
	// Arguments telling which part of the texture we upload to
	// (together with the last argument of writeTexture)
	wgpu::ImageCopyTexture destination;
	destination.texture = out.gpu_data;
	destination.mipLevel = 0;
	destination.origin = { 0, 0, 0 }; // equivalent of the offset argument of Queue::writeBuffer
	destination.aspect = wgpu::TextureAspect::All; // only relevant for depth/Stencil textures

	// Arguments telling how the C++ side pixel memory is laid out
	wgpu::TextureDataLayout source;
	source.offset = 0;
	source.bytesPerRow = (image.float_data ? sizeof(color) : sizeof(color8)) * image.width;
	source.rowsPerImage = image.height;

	wgpu::Extent3D size = {static_cast<uint32_t>(image.width), static_cast<uint32_t>(image.height), static_cast<uint32_t>(image.frames)};
	state.device.getQueue().writeTexture(destination, image.data, image.width * image.height * (image.float_data ? sizeof(color) : sizeof(color8)) * image.frames, source, size);
	return out;
}
WAYLIB_OPTIONAL(texture) create_texture_from_image(wgpu_state state, image* image, texture_config config /*= {}*/) {
	return create_texture_from_image(state, *image, config);
}
WAYLIB_OPTIONAL(texture) create_texture_from_image(wgpu_state state, WAYLIB_OPTIONAL(image)&& image, texture_config config /*= {}*/) {
	if(!image.has_value) return {};
	auto imgPtr = new struct image(image.value);
	imgPtr->heap_allocated = true;
	return create_texture_from_image(state, imgPtr, config);
}

image texture_not_found_image(size_t dimensions) {
	image img;
	img.float_data = false;
	img.data = new color8[dimensions * dimensions];
	for (size_t i = 0; i < dimensions; ++i) {
		for (size_t j = 0; j < dimensions; ++j) {
			color8 *p = &img.data[j * dimensions + i];
			p->r = (float(i) / dimensions) * 255;
			p->g = (float(j) / dimensions) * 255;
			p->b = .5 * 255;
			p->a = 1 * 255;
		}
	}

	img.heap_allocated = true;
	img.mipmaps = img.frames = 1;
	img.height = img.width = dimensions;
	return img;
}

WAYLIB_OPTIONAL(const texture*) get_default_texture(wgpu_state state) {
	static image image = texture_not_found_image(16);
	static WAYLIB_OPTIONAL(texture) texture = create_texture_from_image(state, image, {.color_filter = wgpu::FilterMode::Nearest});

	if(texture.has_value) return &texture.value;
	return {};
}
WAYLIB_OPTIONAL(const texture*) get_default_cube_texture(wgpu_state state) {
	static image image = [] {
		std::array<WAYLIB_NAMESPACE_NAME::image, 6> faces; faces.fill(texture_not_found_image(16));
		return merge_images(faces).value;
	}();
	static WAYLIB_OPTIONAL(texture) texture = create_texture_from_image(state, image, {.color_filter = wgpu::FilterMode::Nearest, .cubemap = true});

	if(texture.has_value) return &texture.value;
	return {};
}

#ifdef WAYLIB_NAMESPACE_NAME
}
#endif