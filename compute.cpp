#include "waylib.hpp"
#include "common.h"

#include <cassert>
#include <cstring>
#include <string_view>
#include <utility>

#ifdef WAYLIB_NAMESPACE_NAME
namespace WAYLIB_NAMESPACE_NAME {
#endif

//////////////////////////////////////////////////////////////////////
// #Buffer
//////////////////////////////////////////////////////////////////////

void gpu_buffer_upload(waylib_state state, gpu_buffer& gpu_buffer, WGPUBufferUsageFlags usage /*= wgpu::BufferUsage::CopyDst | wgpu::BufferUsage::Storage*/, bool free_cpu_data /*= false*/) {
	if(gpu_buffer.data == nullptr) {
		gpu_buffer.data = state.device.createBuffer(WGPUBufferDescriptor {
			.label = gpu_buffer.label ? gpu_buffer.label : "Generic Waylib Buffer",
			.usage = usage,
			.size = gpu_buffer.offset + gpu_buffer.size,
			.mappedAtCreation = false,
		});
		if(gpu_buffer.cpu_data) {
			state.device.getQueue().writeBuffer(gpu_buffer.data, gpu_buffer.offset, gpu_buffer.cpu_data, gpu_buffer.size);
			// memcpy(gpu_buffer.data.getMappedRange(gpu_buffer.offset, gpu_buffer.size), gpu_buffer.cpu_data, gpu_buffer.size);
			// gpu_buffer.data.unmap();
		}
	} else state.device.getQueue().writeBuffer(gpu_buffer.data, gpu_buffer.offset, gpu_buffer.cpu_data, gpu_buffer.size);

	if(free_cpu_data) free(gpu_buffer.cpu_data);
}
void gpu_buffer_upload(waylib_state state, gpu_buffer* gpu_buffer, WGPUBufferUsageFlags usage /*= WGPUBufferUsage_CopyDst | WGPUBufferUsage_Storage*/, bool free_cpu_data /*= false*/){
	gpu_buffer_upload(state, *gpu_buffer, usage, free_cpu_data);
}

gpu_buffer create_gpu_buffer(waylib_state state, std::span<std::byte> data, WGPUBufferUsageFlags usage /*= wgpu::BufferUsage::CopyDst | wgpu::BufferUsage::Storage*/, WAYLIB_OPTIONAL(std::string_view) label /*= {}*/) {
	gpu_buffer out = {
		.size = data.size(),
		.offset = 0,
		.cpu_data = (uint8_t*)data.data(),
		.data = nullptr,
		.label = label.has_value ? cstring_from_view(label.value) : nullptr
	};
	gpu_buffer_upload(state, out, usage, false);
	return out;
}
gpu_buffer create_gpu_buffer(waylib_state state, void* data, size_t size, WGPUBufferUsageFlags usage /*= WGPUBufferUsage_CopyDst | WGPUBufferUsage_Storage*/, const char* label /*= nullptr*/) {
	return create_gpu_buffer(state, {(std::byte*)data, size}, usage, label ? WAYLIB_OPTIONAL(std::string_view){label} : WAYLIB_OPTIONAL(std::string_view){});
}


void* gpu_buffer_map(waylib_state state, gpu_buffer& gpu_buffer, WGPUMapModeFlags mode /*= wgpu::MapMode::None*/) {
	if(mode != wgpu::MapMode::None) {
		bool done = false;
		// TODO: Update API
		auto handle = gpu_buffer.data.mapAsync(mode, gpu_buffer.offset, gpu_buffer.size, [&](wgpu::BufferMapAsyncStatus status) {
			done = true;
		});
		while(!done) state.instance.processEvents();
	}
	return gpu_buffer.data.getMappedRange(gpu_buffer.offset, gpu_buffer.size);
}
void* gpu_buffer_map(waylib_state state, gpu_buffer* gpu_buffer, WGPUMapModeFlags mode /*= WGPUMapMode_None*/) {
	return gpu_buffer_map(state, *gpu_buffer, mode);
}

const void* gpu_buffer_map_const(waylib_state state, gpu_buffer& gpu_buffer, WGPUMapModeFlags mode /*= wgpu::MapMode::None*/) {
	if(mode != wgpu::MapMode::None) {
		bool done = false;
		// TODO: Update API
		auto handle = gpu_buffer.data.mapAsync(mode, gpu_buffer.offset, gpu_buffer.size, [&](wgpu::BufferMapAsyncStatus status) {
			done = true;
		});
		while(!done) state.instance.processEvents();
	}
	return gpu_buffer.data.getConstMappedRange(gpu_buffer.offset, gpu_buffer.size);
}
const void* gpu_buffer_map_const(waylib_state state, gpu_buffer* gpu_buffer, WGPUMapModeFlags mode /*= wgpu::MapMode::None*/) {
	return gpu_buffer_map_const(state, *gpu_buffer, mode);
}

void gpu_buffer_unmap(gpu_buffer& gpu_buffer) {
	gpu_buffer.data.unmap();
}
void gpu_buffer_unmap(gpu_buffer* gpu_buffer) {
	gpu_buffer->data.unmap();
}

void release_gpu_buffer(gpu_buffer& gpu_buffer) {
	if(gpu_buffer.heap_allocated && gpu_buffer.cpu_data) free(gpu_buffer.cpu_data);
	gpu_buffer.data.destroy(); // TODO: Needed?
	gpu_buffer.data.release();
}
void release_gpu_buffer(gpu_buffer* gpu_buffer) {
	release_gpu_buffer(*gpu_buffer);
}

void gpu_buffer_copy_record_existing(WGPUCommandEncoder _encoder, gpu_buffer& dest, const gpu_buffer& source) {
	wgpu::CommandEncoder encoder = _encoder;
	encoder.copyBufferToBuffer(source.data, source.offset, dest.data, dest.offset, dest.size);
}
void gpu_buffer_copy_record_existing(WGPUCommandEncoder encoder, gpu_buffer* dest, const gpu_buffer* source) {
	gpu_buffer_copy_record_existing(encoder, *dest, *source);
}

void gpu_buffer_copy(waylib_state state, gpu_buffer& dest, const gpu_buffer& source) {
	auto encoder = state.device.createCommandEncoder();
	gpu_buffer_copy_record_existing(encoder, dest, source);
	state.device.getQueue().submit(encoder.finish());
	encoder.release();
}
void gpu_buffer_copy(waylib_state state, gpu_buffer* dest, const gpu_buffer* source) {
	gpu_buffer_copy(state, *dest, *source);
}

// You must clear a landing pad for the data (set cpu_data to null) before calling this function
void gpu_buffer_download(waylib_state state, gpu_buffer& gpu_buffer, bool create_intermediate_gpu_buffer /*= true*/) {
	assert(gpu_buffer.cpu_data == nullptr);
	if(create_intermediate_gpu_buffer) {
		struct gpu_buffer out{
			.size = gpu_buffer.size,
			.offset = 0,
			.label = "Intermediate"
		};
		gpu_buffer_upload(state, out, wgpu::BufferUsage::CopyDst | wgpu::BufferUsage::MapRead);
		gpu_buffer_copy(state, out, gpu_buffer);
		gpu_buffer_download(state, out, false);
		gpu_buffer.cpu_data = std::exchange(out.cpu_data, nullptr);
		gpu_buffer.heap_allocated = std::exchange(out.heap_allocated, false);
		release_gpu_buffer(out);
	} else {
		gpu_buffer.cpu_data = new uint8_t[gpu_buffer.size];
		gpu_buffer.heap_allocated = true;
		auto src = gpu_buffer_map_const(state, gpu_buffer, wgpu::MapMode::Read);
		memcpy(gpu_buffer.cpu_data, src, gpu_buffer.size);
		gpu_buffer_unmap(gpu_buffer);
	}
}
void gpu_buffer_download(waylib_state state, gpu_buffer* gpu_buffer, bool create_intermediate_gpu_buffer /*= true*/){
	gpu_buffer_download(state, *gpu_buffer, create_intermediate_gpu_buffer);
}

// TODO: Add gpu_buffer->image and image->gpu_buffer functions

//////////////////////////////////////////////////////////////////////
// #Computer
//////////////////////////////////////////////////////////////////////

void computer_upload(waylib_state state, computer& compute, WAYLIB_OPTIONAL(std::string_view) label /*= {}*/) {
	wgpu::ComputePipelineDescriptor computePipelineDesc = wgpu::Default;
	computePipelineDesc.label = label.has_value ? cstring_from_view(label.value) : "Waylib Compute Pipeline";
	computePipelineDesc.compute.module = compute.shader.module;
	computePipelineDesc.compute.entryPoint = compute.shader.compute_entry_point;
	if(compute.pipeline) compute.pipeline.release();
	compute.pipeline = state.device.createComputePipeline(computePipelineDesc);
}
void computer_upload(waylib_state state, computer* compute, const char* label /*= nullptr*/) {
	computer_upload(state, *compute, label ? WAYLIB_OPTIONAL(std::string_view){label} : WAYLIB_OPTIONAL(std::string_view){});
}

WAYLIB_OPTIONAL(const texture*) get_default_texture(waylib_state state);

computer_recording_state computer_record_existing(waylib_state state, WGPUCommandEncoder _encoder, computer& compute, vec3u workgroups, bool end_pass /*= true*/) {
	const static texture& default_texture = *throw_if_null(get_default_texture(state));
	std::vector<wgpu::BindGroupEntry> bufferEntries(compute.buffer_count, wgpu::Default);
	for(size_t i = 0; i < compute.buffer_count; ++i) {
		bufferEntries[i].binding = i;
		bufferEntries[i].buffer = compute.buffers[i].data;
		bufferEntries[i].offset = compute.buffers[i].offset;
		bufferEntries[i].size = compute.buffers[i].size;
	}
	auto gpu_bufferBindGroup = state.device.createBindGroup(WGPUBindGroupDescriptor{
		.label = "Waylib Compute Buffer Bind Group",
		.layout = compute.pipeline.getBindGroupLayout(0),
		.entryCount = bufferEntries.size(),
		.entries = bufferEntries.data()
	});

	std::vector<wgpu::BindGroupEntry> textureEntries(compute.texture_count /* * 2*/, wgpu::Default);
	// for(size_t i = 0; i < compute.texture_count; ++i) {
	// 	textureEntries[2 * i].binding = 2 * i;
	// 	textureEntries[2 * i].textureView = compute.textures[i].view;
	// 	textureEntries[2 * i + 1].binding = 2 * i + 1;
	// 	textureEntries[2 * i + 1].sampler = compute.textures[i].sampler;
	// }
	for(size_t i = 0; i < compute.texture_count; ++i) {
		textureEntries[i].binding = i;
		textureEntries[i].textureView = i != (size_t)texture_slot::Cubemap ? compute.textures[i].view : default_texture.view;
	}
	auto textureBindGroup = state.device.createBindGroup(WGPUBindGroupDescriptor{
		.label = "Waylib Compute Texture Bind Group",
		.layout = compute.pipeline.getBindGroupLayout(1),
		.entryCount = textureEntries.size(),
		.entries = textureEntries.data()
	});

	wgpu::CommandEncoder encoder = _encoder;
	auto pass = encoder.beginComputePass();
	{
		pass.setPipeline(compute.pipeline);
		pass.setBindGroup(0, gpu_bufferBindGroup, 0, nullptr);
		pass.setBindGroup(1, textureBindGroup, 0, nullptr);
		pass.dispatchWorkgroups(workgroups.x, workgroups.y, workgroups.z);
	}
	if(end_pass) pass.end();
	return {pass, gpu_bufferBindGroup};
}
computer_recording_state computer_record_existing(waylib_state state, WGPUCommandEncoder encoder, computer* compute, vec3u workgroups, bool end_pass /*= true*/) {
	return computer_record_existing(state, encoder, *compute, workgroups, end_pass);
}

void computer_dispatch(waylib_state state, computer& compute, vec3u workgroups) {
	auto encoder = state.device.createCommandEncoder();
	auto record_state = computer_record_existing(state, encoder, compute, workgroups);
	state.device.getQueue().submit(encoder.finish());
	record_state.pass.release();
	encoder.release();
	record_state.bind_group.release();
}
void computer_dispatch(waylib_state state, computer* compute, vec3u workgroups) {
	computer_dispatch(state, *compute, workgroups);
}

void release_computer(computer& compute, bool free_shader /*= true*/, bool free_gpu_buffers /*= false*/, bool free_textures /*= false*/) {
	// if(free_shader) shader_
	compute.pipeline.release();
}
void release_computer(computer* compute, bool free_shader /*= true*/, bool free_gpu_buffers /*= false*/, bool free_textures /*= false*/) {
	release_computer(*compute, free_shader, free_gpu_buffers, free_textures);
}

void quick_dispatch(waylib_state state, std::span<gpu_buffer> gpu_buffers, std::span<texture> textures, shader compute_shader, vec3u workgroups) {
	computer compute {
		.buffer_count = (index_t)gpu_buffers.size(),
		.buffers = gpu_buffers.data(),
		.texture_count = (index_t)textures.size(),
		.textures = textures.data(),
		.shader = compute_shader
	};
	computer_upload(state, compute);
	computer_dispatch(state, compute, workgroups);
	release_computer(compute, true, false, false);
}
void quick_dispatch(waylib_state state, gpu_buffer* gpu_buffers, size_t gpu_buffer_size, texture* textures, size_t texture_size, shader compute_shader, vec3u workgroups) {
	quick_dispatch(state, {gpu_buffers, gpu_buffer_size}, {textures, texture_size}, compute_shader, workgroups);
}

#ifdef WAYLIB_NAMESPACE_NAME
}
#endif