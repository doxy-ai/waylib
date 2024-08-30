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

void upload_buffer(wgpu_state state, buffer& buffer, WGPUBufferUsageFlags usage /*= wgpu::BufferUsage::CopyDst | wgpu::BufferUsage::Storage*/, bool free_cpu_data /*= false*/) {
	if(buffer.data == nullptr) {
		buffer.data = state.device.createBuffer(WGPUBufferDescriptor {
			.label = buffer.label ? buffer.label : "Generic Waylib Buffer",
			.usage = usage,
			.size = buffer.offset + buffer.size,
			.mappedAtCreation = false,
		});
		if(buffer.cpu_data) {
			state.device.getQueue().writeBuffer(buffer.data, buffer.offset, buffer.cpu_data, buffer.size);
			// memcpy(buffer.data.getMappedRange(buffer.offset, buffer.size), buffer.cpu_data, buffer.size);
			// buffer.data.unmap();
		}
	} else state.device.getQueue().writeBuffer(buffer.data, buffer.offset, buffer.cpu_data, buffer.size);

	if(free_cpu_data) free(buffer.cpu_data);
}
void upload_buffer(wgpu_state state, buffer* buffer, WGPUBufferUsageFlags usage /*= WGPUBufferUsage_CopyDst | WGPUBufferUsage_Storage*/, bool free_cpu_data /*= false*/){
	upload_buffer(state, *buffer, usage, free_cpu_data);
}

buffer create_buffer(wgpu_state state, std::span<std::byte> data, WGPUBufferUsageFlags usage /*= wgpu::BufferUsage::CopyDst | wgpu::BufferUsage::Storage*/, WAYLIB_OPTIONAL(std::string_view) label /*= {}*/) {
	buffer out = {
		.size = data.size(),
		.offset = 0,
		.cpu_data = (uint8_t*)data.data(),
		.data = nullptr,
		.label = label.has_value ? cstring_from_view(label.value) : nullptr
	};
	upload_buffer(state, out, usage, false);
	return out;
}
buffer create_buffer(wgpu_state state, void* data, size_t size, WGPUBufferUsageFlags usage /*= WGPUBufferUsage_CopyDst | WGPUBufferUsage_Storage*/, const char* label /*= nullptr*/) {
	return create_buffer(state, {(std::byte*)data, size}, usage, label ? WAYLIB_OPTIONAL(std::string_view){label} : WAYLIB_OPTIONAL(std::string_view){});
}


void* buffer_map(wgpu_state state, buffer& buffer, WGPUMapModeFlags mode /*= wgpu::MapMode::None*/) {
	if(mode != wgpu::MapMode::None) {
		bool done = false;
		// TODO: Update API
		auto handle = buffer.data.mapAsync(mode, buffer.offset, buffer.size, [&](wgpu::BufferMapAsyncStatus status) {
			done = true;
		});
		while(!done) state.instance.processEvents();
	}
	return buffer.data.getMappedRange(buffer.offset, buffer.size);
}
void* buffer_map(wgpu_state state, buffer* buffer, WGPUMapModeFlags mode /*= WGPUMapMode_None*/) {
	return buffer_map(state, *buffer, mode);
}

const void* buffer_map_const(wgpu_state state, buffer& buffer, WGPUMapModeFlags mode /*= wgpu::MapMode::None*/) {
	if(mode != wgpu::MapMode::None) {
		bool done = false;
		// TODO: Update API
		auto handle = buffer.data.mapAsync(mode, buffer.offset, buffer.size, [&](wgpu::BufferMapAsyncStatus status) {
			done = true;
		});
		while(!done) state.instance.processEvents();
	}
	return buffer.data.getConstMappedRange(buffer.offset, buffer.size);
}
const void* buffer_map_const(wgpu_state state, buffer* buffer, WGPUMapModeFlags mode /*= wgpu::MapMode::None*/) {
	return buffer_map_const(state, *buffer, mode);
}

void buffer_unmap(buffer& buffer) {
	buffer.data.unmap();
}
void buffer_unmap(buffer* buffer) {
	buffer->data.unmap();
}

void buffer_release(buffer& buffer) {
	if(buffer.heap_allocated && buffer.cpu_data) free(buffer.cpu_data);
	buffer.data.destroy(); // TODO: Needed?
	buffer.data.release();
}
void buffer_release(buffer* buffer) {
	buffer_release(*buffer);
}

void buffer_copy_record_existing(WGPUCommandEncoder _encoder, buffer& dest, buffer& source) {
	wgpu::CommandEncoder encoder = _encoder;
	encoder.copyBufferToBuffer(source.data, source.offset, dest.data, dest.offset, dest.size);
}
void buffer_copy_record_existing(WGPUCommandEncoder encoder, buffer* dest, buffer* source) {
	buffer_copy_record_existing(encoder, *dest, *source);
}

void buffer_copy(wgpu_state state, buffer& dest, buffer& source) {
	auto encoder = state.device.createCommandEncoder();
	buffer_copy_record_existing(encoder, dest, source);
	state.device.getQueue().submit(encoder.finish());
	encoder.release();
}
void buffer_copy(wgpu_state state, buffer* dest, buffer* source) {
	buffer_copy(state, *dest, *source);
}

