#ifndef VK_RAYTRACING_GLTFUTILS_H
#define VK_RAYTRACING_GLTFUTILS_H

#include <filesystem>
#include <tinygltf/tiny_gltf.h>
#include <vulkan/vulkan.hpp>

namespace app {
bool loadGltf(const std::filesystem::path& filename, tinygltf::Model& model);

vk::Filter            extractFilter(int filter);
vk::SamplerMipmapMode extractMipmapMode(int filter);
}



#endif //VK_RAYTRACING_GLTFUTILS_H