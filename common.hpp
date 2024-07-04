#pragma once
#include <webgpu/webgpu.hpp>

#include "math.hpp"

#include <cstdint>
#include <cstddef>
#include <span>

#ifdef __cpp_exceptions
#include <stdexcept>
#endif

#ifdef WAYLIB_NAMESPACE_NAME
namespace WAYLIB_NAMESPACE_NAME {
#endif

constexpr static WGPUTextureFormat depth_texture_format = wgpu::TextureFormat::Depth24Plus;

#include "common.h"

struct pipeline_globals {
	bool created = false;
	// Group 0 is Instance Data
	// Group 1 is Time Data
	std::array<WGPUBindGroupLayout, 2> bindGroupLayouts;
	wgpu::PipelineLayout layout;
};

std::string get_error_message_and_clear();
#ifdef __cpp_exceptions
	struct error: public std::runtime_error {
		using std::runtime_error::runtime_error;
	};

	template<typename T>
	T throw_if_null(const WAYLIB_OPTIONAL(T)& opt) {
		if(opt.has_value) return opt.value;

		auto msg = get_error_message_and_clear();
		throw error(msg.empty() ? "Null value encountered" : msg);
	}
#else
	template<typename T>
	T throw_if_null(const WAYLIB_OPTIONAL(T)& opt) {
		assert(opt.has_value, get_error_message());
		return opt.value;
	}
#endif
void set_error_message(const std::string_view view);
void set_error_message(const std::string& str);

#ifdef WAYLIB_NAMESPACE_NAME
}
#endif