#include "GltfUtils.h"

#define TINYGLTF_IMPLEMENTATION
#define STB_IMAGE_IMPLEMENTATION
#define STB_IMAGE_WRITE_IMPLEMENTATION
#include <tinygltf/tiny_gltf.h>

#include <filesystem>
#include <iostream>

namespace app {
bool loadGltf(const std::filesystem::path &filename, tinygltf::Model &model) {
    tinygltf::TinyGLTF loader;
    std::string err;
    std::string warn;
    const std::string extension = filename.extension().string();

    // Check file extension and load file
    bool ret{};
    if (extension == ".glb")
        ret = loader.LoadBinaryFromFile(&model, &err, &warn, filename.string());
    else if (extension == ".gltf")
        ret = loader.LoadASCIIFromFile(&model, &err, &warn, filename.string());
    else
        throw std::runtime_error("[ERROR] Tried to load unsupported file format.\n Currently supported file formats: .glb, .gltf.");

    if (!warn.empty())
        std::cout << ("[WARNING] Warn: " + warn) << std::endl;
    if (!err.empty())
        throw std::runtime_error("[ERROR] " + err);
    if (!ret)
        throw std::runtime_error("[ERROR] Failed to parse glTF file: " + filename.string());

    std::cout << ("[GltfLoader] Parsed " + filename.string()) << std::endl;
}

vk::Filter extractFilter(int filter) {
    switch (filter) {
        case TINYGLTF_TEXTURE_FILTER_LINEAR:
        case TINYGLTF_TEXTURE_FILTER_LINEAR_MIPMAP_LINEAR:
        case TINYGLTF_TEXTURE_FILTER_LINEAR_MIPMAP_NEAREST:
            return vk::Filter::eLinear;

        case TINYGLTF_TEXTURE_FILTER_NEAREST:
        case TINYGLTF_TEXTURE_FILTER_NEAREST_MIPMAP_LINEAR:
        case TINYGLTF_TEXTURE_FILTER_NEAREST_MIPMAP_NEAREST:
            return vk::Filter::eNearest;

        default:
            return vk::Filter::eLinear;
    }
}

vk::SamplerMipmapMode extractMipmapMode(int filter) {
    switch (filter) {
        case TINYGLTF_TEXTURE_FILTER_LINEAR_MIPMAP_LINEAR:
        case TINYGLTF_TEXTURE_FILTER_NEAREST_MIPMAP_LINEAR:
            return vk::SamplerMipmapMode::eLinear;

        case TINYGLTF_TEXTURE_FILTER_LINEAR_MIPMAP_NEAREST:
        case TINYGLTF_TEXTURE_FILTER_NEAREST_MIPMAP_NEAREST:
            return vk::SamplerMipmapMode::eNearest;

        default:
            return vk::SamplerMipmapMode::eLinear;
    }
}
}
