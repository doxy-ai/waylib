#include "waylib.hpp"
#include "common.h"
#include "common.hpp"
#include "config.h"
#include "waylib.h"

#include "thirdparty/mikktspace.h"

#include <cassert>
#include <chrono>
#include <cstring>
#include <glm/ext/quaternion_common.hpp>
#include <limits>
#include <glm/ext/matrix_transform.hpp>
#include <glm/ext/matrix_clip_space.hpp>
#include <thread>
#include <webgpu/webgpu.hpp>

#define OPEN_URL_NAMESPACE wl_detail
#include "thirdparty/open.hpp"

#ifdef WAYLIB_NAMESPACE_NAME
namespace WAYLIB_NAMESPACE_NAME {
#endif

// Defined in common.cpp
pipeline_globals& create_pipeline_globals(waylib_state state);

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

wgpu::Buffer get_zero_buffer(waylib_state state, size_t size) {
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
		// std::memcpy(zeroBuffer.getMappedRange(0, zeroData.size()), zeroData.data(), zeroData.size()); zeroBuffer.unmap();
	}
	return zeroBuffer;
};

void upload_utility_data(frame_state& frame, WAYLIB_OPTIONAL(camera_upload_data&) camera, std::span<light> lights, WAYLIB_OPTIONAL(frame_time) frame_time) WAYLIB_TRY {
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
		.usage = wgpu::BufferUsage::CopyDst | wgpu::BufferUsage::Uniform,
		.size = sizeof(frame_time),
		.mappedAtCreation = false,
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
		wgpu::Buffer cameraBuffer = frame.state.device.createBuffer(cameraBufferDesc);
		frame.state.device.getQueue().writeBuffer(cameraBuffer, 0, &camera.value, sizeof(camera_upload_data));
		bindings[0].buffer = cameraBuffer;
		frame_defer(frame) { cameraBuffer.destroy(); cameraBuffer.release(); };
	} else bindings[0].buffer = get_zero_buffer(frame.state, sizeof(camera_upload_data));

	if(!lights.empty()) {
		lightBufferDesc.size = sizeof(light) * lights.size();
		wgpu::Buffer lightBuffer = frame.state.device.createBuffer(lightBufferDesc);
		frame.state.device.getQueue().writeBuffer(lightBuffer, 0, lights.data(), lightBufferDesc.size);
		bindings[1].buffer = lightBuffer;
		bindings[1].size = lightBufferDesc.size;
		frame_defer(frame) { lightBuffer.destroy(); lightBuffer.release(); };
	} else {
		bindings[1].buffer = get_zero_buffer(frame.state, sizeof(light));
		bindings[1].size = sizeof(light);
	}

	if(frame_time.has_value) {
		wgpu::Buffer timeBuffer = frame.state.device.createBuffer(timeBufferDesc);
		frame.state.device.getQueue().writeBuffer(timeBuffer, 0, &frame_time.value, sizeof(frame_time.value));
		bindings[2].buffer = timeBuffer;
		frame_defer(frame) { timeBuffer.destroy(); timeBuffer.release(); };
	} else bindings[2].buffer = get_zero_buffer(frame.state, sizeof(frame_time.value));

	auto bindGroup = frame.state.device.createBindGroup(bindGroupDesc); frame_defer(frame) { bindGroup.release(); };
	frame.render_pass.setBindGroup(1, bindGroup, 0, nullptr);
} WAYLIB_CATCH()
void upload_utility_data(frame_state* frame, WAYLIB_OPTIONAL(camera_upload_data*) data, light* lights, size_t light_count, WAYLIB_OPTIONAL(frame_time) frame_time) {
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
        emscripten_sleep(1);
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
bool format_is_srgb(wgpu::TextureFormat format) {
	switch(format) {
	case WGPUTextureFormat_RGBA8UnormSrgb:
	case WGPUTextureFormat_BGRA8UnormSrgb:
	case WGPUTextureFormat_BC1RGBAUnormSrgb:
	case WGPUTextureFormat_BC2RGBAUnormSrgb:
	case WGPUTextureFormat_BC3RGBAUnormSrgb:
	case WGPUTextureFormat_ETC2RGB8UnormSrgb:
	case WGPUTextureFormat_ETC2RGB8A1UnormSrgb:
	case WGPUTextureFormat_ETC2RGBA8UnormSrgb:
	case WGPUTextureFormat_ASTC4x4UnormSrgb:
	case WGPUTextureFormat_ASTC5x4UnormSrgb:
	case WGPUTextureFormat_ASTC5x5UnormSrgb:
	case WGPUTextureFormat_ASTC6x5UnormSrgb:
	case WGPUTextureFormat_ASTC6x6UnormSrgb:
	case WGPUTextureFormat_ASTC8x5UnormSrgb:
	case WGPUTextureFormat_ASTC8x6UnormSrgb:
	case WGPUTextureFormat_ASTC8x8UnormSrgb:
	case WGPUTextureFormat_ASTC10x5UnormSrgb:
	case WGPUTextureFormat_ASTC10x6UnormSrgb:
	case WGPUTextureFormat_ASTC10x8UnormSrgb:
	case WGPUTextureFormat_ASTC10x10UnormSrgb:
	case WGPUTextureFormat_ASTC12x10UnormSrgb:
	case WGPUTextureFormat_ASTC12x12UnormSrgb:
		return true;
	default: return false;
	}
}

wgpu::TextureFormat surface_preferred_format(waylib_state state) {
	wgpu::SurfaceCapabilities capabilities;
	state.surface.getCapabilities(state.adapter, &capabilities); // TODO: Always returns error?

	// Assumes the formats are in order of preference... finds the first SRGB format
	for(size_t i = 0; i < capabilities.formatCount; ++i)
		if(format_is_srgb(capabilities.formats[i]))
			return capabilities.formats[i];

	set_error_message_raw("Failed to find SRGB surface format... falling back to linear.");
	return capabilities.formats[0];
}

waylib_state create_default_state_from_instance(WGPUInstance instance, WGPUSurface surface /*= nullptr*/, bool prefer_low_power /*= false*/) WAYLIB_TRY {
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

waylib_state create_default_state(WGPUSurface surface /*= nullptr*/, bool prefer_low_power /*= false*/) WAYLIB_TRY {
	return create_default_state_from_instance(wgpuCreateInstance({}), surface, prefer_low_power);
} WAYLIB_CATCH({})

void release_waylib_state(waylib_state state, bool release_adapter /*= true*/, bool release_instance /*= true*/) WAYLIB_TRY {
	state.device.getQueue().release();
	state.surface.release();
	state.device.release();
	if(release_adapter) state.adapter.release();
	if(release_instance) state.instance.release();
} WAYLIB_CATCH()

// Returns true if the request present_mode is supported... returns false if we fall back fifo (can be confirmed its not just a memory allocation failure by making sure the first word of the error message is "desired")
bool configure_surface(waylib_state state, vec2i size, surface_configuration config /*= {}*/) WAYLIB_TRY {
	// Configure the surface
	wgpu::SurfaceConfiguration descriptor = {};

	wgpu::SurfaceCapabilities capabilities;
	state.surface.getCapabilities(state.adapter, &capabilities); // TODO: Always returns error?

	// Pick the prefered surface format with fallback
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
	descriptor.format = surface_preferred_format(state);
	descriptor.viewFormatCount = 1;
	descriptor.viewFormats = &descriptor.format;
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

shader_preprocessor* shader_preprocessor_clone(shader_preprocessor* source) WAYLIB_TRY {
	return new shader_preprocessor(*source);
} WAYLIB_CATCH(nullptr)

void release_shader_preprocessor(shader_preprocessor* processor) WAYLIB_TRY {
	delete processor;
} WAYLIB_CATCH()

shader_preprocessor* preprocessor_initialize_virtual_filesystem(shader_preprocessor* processor, waylib_state state, preprocess_shader_config config /*= {}*/) WAYLIB_TRY {
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
	preprocess_shader_from_memory_and_cache(processor,
#include "shaders/waylib/pbr_data.wgsl"
		, "waylib/pbr_data", config);
	preprocess_shader_from_memory_and_cache(processor,
#include "shaders/waylib/pbr.wgsl"
		, "waylib/pbr", config);
	preprocess_shader_from_memory_and_cache(processor,
#include "shaders/waylib/geometry_shader.wgsl"
		, "waylib/geometry_shader", config);
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

shader_preprocessor* preprocessor_initialize_platform_defines(shader_preprocessor* processor, waylib_state state) WAYLIB_TRY {
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

WAYLIB_OPTIONAL(shader) create_shader(waylib_state state, const char* wgsl_source_code, create_shader_configuration config /*= {}*/) WAYLIB_TRY {
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

void release_shader(shader& shader) {
	shader.module.release();
	// TODO: How do we handle entry points?
}
void release_shader(shader* shader) { release_shader(*shader); }

WAYLIB_OPTIONAL(geometry_transformation_shader) create_geometry_transformation_shader(waylib_state state, const char* wgsl_source_code, bool per_vertex_processing /*= false*/, create_shader_configuration config /*= {}*/) WAYLIB_TRY {
	geometry_transformation_shader geometry_transformer {
		.computer = {
			.buffer_count = 7,
			.buffers = new gpu_buffer[7],
			.texture_count = WAYLIB_TEXTURE_SLOT_COUNT,
			.textures = new texture[WAYLIB_TEXTURE_SLOT_COUNT],
			.heap_allocated = true
		},
		.per_vertex_processing = per_vertex_processing,
	};
	auto gp = config.preprocessor ? shader_preprocessor_clone(config.preprocessor) : preprocessor_initialize_virtual_filesystem(create_shader_preprocessor(), state);
	if(!gp) return {};
	defer {release_shader_preprocessor(gp);};
	if(per_vertex_processing) preprocessor_add_define(gp, "WAYLIB_GEOMETRY_SHADER_PER_VERTEX_PROCESSING", "1");
	config.preprocessor = gp;

	if(!config.compute_entry_point) {
		set_error_message_raw("NOTE: Overridding compute entry point with default.");
		config.compute_entry_point = "waylib_default_geometry_shader";
	}

	auto shader = create_shader(state, wgsl_source_code, config);
	if(!shader.has_value) return {};
	geometry_transformer.computer.shader = shader.value;
	computer_upload(state, geometry_transformer.computer);

	return geometry_transformer;
} WAYLIB_CATCH({})

void release_geometry_transformation_shader(geometry_transformation_shader& shader) {
	release_computer(shader.computer);
}
void release_geometry_transformation_shader(geometry_transformation_shader* shader) {
	release_geometry_transformation_shader(*shader);
}

namespace detail{
	std::pair<wgpu::RenderPipelineDescriptor, WAYLIB_OPTIONAL(wgpu::FragmentState)> shader_configure_render_pipeline_descriptor(waylib_state state, shader& shader, wgpu::RenderPipelineDescriptor pipelineDesc = {}) {
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
			.format = wgpu::VertexFormat::Float32x3,
			.offset = 0,
			.shaderLocation = 4,
		};
		static WGPUVertexBufferLayout tangentLayout = {
			.arrayStride = sizeof(vec3f),
			.stepMode = wgpu::VertexStepMode::Vertex,
			.attributeCount = 1,
			.attributes = &tangentAttribute,
		};

		static WGPUVertexAttribute bitangentAttribute = {
			.format = wgpu::VertexFormat::Float32x3,
			.offset = 0,
			.shaderLocation = 5,
		};
		static WGPUVertexBufferLayout bitangentLayout = {
			.arrayStride = sizeof(vec3f),
			.stepMode = wgpu::VertexStepMode::Vertex,
			.attributeCount = 1,
			.attributes = &bitangentAttribute,
		};

		static WGPUVertexAttribute texcoord2Attribute = {
			.format = wgpu::VertexFormat::Float32x2,
			.offset = 0,
			.shaderLocation = 6,
		};
		static WGPUVertexBufferLayout texcoord2Layout = {
			.arrayStride = sizeof(vec2f),
			.stepMode = wgpu::VertexStepMode::Vertex,
			.attributeCount = 1,
			.attributes = &texcoord2Attribute,
		};
		static std::array<WGPUVertexBufferLayout, 7> bufferLayouts = {positionLayout, texcoordLayout, normalLayout, colorLayout, tangentLayout, bitangentLayout, texcoord2Layout};

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

std::array<WGPUBindGroupEntry, 18> enumerate_texture_bindings(frame_state& frame, std::span<WAYLIB_NULLABLE(texture*), WAYLIB_TEXTURE_SLOT_COUNT> textures) {
	static const texture& default_texture = *throw_if_null(get_default_texture(frame.state));
	static const texture& default_cube_texture = *throw_if_null(get_default_cube_texture(frame.state));
	static std::array<WGPUBindGroupEntry, 18> bindings = [] {
		std::array<WGPUBindGroupEntry, 18> bindings = {WGPUBindGroupEntry{
			.binding = 5,
			.textureView = default_texture.view,
		}, WGPUBindGroupEntry{
			.binding = 6,
			.sampler = default_texture.sampler,
		}};
		for(size_t i = 2; i < bindings.size(); i += 2) {
			bindings[i + 0] = bindings[0];
			bindings[i + 0].binding = i + 5;
			bindings[i + 1] = bindings[1];
			bindings[i + 1].binding = i + 6;
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

WGPUBindGroupEntry default_material_data_binding_function(frame_state* frame, material* mat) {
	return WGPUBindGroupEntry{
		.binding = 1,
		.buffer = get_zero_buffer(frame->state, WAYLIB_MATERIAL_DATA_SIZE),
		.size = WAYLIB_MATERIAL_DATA_SIZE
	};
}

void setup_default_bindings(frame_state& frame) {
	static std::array<WGPUBindGroupEntry, 23> bindings = [&frame]{
		size_t size = create_pipeline_globals(frame.state).min_buffer_size;
		std::array<WGPUBindGroupEntry, 23> bindings{WGPUBindGroupEntry{
			.binding = 0,
			.buffer = get_zero_buffer(frame.state, size),
			.offset = 0,
			.size = size,
		}, WGPUBindGroupEntry{
			.binding = 1,
			.buffer = get_zero_buffer(frame.state, size),
			.offset = 0,
			.size = size,
		}, WGPUBindGroupEntry{
			.binding = 2,
			.buffer = get_zero_buffer(frame.state, size),
			.offset = 0,
			.size = size,
		}, WGPUBindGroupEntry{
			.binding = 3,
			.buffer = get_zero_buffer(frame.state, size),
			.offset = 0,
			.size = size,
		}, WGPUBindGroupEntry{
			.binding = 4,
			.buffer = get_zero_buffer(frame.state, size),
			.offset = 0,
			.size = size,
		}};
		auto null = std::array<WAYLIB_NULLABLE(texture*), WAYLIB_TEXTURE_SLOT_COUNT>{}; null.fill(nullptr);
		std::array<WGPUBindGroupEntry, 18> textureBindings = enumerate_texture_bindings(frame, null);
		std::move(textureBindings.begin(), textureBindings.end(), bindings.begin() + 5);
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

WAYLIB_OPTIONAL(frame_state) begin_drawing_render_texture(waylib_state state, WGPUTextureView render_texture, vec2i render_texture_dimensions, WAYLIB_OPTIONAL(color) clear_color /*= {}*/) WAYLIB_TRY {
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
	static frame_finalizers finalizers;

	// Create a command encoder for the draw call
	wgpu::CommandEncoderDescriptor encoderDesc = {};
	encoderDesc.label = "Waylib Render Command Encoder";
	wgpu::CommandEncoder render_encoder = state.device.createCommandEncoder(encoderDesc);
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
	wgpu::RenderPassEncoder renderPass = render_encoder.beginRenderPass(renderPassDesc);

	frame_state out = {state, render_texture, depthTexture, depthTextureView, render_encoder, encoder, renderPass, &finalizers};
	setup_default_bindings(out);
	return out;
} WAYLIB_CATCH({})

WAYLIB_OPTIONAL(frame_state) begin_drawing(waylib_state state, WAYLIB_OPTIONAL(color) clear_color /*= {}*/) WAYLIB_TRY {
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

static WAYLIB_OPTIONAL(std::future<void>) submit_future = {};
void end_drawing(frame_state& frame) WAYLIB_TRY {
	frame.render_pass.end();
	frame.render_pass.release();

	// Finally encode and submit the render pass
	wgpu::CommandBufferDescriptor cmdBufferDescriptor = {};
	cmdBufferDescriptor.label = "Waylib Frame Command Buffer";
	wgpu::CommandBuffer render_commands = frame.render_encoder.finish(cmdBufferDescriptor);
	wgpu::CommandBuffer commands = frame.encoder.finish(cmdBufferDescriptor);
	frame.render_encoder.release(); frame.encoder.release();

	// std::cout << "Submitting command..." << std::endl;
	submit_future = thread_pool_enqueue([commands, render_commands, &frame]{
		frame.state.device.getQueue().submit(std::vector<WGPUCommandBuffer>{commands, render_commands});
		((wgpu::CommandBuffer*)&commands)->release();
		((wgpu::CommandBuffer*)&render_commands)->release();
	});
} WAYLIB_CATCH()
void end_drawing(frame_state* frame) { end_drawing(*frame); }

void present_frame(frame_state& frame) WAYLIB_TRY {
	if(submit_future.has_value) submit_future.value.wait();
	submit_future = {};

	auto finalizers = std::move(*frame.finalizers);
	for(auto& finalizer: finalizers)
		finalizer();

#ifndef __EMSCRIPTEN__
	frame.state.surface.present();
#endif

	// Process callbacks
	process_wgpu_events(frame.state.device);
} WAYLIB_CATCH()
void present_frame(frame_state* frame) { present_frame(*frame); }

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

void begin_camera_mode3D(frame_state& frame, camera3D& camera, vec2i window_dimensions, std::span<light> lights /*={}*/, WAYLIB_OPTIONAL(frame_time) frame_time /*={}*/) WAYLIB_TRY {
	camera_upload_data data {
		.is_3D = true,
		.settings3D = camera,
		.window_dimensions = window_dimensions,
	};
	std::tie(data.get_view_matrix(), data.get_projection_matrix()) = camera3D_get_matrix_impl(camera, window_dimensions);
	upload_utility_data(frame, data, lights, frame_time);
} WAYLIB_CATCH()
void begin_camera_mode3D(frame_state* frame, camera3D* camera, vec2i window_dimensions, light* lights /*= nullptr*/, size_t light_count /*=0*/, WAYLIB_OPTIONAL(frame_time) frame_time /*={}*/) {
	begin_camera_mode3D(*frame, *camera, window_dimensions, {lights, light_count}, frame_time);
}

void begin_camera_mode2D(frame_state& frame, camera2D& camera, vec2i window_dimensions, std::span<light> lights /*={}*/, WAYLIB_OPTIONAL(frame_time) frame_time /*={}*/) WAYLIB_TRY {
	camera_upload_data data {
		.is_3D = true,
		.settings2D = camera,
		.window_dimensions = window_dimensions,
	};
	std::tie(data.get_view_matrix(), data.get_projection_matrix()) = camera2D_get_matrix_impl(camera, window_dimensions);
	upload_utility_data(frame, data, lights, frame_time);
} WAYLIB_CATCH()
void begin_camera_mode2D(frame_state* frame, camera2D* camera, vec2i window_dimensions, light* lights /*= nullptr*/, size_t light_count /*=0*/, WAYLIB_OPTIONAL(frame_time) frame_time /*={}*/) {
	begin_camera_mode2D(*frame, *camera, window_dimensions, {lights, light_count}, frame_time);
}

void begin_camera_mode_identity(frame_state& frame, vec2i window_dimensions, std::span<light> lights /*={}*/, WAYLIB_OPTIONAL(frame_time) frame_time /*={}*/) WAYLIB_TRY {
	camera_upload_data data {
		.is_3D = false,
		.settings3D = {},
		.settings2D = {},
		.window_dimensions = window_dimensions,
	};
	data.get_projection_matrix() = data.get_view_matrix() = glm::identity<glm::mat4x4>();
	upload_utility_data(frame, data, lights, frame_time);
} WAYLIB_CATCH()
void begin_camera_mode_identity(frame_state* frame, vec2i window_dimensions, light* lights /*= nullptr*/, size_t light_count /*=0*/, WAYLIB_OPTIONAL(frame_time) frame_time /*={}*/) {
	begin_camera_mode_identity(*frame, window_dimensions);
}

void reset_camera_mode(frame_state& frame) {
	begin_camera_mode_identity(frame, vec2i(0), {}, {});
}
void reset_camera_mode(frame_state* frame) {
	reset_camera_mode(*frame);
}
#endif // WAYLIB_NO_CAMERAS

//////////////////////////////////////////////////////////////////////
// #Mesh
//////////////////////////////////////////////////////////////////////

constexpr static auto getIndex = [](const struct mesh& mesh, const int triangleID, const int vertID) -> index_t {
	if(mesh.indices)
		return mesh.indices[triangleID * 3 + vertID];
	else return triangleID * 3 + vertID;
};

void mesh_generate_normals(mesh& mesh, bool weighted_normals /*= false*/) WAYLIB_TRY {
	if(mesh.normals && mesh.heap_allocated) delete mesh.normals;
	mesh.normals = new vec3f[mesh.vertexCount];
	std::fill(mesh.normals, mesh.normals + mesh.vertexCount, vec3f(0));

	for(size_t i = 0; i < mesh.triangleCount; ++i) {
		vec3u verts(getIndex(mesh, i, 0), getIndex(mesh, i, 1), getIndex(mesh, i, 2));
		vec3f x = mesh.positions[verts.x];
		vec3f y = mesh.positions[verts.y];
		vec3f z = mesh.positions[verts.z];

		vec3f xy = y - x;
		vec3f xz = z - x;
		vec3f n = cross(xy, xz);
		if(!weighted_normals) n = normalize(n); // Weighted normals gives more importance to larger triangles

		mesh.normals[verts.x] += n;
		mesh.normals[verts.y] += n;
		mesh.normals[verts.z] += n;
	}

	for(size_t i = 0; i < mesh.vertexCount; ++i)
		mesh.normals[i] = normalize(mesh.normals[i]);
} WAYLIB_CATCH()
void mesh_generate_normals(mesh* mesh, bool weighted_normals /*= false*/) { mesh_generate_normals(*mesh, weighted_normals); }

void mesh_generate_tangents(mesh& mesh) WAYLIB_TRY {
	constexpr static auto getNumFaces = +[](const SMikkTSpaceContext* pContext) -> int {
		const struct mesh& mesh = *(struct mesh*)pContext->m_pUserData;
		return mesh.triangleCount;
	};
	constexpr static auto getNumVerticesOfFace = +[](const SMikkTSpaceContext * pContext, const int iFace) -> int {
		return 3;
	};
	constexpr static auto getPosition = +[](const SMikkTSpaceContext* pContext, float fvPosOut[], const int iFace, const int iVert) {
		const struct mesh& mesh = *(struct mesh*)pContext->m_pUserData;
		auto& pos = mesh.positions[getIndex(mesh, iFace, iVert)];
		fvPosOut[0] = pos.x;
		fvPosOut[1] = pos.y;
		fvPosOut[2] = pos.z;
	};
	constexpr static auto getNormal = +[](const SMikkTSpaceContext* pContext, float fvPosOut[], const int iFace, const int iVert) {
		const struct mesh& mesh = *(struct mesh*)pContext->m_pUserData;
		auto& norm = mesh.normals[getIndex(mesh, iFace, iVert)];
		fvPosOut[0] = norm.x;
		fvPosOut[1] = norm.y;
		fvPosOut[2] = norm.z;
	};
	constexpr static auto getTexCoord = +[](const SMikkTSpaceContext* pContext, float fvPosOut[], const int iFace, const int iVert) {
		const struct mesh& mesh = *(struct mesh*)pContext->m_pUserData;
		auto& uvs = mesh.uvs[getIndex(mesh, iFace, iVert)];
		fvPosOut[0] = uvs.x;
		fvPosOut[1] = uvs.y;
	};
	constexpr static auto setTangent = +[](const SMikkTSpaceContext* pContext, const float fvTangent[], const float fSign, const int iFace, const int iVert) {
		struct mesh& mesh = *(struct mesh*)pContext->m_pUserData;
		size_t i = getIndex(mesh, iFace, iVert);
		mesh.bitangents[i] = fSign * cross(mesh.normals[i], mesh.tangents[i] = vec3f(fvTangent[0], fvTangent[1], fvTangent[2]));
	};

	if(!mesh.normals) mesh_generate_normals(mesh);

	if(mesh.tangents && mesh.heap_allocated) delete mesh.tangents;
	if(mesh.bitangents && mesh.heap_allocated) delete mesh.bitangents;
	mesh.tangents = new vec3f[mesh.vertexCount];
	mesh.bitangents = new vec3f[mesh.vertexCount];

	SMikkTSpaceInterface i{
		.m_getNumFaces = getNumFaces,
		.m_getNumVerticesOfFace = getNumVerticesOfFace,
		.m_getPosition = getPosition,
		.m_getNormal = getNormal,
		.m_getTexCoord = getTexCoord,
		.m_setTSpaceBasic = setTangent,
		.m_setTSpace = nullptr
	};
	SMikkTSpaceContext c {
		.m_pInterface = &i,
		.m_pUserData = &mesh
	};
	genTangSpaceDefault(&c);
} WAYLIB_CATCH()
void mesh_generate_tangents(mesh *mesh) { mesh_generate_tangents(*mesh); }


void mesh_upload(waylib_state state, mesh& mesh) WAYLIB_TRY {
	static WGPUBufferDescriptor bufferDesc = {
		.label = "Waylib Vertex Buffer",
		.usage = wgpu::BufferUsage::CopyDst | wgpu::BufferUsage::CopySrc | wgpu::BufferUsage::Vertex | wgpu::BufferUsage::Storage,
		// .size = mesh.vertexCount * WAYLIB_MESH_VERTEX_SIZE,
		.mappedAtCreation = true
	};

	if(mesh.uvs && !mesh.tangents) mesh_generate_tangents(mesh);

	size_t biggest = std::max(mesh.vertexCount * sizeof(vec4f), mesh.triangleCount * sizeof(index_t) * 3);
	std::vector<std::byte> zeroBuffer(biggest, std::byte{});

	if(mesh.buffer) mesh.buffer.release();
	bufferDesc.size = mesh.vertexCount * WAYLIB_MESH_VERTEX_SIZE;
	mesh.buffer = state.device.createBuffer(bufferDesc);

	// Upload geometry data to the buffer
	size_t currentOffset = 0;
	wgpu::Queue queue = state.device.getQueue();
	{ // Position
		void* data = mesh.positions ? (void*)mesh.positions : (void*)zeroBuffer.data();
		memcpy(mesh.buffer.getMappedRange(currentOffset, mesh.vertexCount * sizeof(vec3f)), data, mesh.vertexCount * sizeof(vec3f));
		// queue.writeBuffer(mesh.buffer, currentOffset, data, mesh.vertexCount * sizeof(vec3f));
		currentOffset += mesh.vertexCount * sizeof(vec3f);
	}
	{ // uvs
		void* data = mesh.uvs ? (void*)mesh.uvs : (void*)zeroBuffer.data();
		memcpy(mesh.buffer.getMappedRange(currentOffset, mesh.vertexCount * sizeof(vec2f)), data, mesh.vertexCount * sizeof(vec2f));
		// queue.writeBuffer(mesh.buffer, currentOffset, data, mesh.vertexCount * sizeof(vec2f));
		currentOffset += mesh.vertexCount * sizeof(vec2f);
	}
	{ // uvs2
		void* data = mesh.uvs2 ? (void*)mesh.uvs2 : (void*)zeroBuffer.data();
		memcpy(mesh.buffer.getMappedRange(currentOffset, mesh.vertexCount * sizeof(vec2f)), data, mesh.vertexCount * sizeof(vec2f));
		// queue.writeBuffer(mesh.buffer, currentOffset, data, mesh.vertexCount * sizeof(vec2f));
		currentOffset += mesh.vertexCount * sizeof(vec2f);
	}
	{ // Normals
		void* data = mesh.normals ? (void*)mesh.normals : (void*)zeroBuffer.data();
		memcpy(mesh.buffer.getMappedRange(currentOffset, mesh.vertexCount * sizeof(vec3f)), data, mesh.vertexCount * sizeof(vec3f));
		// queue.writeBuffer(mesh.buffer, currentOffset, data, mesh.vertexCount * sizeof(vec3f));
		currentOffset += mesh.vertexCount * sizeof(vec3f);
	}
	{ // Tangents
		void* data = mesh.tangents ? (void*)mesh.tangents : (void*)zeroBuffer.data();
		memcpy(mesh.buffer.getMappedRange(currentOffset, mesh.vertexCount * sizeof(vec3f)), data, mesh.vertexCount * sizeof(vec3f));
		// queue.writeBuffer(mesh.buffer, currentOffset, data, mesh.vertexCount * sizeof(vec3f));
		currentOffset += mesh.vertexCount * sizeof(vec3f);
	}
	{ // BiTangents
		void* data = mesh.bitangents ? (void*)mesh.bitangents : (void*)zeroBuffer.data();
		memcpy(mesh.buffer.getMappedRange(currentOffset, mesh.vertexCount * sizeof(vec3f)), data, mesh.vertexCount * sizeof(vec3f));
		// queue.writeBuffer(mesh.buffer, currentOffset, data, mesh.vertexCount * sizeof(vec3f));
		currentOffset += mesh.vertexCount * sizeof(vec3f);
	}
	{ // Colors
		void* data = mesh.colors ? (void*)mesh.colors : (void*)zeroBuffer.data();
		memcpy(mesh.buffer.getMappedRange(currentOffset, mesh.vertexCount * sizeof(color)), data, mesh.vertexCount * sizeof(color));
		// queue.writeBuffer(mesh.buffer, currentOffset, data, mesh.vertexCount * sizeof(color));
		currentOffset += mesh.vertexCount * sizeof(color);
	}
	mesh.buffer.unmap(); // Finished copying data
	if(mesh.indices) {
		static WGPUBufferDescriptor bufferDesc {
			.label = "Vertex Position Buffer",
			.usage = wgpu::BufferUsage::CopyDst | wgpu::BufferUsage::Index | wgpu::BufferUsage::Storage,
			// .size = mesh.triangleCount * sizeof(index_t) * 3,
			.mappedAtCreation = false
		};

		if(mesh.indexBuffer) mesh.indexBuffer.release();
		bufferDesc.size = mesh.triangleCount * sizeof(index_t) * 3;
		mesh.indexBuffer = state.device.createBuffer(bufferDesc);

		queue.writeBuffer(mesh.indexBuffer, 0, mesh.indices, mesh.triangleCount * sizeof(index_t) * 3);
		// memcpy(mesh.indexBuffer.getMappedRange(0, bufferDesc.size), mesh.indices, mesh.triangleCount * sizeof(index_t) * 3);
		// mesh.indexBuffer.unmap(); // Finished copying data
	} else if(mesh.indexBuffer) mesh.indexBuffer.release();
} WAYLIB_CATCH()
void mesh_upload(waylib_state state, mesh* mesh) {
	mesh_upload(state, *mesh);
}

void release_mesh(mesh& mesh) {
	if(mesh.heap_allocated) {
		if(mesh.positions) delete mesh.positions;
		if(mesh.uvs) delete mesh.uvs;
		if(mesh.uvs2) delete mesh.uvs2;
		if(mesh.normals) delete mesh.normals;
		if(mesh.tangents) delete mesh.tangents;
		if(mesh.bitangents) delete mesh.bitangents;
		if(mesh.colors) delete mesh.colors;
		if(mesh.indices) delete mesh.indices;
	}
	mesh.buffer.release();
	if(mesh.indexBuffer) mesh.indexBuffer.release();
	// mesh.instanceBuffer.release();
}
void release_mesh(mesh *mesh) { release_mesh(*mesh); }

//////////////////////////////////////////////////////////////////////
// #Material
//////////////////////////////////////////////////////////////////////

pipeline_globals& create_pipeline_globals(waylib_state state); // Declared in waylib.cpp

void material_upload(waylib_state state, material& material, material_configuration config /*= {}*/) WAYLIB_TRY {
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
	pipelineDesc.primitive.cullMode = config.double_sided ? wgpu::CullMode::None : wgpu::CullMode::Back;

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
void material_upload(waylib_state state, material* material, material_configuration config /*= {}*/) {
	material_upload(state, *material, config);
}

void release_material(material& material, bool release_textures /*= true*/, bool release_shaders /*= true*/, bool release_transformer /*= true*/) {
	if(release_shaders) for(auto& shader: material.get_shaders())
		release_shader(shader);
	if(release_textures) for(auto& texture: material.get_textures())
		if(texture) release_texture(texture);
	if(material.geometry_transformer) release_computer(material.geometry_transformer->computer);

	material.pipeline.release(); // TODO: Fails?
}
void release_material(material *material, bool release_textures /*= true*/, bool release_shaders /*= true*/, bool release_transformer /*= true*/) { release_material(*material); }

material create_material(waylib_state state, shader* shaders, size_t shader_count, material_configuration config /*= {}*/) {
	material out {.shaderCount = (index_t)shader_count, .shaders = shaders};
	material_upload(state, out, config);
	out.material_data_binding_function = default_material_data_binding_function;
	return out;
}
material create_material(waylib_state state, std::span<shader> shaders, material_configuration config /*= {}*/) {
	return create_material(state, shaders.data(), shaders.size(), config);
}
material create_material(waylib_state state, shader& shader, material_configuration config /*= {}*/) {
	return create_material(state, &shader, 1, config);
}

wgpu::BindGroupEntry pbr_material_default_data_binding_function(frame_state& frame, material& _mat) {
	static WGPUBufferDescriptor bufferDesc = {
		.label = "Waylib PBR Buffer",
		.usage = wgpu::BufferUsage::CopyDst | wgpu::BufferUsage::Uniform,
		.size = WAYLIB_MATERIAL_DATA_SIZE,
		.mappedAtCreation = false,
	};
	pbr_material& mat = *(pbr_material*)&_mat;

	wgpu::Buffer pbrBuffer = frame.state.device.createBuffer(bufferDesc); frame_defer(frame) { pbrBuffer.destroy(); pbrBuffer.release(); };
	frame.state.device.getQueue().writeBuffer(pbrBuffer, 0, &mat.color, WAYLIB_MATERIAL_DATA_SIZE);
	// std::memcpy(pbrBuffer.getMappedRange(0, WAYLIB_MATERIAL_DATA_SIZE), &mat.color, WAYLIB_MATERIAL_DATA_SIZE);
	// pbrBuffer.unmap();

	return WGPUBindGroupEntry{
		.binding = 1,
		.buffer = pbrBuffer,
		.size = WAYLIB_MATERIAL_DATA_SIZE
	};
}

pbr_material create_pbr_material(waylib_state state, std::span<shader> shaders, std::span<WAYLIB_NULLABLE(texture*)> textures /*= {}*/, material_configuration config /*= {}*/) {
	assert(textures.size() <= WAYLIB_TEXTURE_SLOT_COUNT);
	pbr_material mat = (pbr_material)create_material(state, shaders, config);
	material_set_data_binding_function(mat, pbr_material_default_data_binding_function);
	for(size_t i = 0; i < textures.size(); i++) {
		mat.textures[i] = textures[i];
		if(textures[i]) switch((texture_slot)i) {
			break; case texture_slot::Color: mat.use_color_map = true;
			break; case texture_slot::Cubemap: mat.use_environment_map = true;
			break; case texture_slot::Height: mat.use_height_map = true;
			break; case texture_slot::Normal: mat.use_normal_map = true;
			break; case texture_slot::PackedMap: mat.use_packed_map = true;
			break; case texture_slot::Roughness: if(!mat.use_packed_map) mat.use_roughness_map = true;
			break; case texture_slot::Metalness: if(!mat.use_packed_map) mat.use_metalness_map = true;
			break; case texture_slot::AmbientOcclusion: if(!mat.use_packed_map) mat.use_ambient_occlusion_map = true;
			break; case texture_slot::Emission: mat.use_emission_map = true;
			break; default: {}
		}
	}
	return mat;
}
pbr_material create_pbr_material(waylib_state state, shader* shaders, size_t shader_count, WAYLIB_NULLABLE(texture*)* textures /*= nullptr*/, size_t texture_count /*= 0*/, material_configuration config /*= {}*/) {
	return create_pbr_material(state, {shaders, shader_count}, {textures, texture_count}, config);
}
pbr_material create_pbr_material(waylib_state state, shader& shader, std::span<WAYLIB_NULLABLE(texture*)> textures /*= {}*/, material_configuration config /*= {}*/) {
	return create_pbr_material(state, {&shader, 1}, textures, config);
}

WAYLIB_OPTIONAL(pbr_material) create_default_pbr_material(waylib_state state, std::span<WAYLIB_NULLABLE(texture*)> textures /*= {}*/, material_configuration config /*= {}*/) WAYLIB_TRY {
	constexpr static const char* shaderSource =
#include "shaders/defaults/pbr.wgsl"
	;
	constexpr static const char* geometryShaderSource =
#include "shaders/defaults/pbr.displacement.wgsl"
	;

	bool free_preprocessor = false;
	if(!config.preprocessor) {
		config.preprocessor = preprocessor_initialize_virtual_filesystem(create_shader_preprocessor(), state);
		free_preprocessor = true;
	}

	auto shader = create_shader(state, shaderSource,
		{.vertex_entry_point = "waylib_default_vertex_shader", .fragment_entry_point = "waylib_default_fragment_shader", .name = "Waylib Default PBR Shader", .preprocessor = config.preprocessor}
	);
	if(!shader.has_value) {
		if(free_preprocessor) release_shader_preprocessor(config.preprocessor);
		return {};
	}

	auto mat = create_pbr_material(state, shader.value, textures, config);
	if(textures.size() > (size_t)texture_slot::Height && textures[(size_t)texture_slot::Height]) { // should enable displacement?
		static geometry_transformation_shader transformer = throw_if_null(create_geometry_transformation_shader(state, geometryShaderSource, true, {.name = "Waylib Default PBR Displacemant Shader", .preprocessor = config.preprocessor}));
		mat.geometry_transformer = &transformer;
	}

	if(free_preprocessor) release_shader_preprocessor(config.preprocessor);
	return mat;
} WAYLIB_CATCH({})
WAYLIB_OPTIONAL(pbr_material) create_default_pbr_material(waylib_state state, WAYLIB_NULLABLE(texture*)* textures, size_t texture_count, material_configuration config /*= {}*/) {
	return create_default_pbr_material(state, {textures, texture_count}, config);
}

//////////////////////////////////////////////////////////////////////
// #Model
//////////////////////////////////////////////////////////////////////

void model_upload(waylib_state state, model& model) {
	for(size_t i = 0; i < model.mesh_count; ++i)
		mesh_upload(state, model.meshes[i]);
	// for(size_t i = 0; i < model.material_count; ++i) // TODO: How do we handle material configurations?
	// 	material_upload(state, model.materials[i]);
}
void model_upload(waylib_state state, model* model) {
	model_upload(state, *model);
}

void release_model(model& model, bool release_meshes /*= true*/, bool release_materials /*= true*/, bool release_textures /*= true*/, bool release_shaders /*= true*/) {
	if(release_meshes) for(auto& mesh: model.get_meshes())
		release_mesh(mesh);
	if(release_materials) for(auto& material: model.get_materials())
		release_material(material, release_textures, release_shaders);

	if(model.heap_allocated) delete model.bones;
	// TODO: Other things need deleting?
}
void release_model(model* model) { release_model(*model); }

void model_draw_instanced(frame_state& frame, model& model, std::span<model_instance_data> instances) WAYLIB_TRY {
	static WGPUBufferDescriptor instanceBufferDesc = {
		.label = "Waylib Instance Buffer",
		.usage = wgpu::BufferUsage::CopyDst | wgpu::BufferUsage::Storage,
		.size = sizeof(model_instance_data),
		.mappedAtCreation = false,
	};
	static WGPUBufferDescriptor meshDataBufferDesc = {
		.label = "Waylib Mesh Data Buffer",
		.usage = wgpu::BufferUsage::CopyDst | wgpu::BufferUsage::Storage,
		.size = sizeof(mesh_metadata),
		.mappedAtCreation = false,
	};
	static std::array<WGPUBindGroupEntry, 23> bindings = {WGPUBindGroupEntry{
		.binding = 0,
		.buffer = get_zero_buffer(frame.state, instanceBufferDesc.size),
		.offset = 0,
		.size = sizeof(model_instance_data) * instances.size(),
	}, WGPUBindGroupEntry{
		.binding = 1,
		.buffer = get_zero_buffer(frame.state, WAYLIB_MATERIAL_DATA_SIZE),
		.offset = 0,
		.size = WAYLIB_MATERIAL_DATA_SIZE
	}, WGPUBindGroupEntry{
		.binding = 2,
		.buffer = get_zero_buffer(frame.state, sizeof(mesh_metadata)),
		.offset = 0,
		.size = sizeof(mesh_metadata),
	}, WGPUBindGroupEntry{
		.binding = 3,
		.buffer = get_zero_buffer(frame.state, WAYLIB_MESH_VERTEX_SIZE),
		.offset = 0,
		.size = WAYLIB_MESH_VERTEX_SIZE,
	}, WGPUBindGroupEntry{
		.binding = 4,
		.buffer = get_zero_buffer(frame.state, sizeof(index_t) * 3),
		.offset = 0,
		.size = sizeof(index_t) * 3,
	}};
	static WGPUBindGroupDescriptor bindGroupDesc {
		.label = "Waylib Per Model Bind Group",
		.layout = create_pipeline_globals(frame.state).bindGroupLayouts[0], // Group 0 is instance/texture data
		.entryCount = bindings.size(),
		.entries = bindings.data(),
	};

	if(instances.size() > 0) {
		instanceBufferDesc.size = sizeof(model_instance_data) * instances.size();
		wgpu::Buffer instanceBuffer = frame.state.device.createBuffer(instanceBufferDesc); frame_defer(frame) { instanceBuffer.destroy(); instanceBuffer.release(); };
		// memcpy(instanceBuffer.getMappedRange(0, instanceBufferDesc.size), instances.data(), instanceBufferDesc.size);
		// instanceBuffer.unmap();
		frame.state.device.getQueue().writeBuffer(instanceBuffer, 0, instances.data(), sizeof(model_instance_data) * instances.size());
		frame_defer(frame) { instanceBuffer.destroy(); instanceBuffer.release(); };

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
		if(mat.material_data_binding_function)
			bindings[1] = mat.material_data_binding_function(&frame, &mat);
		std::move(tex.begin(), tex.end(), bindings.begin() + 5);

		auto& mesh = model.meshes[i];
		wgpu::Buffer vertexBuffer = mesh.buffer, indexBuffer = mesh.indexBuffer;
		mesh_metadata metadata = mesh.get_metadata();
		if(mat.geometry_transformer) {
			constexpr static auto roundUpTo3 = [](size_t v){
				int r = v % 3;
				if (r == 0) return v;
				return v + 3 - r;
			};

			size_t scaled_count = roundUpTo3(std::max(mesh.triangleCount, mesh.vertexCount) * mat.geometry_transformer->vertex_multiplier);
			gpu_buffer vertexOutput {
				.size = scaled_count * WAYLIB_MESH_VERTEX_SIZE,
				.offset = 0,
			};
			gpu_buffer indexOutput {
				.size = (indexBuffer ? roundUpTo3(mesh.triangleCount * mat.geometry_transformer->vertex_multiplier) : 0) * sizeof(index_t),
				.offset = 0
			};
			gpu_buffer_upload(frame.state, vertexOutput, wgpu::BufferUsage::CopyDst | wgpu::BufferUsage::Vertex | wgpu::BufferUsage::Storage);
			gpu_buffer_copy_record_existing(frame.encoder, vertexOutput, gpu_buffer{.size=vertexBuffer.getSize(), .offset=0, .data=vertexBuffer});
			frame_defer(frame) { release_gpu_buffer(vertexOutput); };
			if(indexBuffer) {
				gpu_buffer_upload(frame.state, indexOutput, wgpu::BufferUsage::CopyDst | wgpu::BufferUsage::Index | wgpu::BufferUsage::Storage);
				gpu_buffer_copy_record_existing(frame.encoder, indexOutput, gpu_buffer{.size=indexBuffer.getSize(), .offset=0, .data=indexBuffer});
				frame_defer(frame) { release_gpu_buffer(indexOutput); };
			}

			gpu_buffer gpu_metadata_in = create_gpu_buffer(frame.state, metadata); frame_defer(frame) { release_gpu_buffer(gpu_metadata_in); }; // TODO: Should be a uniform gpu_buffer?
			// TODO: Scale output metadata here?
			gpu_buffer gpu_metadata_out = create_gpu_buffer(frame.state, metadata); frame_defer(frame) { release_gpu_buffer(gpu_metadata_out); };

			auto& computer = mat.geometry_transformer->computer;
			computer.buffers[0] = gpu_metadata_in;
			computer.buffers[1] = {.size = WAYLIB_MATERIAL_DATA_SIZE, .offset = 0, .data = bindings[1].buffer};
			computer.buffers[2] = gpu_metadata_out;
			computer.buffers[3] = {.size = static_cast<size_t>(vertexBuffer.getSize()), .offset = 0, .data = vertexBuffer};
			computer.buffers[4] = vertexOutput;
			if(indexBuffer) {
				computer.buffers[5] = {.size = static_cast<size_t>(indexBuffer.getSize()), .offset = 0, .data = indexBuffer};
				computer.buffers[6] = indexOutput;
			} else {
				// instanceBufferDesc.mappedAtCreation = false;
				instanceBufferDesc.label = "Trash Buffer";
				static wgpu::Buffer trashBufferIn = frame.state.device.createBuffer(instanceBufferDesc);
				static wgpu::Buffer trashBufferOut = frame.state.device.createBuffer(instanceBufferDesc);
				instanceBufferDesc.label = "Waylib Instance Buffer";
				// instanceBufferDesc.mappedAtCreation = true;
				computer.buffers[5] = {.size = static_cast<size_t>(trashBufferIn.getSize()), .offset = 0, .data = trashBufferIn};
				computer.buffers[6] = {.size = static_cast<size_t>(trashBufferOut.getSize()), .offset = 0, .data = trashBufferOut};
			}
			for(size_t i = 0; i < WAYLIB_TEXTURE_SLOT_COUNT; ++i)
				if(mat.textures[i])
					computer.textures[i] = *mat.textures[i];
				else computer.textures[i] = *get_default_texture(frame.state).value;

			vec3u workgroups = mat.geometry_transformer->per_vertex_processing
				? vec3u{roundUpTo3(mesh.vertexCount * mat.geometry_transformer->vertex_multiplier) / 64, 1, 1}
				: vec3u{roundUpTo3(mesh.triangleCount * mat.geometry_transformer->vertex_multiplier * 3) / 64, 1, 1};
			computer_record_existing(frame.state, frame.encoder, computer, workgroups);
			if( !(!mat.geometry_transformer->force_vertex_count_sync && mat.geometry_transformer->vertex_multiplier == 1) ){ // If vertex count likely changed... we need to sync the new count
				gpu_metadata_out.cpu_data = nullptr; // Prepare the buffer for downloading
				gpu_buffer_download(frame.state, gpu_metadata_out);
				metadata = *(mesh_metadata*)gpu_metadata_out.cpu_data;
			}

			vertexBuffer = vertexOutput.data;
			if(indexBuffer) indexBuffer = indexOutput.data;
		}

		// Bind the mesh data buffer
		wgpu::Buffer meshDataBuffer = frame.state.device.createBuffer(meshDataBufferDesc); frame_defer(frame) { meshDataBuffer.destroy(); meshDataBuffer.release(); };
		// memcpy(meshDataBuffer.getMappedRange(0, meshDataBufferDesc.size), &metadata, meshDataBufferDesc.size);
		frame.state.device.getQueue().writeBuffer(meshDataBuffer, 0, &metadata, meshDataBufferDesc.size);
		bindings[2].buffer = meshDataBuffer;
		// meshDataBuffer.unmap();
		// Bind raw access to the vertex and index buffers
		bindings[3].buffer = vertexBuffer;
		bindings[4].buffer = indexBuffer ? indexBuffer : get_zero_buffer(frame.state, sizeof(index_t) * 3);

		// Bind the instance and texture buffers
		wgpu::BindGroup bindGroup = frame.state.device.createBindGroup(bindGroupDesc); //frame_defer(frame) { bindGroup.release(); };
		frame.render_pass.setBindGroup(0, bindGroup, 0, nullptr);

		// Position
		frame.render_pass.setVertexBuffer(0, vertexBuffer, metadata.position_start, metadata.vertex_count * sizeof(vec3f));
		// Texcoord
		frame.render_pass.setVertexBuffer(1, vertexBuffer, metadata.uvs_start, metadata.vertex_count * sizeof(vec2f));
		// Texcoord 2
		frame.render_pass.setVertexBuffer(6, vertexBuffer, metadata.uvs2_start, metadata.vertex_count * sizeof(vec2f));
		// Normals
		frame.render_pass.setVertexBuffer(2, vertexBuffer, metadata.normals_start, metadata.vertex_count * sizeof(vec3f));
		// Tangents
		frame.render_pass.setVertexBuffer(4, vertexBuffer, metadata.tangents_start, metadata.vertex_count * sizeof(vec3f));
		// BiTangents
		frame.render_pass.setVertexBuffer(5, vertexBuffer, metadata.bitangents_start, metadata.vertex_count * sizeof(vec3f));
		// Colors
		frame.render_pass.setVertexBuffer(3, vertexBuffer, metadata.colors_start, metadata.vertex_count * sizeof(color));

		if(mesh.indexBuffer) {
			frame.render_pass.setIndexBuffer(indexBuffer, calculate_index_format<index_t>(), 0, metadata.triangle_count * 3 * sizeof(index_t));
			frame.render_pass.drawIndexed(metadata.triangle_count * 3, std::max<size_t>(instances.size(), 1), 0, 0, 0);
		} else
			frame.render_pass.draw(metadata.vertex_count, std::max<size_t>(instances.size(), 1), 0, 0);
	}
} WAYLIB_CATCH()
void model_draw_instanced(frame_state* frame, model* model, model_instance_data* instances, size_t instance_count) {
	model_draw_instanced(*frame, *model, {instances, instance_count});
}

void model_draw(frame_state& frame, model& model) {
	model_instance_data instance = {model.transform, {}, {1, 1, 1, 1}};
	convert(instance.inverse_transform) = glm::inverse(convert(model.transform));
	model_draw_instanced(frame, model, {&instance, 1});
}
void model_draw(frame_state* frame, model* model) {
	model_draw(*frame, *model);
}

//////////////////////////////////////////////////////////////////////
// #Texture
//////////////////////////////////////////////////////////////////////

WAYLIB_OPTIONAL(texture) create_texture(waylib_state state, vec2i dimensions, texture_config config /*= {}*/) {
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

WAYLIB_OPTIONAL(texture) create_texture_from_image(waylib_state state, image& image, texture_config config /*= {}*/) {
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
WAYLIB_OPTIONAL(texture) create_texture_from_image(waylib_state state, image* image, texture_config config /*= {}*/) {
	return create_texture_from_image(state, *image, config);
}
WAYLIB_OPTIONAL(texture) create_texture_from_image(waylib_state state, WAYLIB_OPTIONAL(image)&& image, texture_config config /*= {}*/) {
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

WAYLIB_OPTIONAL(const texture*) get_default_texture(waylib_state state) {
	static image image = texture_not_found_image(16);
	static WAYLIB_OPTIONAL(texture) texture = create_texture_from_image(state, image, {.color_filter = wgpu::FilterMode::Nearest});

	if(texture.has_value) return &texture.value;
	return {};
}
WAYLIB_OPTIONAL(const texture*) get_default_cube_texture(waylib_state state) {
	static image image = [] {
		std::array<WAYLIB_NAMESPACE_NAME::image, 6> faces; faces.fill(texture_not_found_image(16));
		return merge_images(faces).value;
	}();
	static WAYLIB_OPTIONAL(texture) texture = create_texture_from_image(state, image, {.color_filter = wgpu::FilterMode::Nearest, .cubemap = true});

	if(texture.has_value) return &texture.value;
	return {};
}

void release_image(image& image) {
	if(image.heap_allocated) delete image.data;
}
void release_image(image* image) { release_image(*image); }

void release_texture(texture& texture) {
	if(texture.heap_allocated) release_image(texture.cpu_data);
	texture.view.release();
	texture.sampler.release();
	texture.gpu_data.release();
}
void release_texture(texture* texture) { release_texture(*texture); }

#ifdef WAYLIB_NAMESPACE_NAME
}
#endif