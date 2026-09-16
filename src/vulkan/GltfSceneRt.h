#ifndef VK_RAYTRACING_GLTFSCENERT_H
#define VK_RAYTRACING_GLTFSCENERT_H

#include "ResourceAllocator.h"
#include "GltfSceneVulkan.h"
#include "../GltfScene.h"

namespace ptvk {
class GltfSceneRt {
public:
    GltfSceneRt(const ResourceAllocator& allocator, const vk::raii::Device& device);

    vk::raii::AccelerationStructureKHR& getTlas() { return m_tlas.accel; }

    void create(vk::raii::CommandBuffer& cmd, app::GltfScene& scene, GltfSceneVulkan& sceneVk, vk::BuildAccelerationStructureFlagsKHR flags);
private:
    const ResourceAllocator& m_allocator;
    const vk::raii::Device&  m_device;

    std::vector<AccelerationStructure>  m_blas{};
    AccelerationStructure               m_tlas{};
    std::vector<vk::AccelerationStructureInstanceKHR>  m_tlasInstances{};

    Buffer m_blasScratchBuffer;
    Buffer m_tlasScratchBuffer;
    Buffer m_instancesScratchBuffer;
};
}


#endif //VK_RAYTRACING_GLTFSCENERT_H
