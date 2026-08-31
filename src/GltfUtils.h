#ifndef VK_RAYTRACING_GLTFUTILS_H
#define VK_RAYTRACING_GLTFUTILS_H

#include <filesystem>
#include <vulkan/vulkan.hpp>
#include <glm/glm.hpp>
#include <tinygltf/tiny_gltf.h>

namespace app::gltfutils {
bool                  loadGltf(const std::filesystem::path& filename, tinygltf::Model& model);
vk::Filter            extractFilter(int filter);
vk::SamplerMipmapMode extractMipmapMode(int filter);

std::string generatePrimitiveKey(const tinygltf::Primitive& primitive);
void        getNodeTRS(const tinygltf::Node& node, glm::vec3& translation, glm::quat& rotation, glm::vec3& scale);
glm::mat4   getNodeTransformMatrix(const tinygltf::Node& node);
size_t      getVertexCount(const tinygltf::Model& model, const tinygltf::Primitive& primitive);
size_t      getIndexCount(const tinygltf::Model& model, const tinygltf::Primitive& primitive);
}

#endif //VK_RAYTRACING_GLTFUTILS_H
