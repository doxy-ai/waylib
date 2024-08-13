#include "waylib.hpp"

#define TCPP_IMPLEMENTATION
#include "thirdparty/tcppLibrary.hpp"

#include <optional>
#include <fstream>

#ifdef WAYLIB_NAMESPACE_NAME
namespace WAYLIB_NAMESPACE_NAME {
#endif

namespace detail {
	std::string guarded_defines(shader_preprocessor* processor) {
		return "#ifndef __WAYLIB_INTERNAL_DEFINES__\n#define __WAYLIB_INTERNAL_DEFINES__\n" + processor->defines + "\n#endif";
	}

	std::string remove_whitespace(const std::string& in) {
		std::string out;
		for(char c: in)
			if(!std::isspace(c))
				out += c;
		return out;
	}

	std::string process_pragma_once(std::string data, const std::filesystem::path& path) {
		if(auto once = data.find("#pragma once"); once != std::string::npos) {
			std::string guard = path.string();
			std::transform(guard.begin(), guard.end(), guard.begin(), [](char c) -> char {
				if(!std::isalnum(c)) return '_';
				return std::toupper(c);
			});
			guard = "__" + guard + "_GUARD__";
			data.replace(once, 12, "#ifndef " + guard + "\n#define " + guard + "\n");
			data += "#endif //" + guard + "\n";
		}
		return data;
	}

	std::optional<std::string> read_entire_file(std::filesystem::path path, const preprocess_shader_config& config = {}) {
		std::ifstream fin(path);
		if(!fin) {
			set_error_message("Failed to open file `" + path.string() + "`... does it exist?");
			return {};
		}
		fin.seekg(0, std::ios::end);
		size_t size = fin.tellg();
		fin.seekg(0, std::ios::beg);

		std::string data(size + 1, '\n');
		fin.read(data.data(), size);
		if(config.support_pragma_once)
			return process_pragma_once(data, path);
		return data;
	}

	std::optional<std::string> preprocess_shader(shader_preprocessor* processor, const std::filesystem::path& path, const preprocess_shader_config& config);

	std::optional<std::string> preprocess_shader_from_memory(shader_preprocessor* processor, const std::string& _data, const preprocess_shader_config& config) {
		struct PreprocessFailed {};

		auto data = std::make_unique<tcpp::StringInputStream>(guarded_defines(processor) + _data + "\n");
		tcpp::Lexer lexer(std::move(data));
		try {
			tcpp::Preprocessor preprocessor(lexer, {[](const tcpp::TErrorInfo& info){
				auto error = tcpp::ErrorTypeToString(info.mType);
				if (error.empty()) error = "Unknown error";
				error += " on line: " + std::to_string(info.mLine);
				// TODO: Include that line of the input!
				set_error_message(error);
				throw PreprocessFailed{};
			}, [&](const std::string& _path, bool isSystem){
				std::filesystem::path path = _path;
				if(processor->file_cache.find(path) != processor->file_cache.end())
					return std::make_unique<tcpp::StringInputStream>(processor->file_cache[path]);

				#define WAYLIB_NON_SYSTEM_PATHS {\
					if(config.path) {\
						auto relativeToConfig = std::filesystem::absolute(std::filesystem::path(config.path).parent_path() / path);\
						if(std::filesystem::exists(relativeToConfig)){\
							if(auto res = read_entire_file(relativeToConfig, config); res)\
								return std::make_unique<tcpp::StringInputStream>(*res);\
							else throw PreprocessFailed{};\
						}\
					}\
					auto absolute = std::filesystem::absolute(path);\
					if(std::filesystem::exists(absolute)) {\
						if(auto res = read_entire_file(absolute, config); res)\
							return std::make_unique<tcpp::StringInputStream>(*res);\
						else throw PreprocessFailed{};\
					}\
				}
				if(!isSystem) WAYLIB_NON_SYSTEM_PATHS

				for(auto system: processor->search_paths) {
					system = system / path;
					if(std::filesystem::exists(system)) {
						if(auto res = read_entire_file(system, config); res)
							return std::make_unique<tcpp::StringInputStream>(*res);
						else throw PreprocessFailed{};
					}
				}

				if(isSystem) WAYLIB_NON_SYSTEM_PATHS
				#undef WAYLIB_NON_SYSTEM_PATHS

				set_error_message("Included file `" + path.string() + "` could not be found!");
				throw PreprocessFailed{};
			}, config.remove_comments});

			if(config.remove_whitespace) return remove_whitespace(preprocessor.Process());
			return preprocessor.Process();
		} catch(PreprocessFailed) {
			return {};
		}
	}

