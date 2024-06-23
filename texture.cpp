#include "texture.hpp"

// #include <filesystem>

#define STB_IMAGE_IMPLEMENTATION
#include "thirdparty/stb_image.h"

#define TINYEXR_IMPLEMENTATION
#include "thirdparty/tinyexr.h"

#ifdef WAYLIB_NAMESPACE_NAME
namespace WAYLIB_NAMESPACE_NAME {
#endif

namespace detail {
	template<typename F>
	WAYLIB_OPTIONAL(image) load_stb_image(F loader, bool hdr) {
		image image;
		int channels;
		image.data = loader(&image.width, &image.height, &channels);
		if(image.data == nullptr) {
			std::cerr << "Error: " << stbi_failure_reason() << std::endl;
			return {};
		}

		if(hdr) image.format = channels == 4 ? image_format::RGBAF32 : image_format::RGBAF32;
		else image.format = channels == 4 ? image_format::RGBA8 : image_format::RGBA8;
		image.mipmaps = 0;
		return image;
	}

	template<typename F>
	WAYLIB_OPTIONAL(image) load_exr(F loader) {
		image image;
		const char* err = NULL; // or nullptr in C++11

		int ret = loader((float**)&image.data, &image.width, &image.height, &err);
		if (ret != TINYEXR_SUCCESS) {
			if (err) {
				std::cerr << "Error: " << stbi_failure_reason() << std::endl;
				FreeEXRErrorMessage(err); // release memory of error message.
			}
			return {};
		}

		image.format = image_format::RGBAF32;
		image.mipmaps = 0;
		return image;
	}

	// std::string get_extension(const char* file_path) {
	// 	return std::filesystem::path(file_path).extension();
	// }
}


WAYLIB_OPTIONAL(image) load_image(const char* file_path) {
	// auto extension = detail::get_extension(file_path);
	if(stbi_is_hdr(file_path))
		return detail::load_stb_image([file_path](int* x, int* y, int* c) { return stbi_loadf(file_path, x, y, c, 4); }, true);
	else if(IsEXR(file_path) == TINYEXR_SUCCESS)
		return detail::load_exr([file_path](float** data, int* x, int* y, const char** err) { return LoadEXR(data, x, y, file_path, err); });
	else return detail::load_stb_image([file_path](int* x, int* y, int* c) { return stbi_load(file_path, x, y, c, 4); }, false);
}

WAYLIB_OPTIONAL(image) load_image_from_memory(const unsigned char* data, size_t size) {
	if(stbi_is_hdr_from_memory(data, size))
		return detail::load_stb_image([data, size](int* x, int* y, int* c) { return stbi_loadf_from_memory(data, size, x, y, c, 4); }, true);
	else if(IsEXRFromMemory(data, size) == TINYEXR_SUCCESS)
		return detail::load_exr([data, size](float** out, int* x, int* y, const char** err) { return LoadEXRFromMemory(out, x, y, data, size, err); });
	else return detail::load_stb_image([data, size](int* x, int* y, int* c) { return stbi_load_from_memory(data, size, x, y, c, 4); }, false);
}
WAYLIB_OPTIONAL(image) load_image_from_memory(std::span<std::byte> data) {
	return load_image_from_memory((unsigned char*)data.data(), data.size());
}

#ifdef WAYLIB_NAMESPACE_NAME
}
#endif