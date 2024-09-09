#ifndef WAYLIB_TEXTURE_IS_AVAILABLE
#define WAYLIB_TEXTURE_IS_AVAILABLE

#include "common.h"

#ifdef __cplusplus
extern "C" {
#endif

WAYLIB_OPTIONAL(image) load_image(
	const char* file_path
);

WAYLIB_OPTIONAL(image) load_image_from_memory(
	const unsigned char* data, size_t size
);

WAYLIB_OPTIONAL(image) load_images_as_frames(
	const char** paths, size_t paths_size
);

WAYLIB_OPTIONAL(image) merge_color_and_alpha_images(
	const image* color, 
	const image* alpha
);

#ifdef __cplusplus
} // End extern "C"
#endif

#endif // WAYLIB_TEXTURE_IS_AVAILABLE