#pragma once
#include "common.hpp"

#ifdef WAYLIB_NAMESPACE_NAME
namespace WAYLIB_NAMESPACE_NAME {
#endif

#include "texture.h"


WAYLIB_OPTIONAL(image) load_image_from_memory(
	std::span<std::byte> data
);

WAYLIB_OPTIONAL(image) load_images_as_frames(std::span<std::string_view> paths);

#ifdef WAYLIB_NAMESPACE_NAME
}
#endif