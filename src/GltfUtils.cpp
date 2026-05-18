#include "GltfUtils.h"

#define TINYGLTF_IMPLEMENTATION
#define STB_IMAGE_IMPLEMENTATION
#define STB_IMAGE_WRITE_IMPLEMENTATION
#include <tinygltf/tiny_gltf.h>

#include <filesystem>
#include <iostream>

namespace app {
tinygltf::Model LoadGltfResource(const std::filesystem::path &filename) {
    tinygltf::Model model;
    tinygltf::TinyGLTF loader;
    std::string err;
    std::string warn;
    if (filename.extension() == ".gltf") {
        if (!loader.LoadASCIIFromFile(&model, &err, &warn, filename.string())) {
            std::cout << "[ERROR] Error loading glTF file: " << err << std::endl;
            return {};
        }
    } else if (filename.extension() == ".glb") {
        if (!loader.LoadBinaryFromFile(&model, &err, &warn, filename.string())) {
            std::cout << "[ERROR] Error loading glTF file: " << err.c_str();
            return {};
        }
    } else {
        std::cout << "[ERROR] Unsupported file format: " << filename.extension().string().c_str() << std::endl;
        return {};
    }
    std::cout << "\n[INFO] Loaded glTF file: " << filename.string().c_str() << std::endl;
    return model;
}

vk::Filter ExtractFilter(int filter) {
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

vk::SamplerMipmapMode ExtractMipmapMode(int filter) {
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
