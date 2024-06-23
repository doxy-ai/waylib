#pragma once
#include <cstdint>
#include <cstddef>
#include <webgpu/webgpu.hpp>

#include "wgsl_types.hpp"

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

#ifdef WAYLIB_NAMESPACE_NAME
}
#endif