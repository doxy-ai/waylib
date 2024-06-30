#pragma once
#include <cstdint>
#include <cstddef>
#include <array>
#include <webgpu/webgpu.hpp>

#include "wgsl_types.hpp"

#ifdef __cpp_exceptions
#include <stdexcept>
#endif

#ifdef WAYLIB_NAMESPACE_NAME
namespace WAYLIB_NAMESPACE_NAME {
inline namespace wgpu {
#else
namespace wgpu {
#endif

	using namespace ::wgpu;

	using instance = Instance;
	using adapter = Adapter;
	using device = Device;
	using surface = Surface;
	using queue = Queue;
}

#include "waylib.h"

wgpu::Color to_webgpu(const color8bit& color);
wgpu::Color to_webgpu(const color32bit& color);

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

void time_calculations(time& time);

#ifdef WAYLIB_NAMESPACE_NAME
}
#endif