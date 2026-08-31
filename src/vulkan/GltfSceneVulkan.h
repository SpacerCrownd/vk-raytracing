#ifndef VK_RAYTRACING_GLTFSCENEVULKAN_H
#define VK_RAYTRACING_GLTFSCENEVULKAN_H

#include "Core.h"
#include "../GltfScene.h"
#include "SamplerPool.h"

namespace ptvk {
class GltfSceneVulkan {
public:
    GltfSceneVulkan(const ResourceAllocator &allocator,
                    const SamplerPool &samplerPool,
                    bool generateMipmaps);

    struct VertexBuffers {
        Buffer bPosition;
        Buffer bNormal;
        Buffer bTangent;
        Buffer bTexCoord0;
        Buffer bTexCoord1;
        Buffer bColor;
    };

    void createVkResources(vk::raii::CommandBuffer &cmd, app::GltfScene &scene);
    void updateFromScene(vk::raii::CommandBuffer &cmd, app::GltfScene &scene);
    void destroy();

private:
    const ResourceAllocator               &m_allocator;
    const SamplerPool                     &m_samplerPool;

    Buffer m_bMaterials{};
    Buffer m_bTextureInfos{}; // 1 to 1 correspondence with gltf textures for material indexing
    Buffer m_bLights{};

    Buffer m_bRenderNodes{};
    Buffer m_bRenderPrimitives{};
    Buffer m_bSceneInfo{};

    std::vector<VertexBuffers> m_vertexBuffers{};
    std::vector<Buffer>        m_bIndices{};
    std::vector<Image>         m_images{};

    bool m_generateMipmaps{false}; // TODO

    void uploadTextureImages(vk::raii::CommandBuffer &cmd, app::GltfScene &scene);
    void createTextures(vk::raii::CommandBuffer &cmd, app::GltfScene &scene);
    void createVertexIndexBuffers();
};
}

#endif //VK_RAYTRACING_GLTFSCENEVULKAN_H
