#include "math.hpp"

#include <cmath>

#ifdef WAYLIB_NAMESPACE_NAME
namespace WAYLIB_NAMESPACE_NAME {
#endif

float sin(radian a) { return std::sin(a); }
float cos(radian a) { return std::cos(a); }
float tan(radian a) { return std::tan(a); }
radian asin(float v) { return std::asin(v); }
radian acos(float v) { return std::acos(v); }
radian atan(float v) { return std::atan(v); }
radian atan2(float y, float x) { return std::atan2(y, x); }
radian atan_vec(vec2f v) { return atan2(v.y, v.x); }
float sinh(radian a) { return std::sinh(a); }
float cosh(radian a) { return std::cosh(a); }
float tanh(radian a) { return std::tanh(a); }
radian asinh(float v) { return std::asinh(v); }
radian acosh(float v) { return std::acosh(v); }
radian atanh(float v) { return std::atanh(v); }

// identity
// abs
// all
// any
// arrayLength
// ceil
// clamp
// countLeadingZeros
// countOneBits
// countTrailingZeros
// cross
// degrees
// determinant
// distance
// dot
// exp
// exp2
// extractBits
// faceForward
// firstLeadingBit
// firstTrailingBit
// floor
// fma
// fract
// frexp
// insertBits
// inverseSqrt
// ldexp
// length
// log
// log2
// max
// min
// mix
// modf
// normalize
// pack2x16float
// pack2x16snorm
// pack2x16unorm
// pack4x8snorm
// pack4x8unorm
// pow
// quantizeToF16
// radians
// reflect
// refract
// reverseBits
// round
// saturate
// select
// sign
// smoothstep
// sqrt
// step
// transpose
// trunc
// unpack2x16float
// unpack2x16snorm
// unpack2x16unorm
// unpack4x8snorm
// unpack4x8unorm

#ifdef WAYLIB_NAMESPACE_NAME
}
#endif