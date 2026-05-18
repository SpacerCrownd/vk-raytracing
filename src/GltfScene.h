#ifndef VK_RAYTRACING_SCENE_H
#define VK_RAYTRACING_SCENE_H

#include <tinygltf/tiny_gltf.h>
#include <vector>

#include "shaders/io_gltf.h"
#include "vulkan/Resources.h"
#include "vulkan/Core.h"

// scene data
namespace app {
struct GltfScene {
    std::vector<shaderio::GltfMesh> meshes;
    std::vector<shaderio::GltfInstance> instances;
    shaderio::GltfSceneInfo sceneInfo;

    std::vector<ptvk::Image> images;
    std::vector<vk::raii::Sampler> samplers;

    std::vector<ptvk::Buffer> bGltfDatas; // all raw geometry data buffers
    ptvk::Buffer bMeshes; // buffer containing all buffer views on geometry data buffers
    ptvk::Buffer bInstances; // buffer containing all object instance infos
    ptvk::Buffer bMaterials;
    ptvk::Buffer bSceneInfo;

    std::vector<uint32_t> meshToBufferIndex;

    void ImportGltfData(const tinygltf::Model& model, const ptvk::Core& core) const;

    //TODO
    void ImportGltfScene(const tinygltf::Model& model,  const ptvk::Core& core) const;
};
}

#endif //VK_RAYTRACING_SCENE_H