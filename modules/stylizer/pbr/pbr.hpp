#pragma once

#include "stylizer/core/core.hpp"
#include "stylizer/core/interfaces.h"
#include <cstddef>

STYLIZER_BEGIN_NAMESPACE

	extern "C" {
		#include "pbr.h"
	}

	namespace pbr {

		using geometry_bufferC = pbr_geometry_bufferC;
		using geometry_buffer_config = pbr_geometry_buffer_config;
		using materialC = pbr_materialC;
		using texture_slot = pbr_texture_slot;

		shader_preprocessor& initialize_shader_preprocessor_virtual_filesystem(shader_preprocessor& p, const shader_preprocessor::config &config = {});
		shader_preprocessor& initialize_shader_preprocessor_defines(shader_preprocessor& p);
		inline shader_preprocessor& initialize_shader_preprocessor(shader_preprocessor& p, const shader_preprocessor::config &config = {}) {
			return initialize_shader_preprocessor_defines(initialize_shader_preprocessor_virtual_filesystem(p, config));
		}

		struct geometry_buffer: public pbr::geometry_bufferC {
			STYLIZER_GENERIC_AUTO_RELEASE_SUPPORT_SUB_NAMESPACE(geometry_buffer, pbr)

			geometry_buffer(geometry_buffer&& other) { *this = std::move(other); }
			geometry_buffer& operator=(geometry_buffer&& other) {
				previous = std::exchange(other.previous, nullptr);
				c().color = std::exchange(other.c().color, {});
				c().depth = std::exchange(other.c().depth, {});
				c().normal = std::exchange(other.c().normal, {});
				c().packed = std::exchange(other.c().packed, {});
				return *this;
			}
			texture& color() { return *(texture*)&(c().color); }
			texture& depth() { return *(texture*)&(c().depth); }
			texture& normal() { return *(texture*)&(c().normal); }
			texture& packed() { return *(texture*)&(c().packed); }
			operator bool() { return color() && depth() && normal() && packed(); }

			static geometry_buffer create_default(wgpu_state& state, vec2u size, pbr::geometry_buffer_config config = {}) {
				geometry_buffer out{};
				if(config.base.color_format == wgpu::TextureFormat::Undefined && state.surface_format != wgpu::TextureFormat::Undefined)
					config.base.color_format = state.surface_format;
				out.color() = texture::create(state, size,
					{.format = config.base.color_format, .usage = wgpu::TextureUsage::TextureBinding | wgpu::TextureUsage::RenderAttachment | wgpu::TextureUsage::CopySrc | wgpu::TextureUsage::CopyDst, .sampler_type = texture_create_sampler_type::None},
					{"Stylizer PBR Geometry Buffer Color Texture"}
				);
				out.depth() = texture::create(state, size,
					{.format = config.base.depth_format, .usage = wgpu::TextureUsage::TextureBinding | wgpu::TextureUsage::RenderAttachment | wgpu::TextureUsage::CopySrc | wgpu::TextureUsage::CopyDst, .sampler_type = texture_create_sampler_type::None},
					{"Stylizer PBR Geometry Buffer Depth Texture"}
				);
				out.normal() = texture::create(state, size,
					{.format = config.base.normal_format, .usage = wgpu::TextureUsage::TextureBinding | wgpu::TextureUsage::RenderAttachment | wgpu::TextureUsage::CopySrc | wgpu::TextureUsage::CopyDst, .sampler_type = texture_create_sampler_type::None},
					{"Stylizer PBR Geometry Buffer Normal Texture"}
				);
				out.packed() = texture::create(state, size,
					{.format = config.packed_format, .usage = wgpu::TextureUsage::TextureBinding | wgpu::TextureUsage::RenderAttachment | wgpu::TextureUsage::CopySrc | wgpu::TextureUsage::CopyDst, .sampler_type = texture_create_sampler_type::None},
					{"Stylizer PBR Geometry Buffer Packed Texture"}
				);
				out.previous = config.base.previous;
				return out;
			}

			void release() {
				if(color().gpu_data) color().release();
				if(depth().gpu_data) depth().release();
				if(normal().gpu_data) normal().release();
				if(packed().gpu_data) packed().release();
			}

			std::array<WGPUColorTargetState, 3> targets() {
				return std::array<WGPUColorTargetState, 3>{
					color().default_color_target_state(),
					normal().no_blend_color_target_state(),
					packed().no_blend_color_target_state(),
				};
			}

			std::span<const gpu_buffer> buffers() { return {(gpu_buffer*)nullptr, 0}; }
			std::span<const texture> textures() { return {&color(), 4}; }

			wgpu::BindGroup bind_group(drawing_state& draw, sl::materialC& mat) {
				// Bind utility buffer
				std::array<WGPUBindGroupEntry, 9> entries = {
					draw.utility_buffer_bind_group_entry()
				};

				// Bind previous textures
				uint32_t binding = 1;
				bind_group_bind_previous_textures(draw.state(), static_cast<geometry_buffer*>(previous), 4, entries, binding);

				return draw.state().device.createBindGroup(WGPUBindGroupDescriptor{ // TODO: free when done somehow...
					.label = toWGPU("Stylizer PBR Gbuffer Bind Group"),
					.layout = mat.pipeline.getBindGroupLayout(0),
					.entryCount = entries.size(),
					.entries = entries.data()
				});
			}

			geometry_buffer& resize(wgpu_state& state, vec2u size) {
				auto _new = create_default(state, size, {.base = {
					.color_format = color().gpu_data.getFormat(),
					.depth_format = depth().gpu_data.getFormat(),
					.normal_format = normal().gpu_data.getFormat(),
				}, .packed_format = packed().gpu_data.getFormat()});
				_new.previous = std::exchange(previous, nullptr);

				release();
				*this = std::move(_new);
				return *this;
			}

			geometry_buffer clone_record_existing(wgpu_state& state, wgpu::CommandEncoder encoder) {
				auto out = create_default(state, color().size(), {.base ={
					.color_format = color().gpu_data.getFormat(),
					.depth_format = depth().gpu_data.getFormat(),
					.normal_format = normal().gpu_data.getFormat(),
				}, .packed_format = packed().gpu_data.getFormat()});
				out.previous = std::exchange(previous, nullptr);

				out.color().copy_record_existing(encoder, color());
				out.depth().copy_record_existing(encoder, depth());
				out.normal().copy_record_existing(encoder, normal());
				out.packed().copy_record_existing(encoder, packed());
				return out;
			}
			geometry_buffer clone(wgpu_state& state) {
				auto_release encoder = state.device.createCommandEncoder();

				auto out = clone_record_existing(state, encoder);

				auto_release commands = encoder.finish();
				state.device.getQueue().submit(commands);
				return out;
			}

			drawing_state begin_drawing(wgpu_state& state, STYLIZER_OPTIONAL(colorC) clear_color = {}, STYLIZER_OPTIONAL(gpu_buffer&) utility_buffer = {}) {
				drawing_stateC out {.state = &state, .gbuffer = this};
				// Create command encoders for the draw call
				std::tie(out.render_encoder, out.pre_encoder) = begin_drawing_create_command_encoders(state);

				// Create the render pass
				std::array<wgpu::RenderPassColorAttachment, 3> colorAttachments = {
					begin_drawing_create_color_attachment(color().view, clear_color),
					begin_drawing_create_color_attachment(normal().view, clear_color),
					begin_drawing_create_color_attachment(packed().view, clear_color)
				};
				out.render_pass = begin_drawing_create_render_pass(out.render_encoder, colorAttachments, begin_drawing_create_depth_attachment(depth().view, clear_color));
				static finalizer_list finalizers = {};
				out.finalizers = &finalizers;

				// Set the bind group callback
				out.gbuffer_bind_group_function = +[](drawing_stateC* draw_, sl::materialC* mat) {
					auto& draw = *(drawing_state*)draw_;
					auto& gbuffer = (geometry_buffer&)draw.gbuffer();
					return gbuffer.bind_group(draw, *mat);
				};
				if(utility_buffer) out.utility_buffer = static_cast<gpu_bufferC*>(&utility_buffer.value);
				return out;
			}
		};


		struct material: public materialC {
			STYLIZER_GENERIC_AUTO_RELEASE_SUPPORT_SUB_NAMESPACE(material, pbr)
			inline sl::material& core() { return *(sl::material*)this;}

			material(material&& other) { *this = std::move(other); }
			material& operator=(material&& other) {
				core().operator=(std::move(other.core()));

				albedo_texture = std::exchange(other.albedo_texture, nullptr);
				emission_texture = std::exchange(other.emission_texture, nullptr);
				normal_texture = std::exchange(other.normal_texture, nullptr);
				packed_texture = std::exchange(other.packed_texture, nullptr);
				roughness_texture = std::exchange(other.roughness_texture, nullptr);
				metalness_texture = std::exchange(other.metalness_texture, nullptr);
				ambient_occlusion_texture = std::exchange(other.ambient_occlusion_texture, nullptr);
				environment_texture = std::exchange(other.environment_texture, nullptr);

				albedo = other.albedo;
				emission = other.emission;
				roughness = other.roughness;
				metalness = other.metalness;
				normal_strength = other.normal_strength;
				return *this;
			}

			std::span<gpu_buffer> buffers() { return core().buffers(); }
			std::span<texture> textures() { return core().textures(); }
			std::span<STYLIZER_MANAGEABLE(texture*)> pbr_textures() { return {(STYLIZER_MANAGEABLE(texture*)*)(&c().albedo_texture), 8};}
			std::span<shader> shaders() { return core().shaders(); }
			operator bool() { return pipeline; }

			constexpr static size_t gpu_size = sizeof(pbr_materialC) - offsetof(pbr_materialC, material_id);

			material& update_settings_buffer(wgpu_state& state) {
				used_textures = None;
				if(*albedo_texture) used_textures = static_cast<texture_slot>(used_textures | texture_slot::Albedo);
				if(*emission_texture) used_textures = static_cast<texture_slot>(used_textures | texture_slot::Emission);
				if(*normal_texture) used_textures = static_cast<texture_slot>(used_textures | texture_slot::Normal);
				if(*packed_texture) used_textures = static_cast<texture_slot>(used_textures | texture_slot::Packed);
				if(*roughness_texture) used_textures = static_cast<texture_slot>(used_textures | texture_slot::Roughness);
				if(*metalness_texture) used_textures = static_cast<texture_slot>(used_textures | texture_slot::Metalness);
				if(*ambient_occlusion_texture) used_textures = static_cast<texture_slot>(used_textures | texture_slot::Ambient_Occlusion);
				if(*environment_texture) used_textures = static_cast<texture_slot>(used_textures | texture_slot::Environment);

				if(!settings_buffer)
					settings_buffer = gpu_buffer::create(state, {(std::byte*)&material_id, gpu_size}).gpu_data;
				else {
					gpu_buffer(gpu_bufferC{
						.size = gpu_size,
						.offset = 0,
						.gpu_data = settings_buffer,
					}).write(state, {(std::byte*)&material_id, gpu_size});
				}
				return *this;
			}

			material& rebind_bind_group(wgpu_state& state, bool update_settings = true) {
				auto& default_texture = const_cast<texture&>(texture::default_texture(state));
				auto& default_cube_texture = const_cast<cube_texture&>(cube_texture::default_cube_texture(state));

				std::vector<WGPUBindGroupEntry> entries;
				uint32_t binding = 0;
				// Bind settings buffer
				if(update_settings) update_settings_buffer(state);
				entries.emplace_back(WGPUBindGroupEntry{
					.binding = binding++,
					.buffer = settings_buffer,
					.offset = 0,
					.size = gpu_size,
				});
				// Bind core textures
				for(size_t i = 0, size = pbr_textures().size(); i < size; ++i) {
					auto& texture_ = *pbr_textures()[i];
					texture& texture = texture_ ? *texture_ 
						: (i == pbr_textures().size() - 1 ? default_cube_texture : default_texture);
					entries.emplace_back(WGPUBindGroupEntry{
						.binding = binding++,
						.textureView = texture.view
					});
					wgpu::Sampler sampler = texture.sampler ? texture.sampler : texture::default_sampler(state);
					entries.emplace_back(WGPUBindGroupEntry{ // TODO: Failing?
						.binding = binding++,
						.sampler = sampler
					});
				}

				for(auto& buffer: buffers())
					entries.emplace_back(WGPUBindGroupEntry{
						.binding = binding++,
						.buffer = buffer.gpu_data,
						.offset = buffer.offset,
						.size = buffer.size,
					});
				for(auto& texture: textures()) {
					entries.emplace_back(WGPUBindGroupEntry{
						.binding = binding++,
						.textureView = texture.view
					});
					wgpu::Sampler sampler = texture.sampler ? texture.sampler : texture::default_sampler(state);
					entries.emplace_back(WGPUBindGroupEntry{ // TODO: Failing?
						.binding = binding++,
						.sampler = sampler
					});
				}

				if(bind_group) bind_group.release();
				bind_group = state.device.createBindGroup(WGPUBindGroupDescriptor {
					.label = toWGPU("Stylizer PBR Material Data Bind Group"),
					.layout = pipeline.getBindGroupLayout(2),
					.entryCount = entries.size(),
					.entries = entries.data()
				});
				return *this;
			}


			material& upload(
				wgpu_state& state,
				std::span<const WGPUColorTargetState> gbuffer_targets,
				STYLIZER_OPTIONAL(std::span<WGPUVertexBufferLayout>) mesh_layout = {},
				material_configuration config = {}
			) {
				core().upload_impl(state, gbuffer_targets, mesh_layout, config);
				rebind_bind_group(state); // NOTE: No need to check texture count since we always bind the core textures
				return *this;
			}
			template<GbufferProvider Tgbuffer>
			material& upload(
				wgpu_state& state,
				Tgbuffer& gbuffer,
				STYLIZER_OPTIONAL(std::span<WGPUVertexBufferLayout>) mesh_layout = {},
				material_configuration config = {}
			) {
				return upload(state, gbuffer.targets(), mesh_layout, config);
			}

			void release(bool release_shaders = true) {
				core().release();
				if(*albedo_texture) static_cast<texture*>(*albedo_texture)->release();
				if(*emission_texture) static_cast<texture*>(*emission_texture)->release();
				if(*normal_texture) static_cast<texture*>(*normal_texture)->release();
				if(*packed_texture) static_cast<texture*>(*packed_texture)->release();
				if(*roughness_texture) static_cast<texture*>(*roughness_texture)->release();
				if(*metalness_texture) static_cast<texture*>(*metalness_texture)->release();
				if(*ambient_occlusion_texture) static_cast<texture*>(*ambient_occlusion_texture)->release();
				if(*environment_texture) static_cast<texture*>(*environment_texture)->release();
			}
		};
	}

STYLIZER_END_NAMESPACE