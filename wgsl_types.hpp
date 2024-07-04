#ifndef WAYLIB_WGSL_TYPES_IS_AVAILABLE
#define WAYLIB_WGSL_TYPES_IS_AVAILABLE

#include <cstdint>
// #define GLM_FORCE_MESSAGES
#define GLM_FORCE_SWIZZLE
#define GLM_FORCE_SIZE_T_LENGTH
#define GLM_FORCE_DEPTH_ZERO_TO_ONE // Use a clip space with z in [0, 1]
#define GLM_FORCE_UNRESTRICTED_GENTYPE // Allow more operations on integers
#include <glm/vec2.hpp> // glm::vec2
#include <glm/vec3.hpp> // glm::vec3
#include <glm/vec4.hpp> // glm::vec4
#include <glm/mat4x4.hpp> // glm::mat4

#ifdef WAYLIB_NAMESPACE_NAME
namespace WAYLIB_NAMESPACE_NAME {
#endif

using namespace glm;

namespace detail {
	template<size_t Size, typename Type>
	struct vec_t {
		static_assert(false, "You are attempting to use a vector type not supported by WAYLIB!");
		using type = void;
	};

	template<>
	struct vec_t<2, float> { using type = glm::vec<2, float, glm::defaultp>; };
	template<>
	struct vec_t<3, float> { using type = glm::vec<3, float, glm::defaultp>; };
	template<>
	struct vec_t<4, float> { using type = glm::vec<4, float, glm::defaultp>; };

	template<>
	struct vec_t<2, int32_t> { using type = glm::vec<2, int32_t, glm::defaultp>; };
	template<>
	struct vec_t<3, int32_t> { using type = glm::vec<3, int32_t, glm::defaultp>; };
	template<>
	struct vec_t<4, int32_t> { using type = glm::vec<4, int32_t, glm::defaultp>; };

	template<size_t Rows, size_t Cols, typename Type>
	struct mat_t {
		static_assert(false, "You are attempting to use a matrix type not supported by WAYLIB!");
		using type = void;
	};

	template<>
	struct mat_t<4, 4, float> { using type = glm::mat<4, 4, float, glm::defaultp>; };
}

#ifndef WAYLIB_NO_SCALAR_ALIASES
using u8 = uint8_t;
using u16 = uint16_t;
using u32 = uint32_t;
using u64 = uint64_t;
using i8 = int8_t;
using i16 = int16_t;
using i32 = int32_t;
using i64 = int64_t;
using f32 = float;
using f64 = double;
#endif

template<typename T>
using vec2 = typename detail::vec_t<2, T>::type;
template<typename T>
using vec3 = typename detail::vec_t<3, T>::type;
template<typename T>
using vec4 = typename detail::vec_t<4, T>::type;

template<typename T>
using mat4x4 = typename detail::mat_t<4, 4, T>::type;

#define WAYLIB_MATERIALIZE_TYPE(dst, src, constructor) struct dst : public src { using src::constructor; using src::operator=;\
	dst(const dst& other) : src(other) {}\
	dst(const src& other) : src(other) {} }
WAYLIB_MATERIALIZE_TYPE(vec2i, vec2<int32_t>, vec2);
WAYLIB_MATERIALIZE_TYPE(vec2f, vec2<float>, vec2);
WAYLIB_MATERIALIZE_TYPE(vec3f, vec3<float>, vec3);
WAYLIB_MATERIALIZE_TYPE(vec4f, vec4<float>, vec4);
WAYLIB_MATERIALIZE_TYPE(mat4x4f, mat4x4<float>, mat4x4);

#ifdef WAYLIB_NAMESPACE_NAME
} // End namespace
#endif

#endif // WAYLIB_WGSL_TYPES_IS_AVAILABLE