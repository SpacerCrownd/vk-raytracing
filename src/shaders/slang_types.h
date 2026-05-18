#ifndef SLANG_TYPES_H
#define SLANG_TYPES_H

#ifdef __cplusplus
#include <glm/glm.hpp>

namespace shaderio {
using namespace glm;

using float4x4 = glm::mat4;
using float4x3 = glm::mat4x3;
using float3x4 = glm::mat3x4;
using float3x3 = glm::mat3;
using float2x2 = glm::mat2;
using float2x3 = glm::mat2x3;
using float3x2 = glm::mat3x2;

using float2 = glm::vec2;
using float4 = glm::vec4;
using float3 = glm::vec3;

using int2 = glm::ivec2;
using int3 = glm::ivec3;
using int4 = glm::ivec4;

using uint  = unsigned int;
using uint2 = glm::uvec2;
using uint3 = glm::uvec3;
using uint4 = glm::uvec4;

using bool2 = glm::bvec2;
using bool3 = glm::bvec3;
using bool4 = glm::bvec4;

}
#endif // __cplusplus
#endif //SLANG_TYPES_H