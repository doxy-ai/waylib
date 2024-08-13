#include "texture.hpp"

#include <filesystem>
#include <numeric>
#include <string_view>
#include <vector>

#define STB_IMAGE_IMPLEMENTATION
#include "thirdparty/stb_image.h"

#define TINYEXR_IMPLEMENTATION
#include "thirdparty/tinyexr.h"

#ifdef WAYLIB_NAMESPACE_NAME
namespace WAYLIB_NAMESPACE_NAME {
#endif

//////////////////////////////////////////////////////////////////////
// #Image
//////////////////////////////////////////////////////////////////////

namespace detail {
	template<typename F>
	WAYLIB_OPTIONAL(image) load_stb_image(F loader, bool hdr) {
		image image;
		int channels;
		image.float_data = hdr;
		image.data = (color8*)loader(&image.width, &image.height, &channels);
		if(image.data == nullptr) {
			set_error_message_raw(stbi_failure_reason());
			return {};
		}

		image.mipmaps = 1;
		image.frames = 1;
		return image;
	}

	template<typename F>
	WAYLIB_OPTIONAL(image) load_exr(F loader) {
		image image;
		const char* err = NULL; // or nullptr in C++11

		int ret = loader((float**)&image.data, &image.width, &image.height, &err);
		if (ret != TINYEXR_SUCCESS) {
			if (err) {
				set_error_message_raw(err);
				FreeEXRErrorMessage(err); // release memory of error message.
			}
			return {};
		}

		image.float_data = true;
		image.mipmaps = 1;
		image.frames = 1;
		return image;
	}

	std::string get_extension(const char* file_path) {
		return std::filesystem::path(file_path).extension().string();
	}
}


WAYLIB_OPTIONAL(image) load_image(const char* file_path) WAYLIB_TRY {
	auto extension = detail::get_extension(file_path);
	if(stbi_is_hdr(file_path))
		return detail::load_stb_image([file_path](int* x, int* y, int* c) { return stbi_loadf(file_path, x, y, c, 4); }, true);
	else if(IsEXR(file_path) == TINYEXR_SUCCESS)
		return detail::load_exr([file_path](float** data, int* x, int* y, const char** err) { return LoadEXR(data, x, y, file_path, err); });
	else return detail::load_stb_image([file_path](int* x, int* y, int* c) { return stbi_load(file_path, x, y, c, 4); }, false);
} WAYLIB_CATCH({})

WAYLIB_OPTIONAL(image) load_image_from_memory(const unsigned char* data, size_t size) WAYLIB_TRY {
	if(stbi_is_hdr_from_memory(data, size))
		return detail::load_stb_image([data, size](int* x, int* y, int* c) { return stbi_loadf_from_memory(data, size, x, y, c, 4); }, true);
	else if(IsEXRFromMemory(data, size) == TINYEXR_SUCCESS)
		return detail::load_exr([data, size](float** out, int* x, int* y, const char** err) { return LoadEXRFromMemory(out, x, y, data, size, err); });
	else return detail::load_stb_image([data, size](int* x, int* y, int* c) { return stbi_load_from_memory(data, size, x, y, c, 4); }, false);
} WAYLIB_CATCH({})
WAYLIB_OPTIONAL(image) load_image_from_memory(std::span<std::byte> data) {
	return load_image_from_memory((unsigned char*)data.data(), data.size());
}

WAYLIB_OPTIONAL(wl::image) load_images_as_frames(std::span<std::string_view> paths) {
	std::vector<wl::image> frames;
	for(auto& path: paths) {
		auto res = wl::load_image(wl::cstring_from_view(path.data()));
		if(!res.has_value) return {};
		frames.emplace_back(std::move(res.value));
	}

	auto res = merge_images(frames);
	if(!res.has_value) return {};
	return res.value;  
}
WAYLIB_OPTIONAL(wl::image) load_images_as_frames(const char** _paths, size_t paths_size) {
	std::span<const char*> span{_paths, paths_size};
	std::vector<std::string_view> paths(span.begin(), span.end());
	return load_images_as_frames(paths);
}

#ifdef WAYLIB_NAMESPACE_NAME
}
#endif