#ifndef VK_RAYTRACING_GLTFSCENEVULKAN_H
#define VK_RAYTRACING_GLTFSCENEVULKAN_H

#include "SamplerPool.h"
#include "StagingUploader.h"
#include "../GltfScene.h"

namespace ptvk {
class GltfSceneVulkan {
public:
    GltfSceneVulkan(const ResourceAllocator &allocator,
                    StagingUploader &staging,
                    SamplerPool &samplerPool,
                    bool generateMipmaps);
    ~GltfSceneVulkan();

    struct VertexBuffers {
        Buffer bPosition;
        Buffer bNormal;
        Buffer bTangent;
        Buffer bTexCoord0;
        Buffer bTexCoord1;
        Buffer bColor;
    };

    void createVkResources(const vk::raii::CommandBuffer &cmd, const app::GltfScene &scene);
    void updateFromScene(const vk::raii::CommandBuffer &cmd, const app::GltfScene &scene);
    void destroy();

private:
    const ResourceAllocator &m_allocator;
    StagingUploader         &m_staging;
    SamplerPool             &m_samplerPool;

    std::vector<vk::Sampler> m_samplers;

    Buffer m_bMaterials{};
    Buffer m_bTextureInfos{}; // 1 to 1 correspondence with gltf textures for material and img id
    Buffer m_bLights{};
    Buffer m_bRenderNodes{};
    Buffer m_bRenderPrimitives{};
    Buffer m_bSceneInfo{};

    std::vector<VertexBuffers> m_vertexBuffers{};
    std::vector<Buffer>        m_bIndices{};
    std::vector<Image>         m_images{};

    bool m_generateMipmaps{false}; // TODO

    void uploadTextureImages(const vk::raii::CommandBuffer &cmd, const app::GltfScene &scene);
    void createSamplers(const tinygltf::Model &model);
    void createTextures(const vk::raii::CommandBuffer &cmd, const app::GltfScene &scene);
    void createDefaultImage(int id, const std::array<uint8_t, 4>& color); // create 1x1 dummy image
    void createVertexIndexBuffers();

    void copyImage(const vk::raii::CommandBuffer& cmd, const void* data, vk::DeviceSize size, Image& image, vk::ImageLayout& newLayout);
};
}

#endif //VK_RAYTRACING_GLTFSCENEVULKAN_H