	std::optional<std::string> preprocess_shader_from_memory_and_cache(shader_preprocessor* processor, const std::string& _data, const std::filesystem::path& path, preprocess_shader_config config) {
		std::string data;
		if(config.support_pragma_once) data = process_pragma_once(_data, path);
		else data = _data;

		config.path = path.string().c_str();
		if(auto res = preprocess_shader_from_memory(processor, data, config); res) {
			processor->file_cache[path] = data;
			return *res;
		}
		return {};
	}

	std::optional<std::string> preprocess_shader(shader_preprocessor* processor, const std::filesystem::path& path, const preprocess_shader_config& config) {
		if(processor->file_cache.find(path) != processor->file_cache.end())
			return processor->file_cache[path];

		auto data = read_entire_file(path, config);
		if(!data) return {};
		return preprocess_shader_from_memory_and_cache(processor, *data, path, config);
	}

	void preprocessor_initialize_platform_defines(shader_preprocessor* preprocessor, wgpu::Adapter adapter) {
		constexpr static auto quote = [](const std::string& s) { return "\"" + s + "\""; };
		wgpu::AdapterProperties p;
		adapter.getProperties(&p);
		preprocessor->defines += "#define WGPU_VENDOR_ID " + std::to_string(p.vendorID) + "\n";
		preprocessor->defines += "#define WGPU_VENDOR_NAME " + quote(p.vendorName) + "\n";
		preprocessor->defines += "#define WGPU_ARCHITECTURE " + quote(p.architecture) + "\n";
		preprocessor->defines += "#define WGPU_DEVICE_ID " + std::to_string(p.deviceID) + "\n";
		preprocessor->defines += "#define WGPU_NAME " + quote(p.name) + "\n";
		preprocessor->defines += "#define WGPU_DRIVER_DESCRIPTION " + quote(p.driverDescription) + "\n";
		preprocessor->defines += "#define WGPU_ADAPTER_TYPE " + std::to_string(p.adapterType) + "\n";
		preprocessor->defines += "#define WGPU_BACKEND_TYPE " + std::to_string(p.backendType) + "\n";
		preprocessor->defines += "#define WGPU_COMPATIBILITY_MODE " + std::to_string(p.compatibilityMode) + "\n";
		
		preprocessor->defines += "#define WGPU_ADAPTER_TYPE_DISCRETE_GPU " + std::to_string(wgpu::AdapterType::DiscreteGPU) + "\n";
		preprocessor->defines += "#define WGPU_ADAPTER_TYPE_INTEGRATED_GPU " + std::to_string(wgpu::AdapterType::IntegratedGPU) + "\n";
		preprocessor->defines += "#define WGPU_ADAPTER_TYPE_CPU " + std::to_string(wgpu::AdapterType::CPU) + "\n";

		preprocessor->defines += "#define WGPU_BACKEND_TYPE_WEBGPU " + std::to_string(wgpu::BackendType::WebGPU) + "\n";
		preprocessor->defines += "#define WGPU_BACKEND_TYPE_D3D11 " + std::to_string(wgpu::BackendType::D3D11) + "\n";
		preprocessor->defines += "#define WGPU_BACKEND_TYPE_D3D12 " + std::to_string(wgpu::BackendType::D3D12) + "\n";
		preprocessor->defines += "#define WGPU_BACKEND_TYPE_METAL " + std::to_string(wgpu::BackendType::Metal) + "\n";
		preprocessor->defines += "#define WGPU_BACKEND_TYPE_VULKAN " + std::to_string(wgpu::BackendType::Vulkan) + "\n";
		preprocessor->defines += "#define WGPU_BACKEND_TYPE_OPENGL " + std::to_string(wgpu::BackendType::OpenGL) + "\n";
		preprocessor->defines += "#define WGPU_BACKEND_TYPE_OPENGLES " + std::to_string(wgpu::BackendType::OpenGLES) + "\n";

		preprocessor->defines += "#define WAYLIB_LIGHT_TYPE_DIRECTIONAL " "0" "\n";
		preprocessor->defines += "#define WAYLIB_LIGHT_TYPE_POINT " "1" "\n";
		preprocessor->defines += "#define WAYLIB_LIGHT_TYPE_SPOT " "2" "\n";

		p.freeMembers();
	}
}

#ifdef WAYLIB_NAMESPACE_NAME
}
#endif