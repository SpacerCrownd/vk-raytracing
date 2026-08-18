#ifndef VK_RAYTRACING_GLTFSCENEVULKAN_H
#define VK_RAYTRACING_GLTFSCENEVULKAN_H

#include "Core.h"
#include "../GltfScene.h"

namespace ptvk {
class GltfSceneVulkan {
public:
    struct VertexBuffers {
        Buffer bPosition;
        Buffer bNormal;
        Buffer bTangent;
        Buffer bTexCoord0;
        Buffer bTexCoord1;
        Buffer bColor;
    };

private:
    Buffer          m_bSceneInfo{};
    Buffer          m_bMaterials{};
    Buffer          m_bTextures{};
    VertexBuffers   m_vertexBuffers{};
    Buffer          m_bIndices{};
    Buffer          m_bLights{};
    Buffer          m_bRenderNodes{};
    Buffer          m_bRenderPrimitives{};

    std::vector<vk::raii::Sampler> m_samplers{};
};
}

#endif //VK_RAYTRACING_GLTFSCENEVULKAN_H
