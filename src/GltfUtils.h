#ifndef VK_RAYTRACING_GLTFUTILS_H
#define VK_RAYTRACING_GLTFUTILS_H

#include <filesystem>
#include <tinygltf/tiny_gltf.h>
#include <vulkan/vulkan.hpp>

namespace app {
    tinygltf::Model LoadGltfResource(const std::filesystem::path &filename);
    vk::Filter ExtractFilter(int filter);
    vk::SamplerMipmapMode ExtractMipmapMode(int filter);
}



#endif //VK_RAYTRACING_GLTFUTILS_H