// You must clear a landing pad for the data (set cpu_data to null) before calling this function
void buffer_download(wgpu_state state, buffer& buffer, bool create_intermediate_buffer /*= true*/) {
	assert(buffer.cpu_data == nullptr);
	if(create_intermediate_buffer) {
		struct buffer out{
			.size = buffer.size,
			.offset = 0,
			.label = "Intermediate"
		};
		upload_buffer(state, out, wgpu::BufferUsage::CopyDst | wgpu::BufferUsage::MapRead);
		buffer_copy(state, out, buffer);
		buffer_download(state, out, false);
		buffer.cpu_data = std::exchange(out.cpu_data, nullptr);
		buffer.heap_allocated = std::exchange(out.heap_allocated, false);
		buffer_release(out);
	} else {
		buffer.cpu_data = new uint8_t[buffer.size];
		buffer.heap_allocated = true;
		auto src = buffer_map_const(state, buffer, wgpu::MapMode::Read);
		memcpy(buffer.cpu_data, src, buffer.size);
		buffer_unmap(buffer);
	}
}
void buffer_download(wgpu_state state, buffer* buffer, bool create_intermediate_buffer /*= true*/){
	buffer_download(state, *buffer, create_intermediate_buffer);
}

// TODO: Add buffer->image and image->buffer functions

//////////////////////////////////////////////////////////////////////
// #Computer
//////////////////////////////////////////////////////////////////////

void upload_computer(wgpu_state state, computer& compute, WAYLIB_OPTIONAL(std::string_view) label /*= {}*/) {
	wgpu::ComputePipelineDescriptor computePipelineDesc = wgpu::Default;
	computePipelineDesc.label = label.has_value ? cstring_from_view(label.value) : "Waylib Compute Pipeline";
	computePipelineDesc.compute.module = compute.shader.module;
	computePipelineDesc.compute.entryPoint = compute.shader.compute_entry_point;
	if(compute.pipeline) compute.pipeline.release();
	compute.pipeline = state.device.createComputePipeline(computePipelineDesc);
}
void upload_computer(wgpu_state state, computer* compute, const char* label /*= nullptr*/) {
	upload_computer(state, *compute, label ? WAYLIB_OPTIONAL(std::string_view){label} : WAYLIB_OPTIONAL(std::string_view){});
}

WAYLIB_OPTIONAL(const texture*) get_default_texture(wgpu_state state);

computer_recording_state computer_record_existing(wgpu_state state, WGPUCommandEncoder _encoder, computer& compute, vec3u workgroups, bool end_pass /*= true*/) {
	const static texture& default_texture = *throw_if_null(get_default_texture(state));
	std::vector<wgpu::BindGroupEntry> bufferEntries(compute.buffer_count, wgpu::Default);
	for(size_t i = 0; i < compute.buffer_count; ++i) {
		bufferEntries[i].binding = i;
		bufferEntries[i].buffer = compute.buffers[i].data;
		bufferEntries[i].offset = compute.buffers[i].offset;
		bufferEntries[i].size = compute.buffers[i].size;
	}
	auto bufferBindGroup = state.device.createBindGroup(WGPUBindGroupDescriptor{
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
		pass.setBindGroup(0, bufferBindGroup, 0, nullptr);
		pass.setBindGroup(1, textureBindGroup, 0, nullptr);
		pass.dispatchWorkgroups(workgroups.x, workgroups.y, workgroups.z);
	}
	if(end_pass) pass.end();
	return {pass, bufferBindGroup};
}
computer_recording_state computer_record_existing(wgpu_state state, WGPUCommandEncoder encoder, computer* compute, vec3u workgroups, bool end_pass /*= true*/) {
	return computer_record_existing(state, encoder, *compute, workgroups, end_pass);
}

void computer_dispatch(wgpu_state state, computer& compute, vec3u workgroups) {
	auto encoder = state.device.createCommandEncoder();
	auto record_state = computer_record_existing(state, encoder, compute, workgroups);
	state.device.getQueue().submit(encoder.finish());
	record_state.pass.release();
	encoder.release();
	record_state.bind_group.release();
}
void computer_dispatch(wgpu_state state, computer* compute, vec3u workgroups) {
	computer_dispatch(state, *compute, workgroups);
}

void release_computer(computer& compute, bool free_shader /*= true*/, bool free_buffers /*= false*/, bool free_textures /*= false*/) {
	// if(free_shader) shader_
	compute.pipeline.release();
}
void release_computer(computer* compute, bool free_shader /*= true*/, bool free_buffers /*= false*/, bool free_textures /*= false*/) {
	release_computer(*compute, free_shader, free_buffers, free_textures);
}

void quick_dispatch(wgpu_state state, std::span<buffer> buffers, std::span<texture> textures, shader compute_shader, vec3u workgroups) {
	computer compute {
		.buffer_count = (index_t)buffers.size(),
		.buffers = buffers.data(),
		.texture_count = (index_t)textures.size(),
		.textures = textures.data(),
		.shader = compute_shader
	};
	upload_computer(state, compute);
	computer_dispatch(state, compute, workgroups);
	release_computer(compute, true, false, false);
}
void quick_dispatch(wgpu_state state, buffer* buffers, size_t buffer_size, texture* textures, size_t texture_size, shader compute_shader, vec3u workgroups) {
	quick_dispatch(state, {buffers, buffer_size}, {textures, texture_size}, compute_shader, workgroups);
}

#ifdef WAYLIB_NAMESPACE_NAME
}
#endif