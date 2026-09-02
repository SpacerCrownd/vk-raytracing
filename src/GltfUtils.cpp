#include "GltfUtils.h"

#define GLM_ENABLE_EXPERIMENTAL
#include <glm/gtx/transform.hpp>
#include <glm/gtx/quaternion.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <glm/gtx/matrix_decompose.hpp>
#include <iostream>


namespace app::gltfutils {
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
    return ret;
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

vk::SamplerAddressMode extractWrapMode(int wrapMode) {
    switch (wrapMode) {
        case TINYGLTF_TEXTURE_WRAP_REPEAT:
            return vk::SamplerAddressMode::eRepeat;

        case TINYGLTF_TEXTURE_WRAP_CLAMP_TO_EDGE:
            return vk::SamplerAddressMode::eClampToEdge;

        case TINYGLTF_TEXTURE_WRAP_MIRRORED_REPEAT:
            return vk::SamplerAddressMode::eMirroredRepeat;

        default:
            return vk::SamplerAddressMode::eRepeat;
    }
}

std::string generatePrimitiveKey(const tinygltf::Primitive& primitive)
{
    std::stringstream string;
    for(const auto& kv : primitive.attributes) {
        string << kv.first << ":" << kv.second << " ";
    }
    string << "indices:" << primitive.indices;
    return string.str();
}

void getNodeTRS(const tinygltf::Node& node, glm::vec3& translation, glm::quat& rotation, glm::vec3& scale) {
    // Initialize translation, rotation, and scale to default values
    translation = glm::vec3(0.0f, 0.0f, 0.0f);
    rotation = glm::quat(1.0f, 0.0f, 0.0f, 0.0f);
    scale = glm::vec3(1.0f, 1.0f, 1.0f);

    // Check if the node has a matrix defined
    if(node.matrix.size() == 16) {
        glm::mat4 matrix = glm::make_mat4(node.matrix.data());
        glm::vec3 skew;
        glm::vec4 perspective;
        glm::decompose(matrix, scale, rotation, translation, skew, perspective);
        return;
    }

    // Retrieve translation if available
    if(node.translation.size() == 3) {
        translation = glm::make_vec3(node.translation.data());
    }

    // Retrieve rotation if available
    if(node.rotation.size() == 4) {
        rotation.x = static_cast<float>(node.rotation[0]);
        rotation.y = static_cast<float>(node.rotation[1]);
        rotation.z = static_cast<float>(node.rotation[2]);
        rotation.w = static_cast<float>(node.rotation[3]);
    }

    // Retrieve scale if available
    if(node.scale.size() == 3) {
        scale = glm::make_vec3(node.scale.data());
    }
}

glm::mat4 getNodeTransformMatrix(const tinygltf::Node &node) {
    if(node.matrix.size() == 16) {
        return glm::make_mat4(node.matrix.data());
    }

    glm::vec3 translation;
    glm::quat rotation;
    glm::vec3 scale;
    getNodeTRS(node, translation, rotation, scale);

    return glm::translate(glm::mat4(1.0f), translation) * glm::mat4_cast(rotation) * glm::scale(glm::mat4(1.0f), scale);
}

size_t getVertexCount(const tinygltf::Model &model, const tinygltf::Primitive &primitive) {
    const tinygltf::Accessor& vertexAccessor = model.accessors.at(primitive.attributes.at("POSITION"));
    return vertexAccessor.count;
}

size_t getIndexCount(const tinygltf::Model &model, const tinygltf::Primitive &primitive) {
    if(primitive.indices > -1)
    {
        const tinygltf::Accessor& indexAccessor = model.accessors[primitive.indices];
        return indexAccessor.count;
    }
    return getVertexCount(model, primitive);
}

}
