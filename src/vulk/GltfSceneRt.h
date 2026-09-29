#ifndef VK_RAYTRACING_GLTFSCENERT_H
#define VK_RAYTRACING_GLTFSCENERT_H

#include "../GltfScene.h"
#include "AccelerationStructure.h"
#include "ResourceAllocator.h"
#include "GltfSceneVk.h"
#include "Core.h"

namespace ptvk {
class GltfSceneRt {
public:
    GltfSceneRt(const ResourceAllocator& allocator, const Device& device);

    vk::raii::AccelerationStructureKHR& getTlas() { return m_tlas.accel; }

    void createBLAS(Core &vkCore,
                    const app::GltfScene &scene,
                    const GltfSceneVk &sceneVk,
                    vk::BuildAccelerationStructureFlagsKHR flags);

    void createTLAS(const vk::raii::CommandBuffer &cmd,
                    const app::GltfScene &scene,
                    const GltfSceneVk &sceneVk,
                    vk::BuildAccelerationStructureFlagsKHR flags);

    // update tlas instances from dirty flags
    bool syncTLAS(const vk::raii::CommandBuffer &cmd, app::GltfScene &scene);

private:
    const ResourceAllocator& m_allocator;
    const Device&            m_device;

    std::vector<AccelerationStructure>          m_blas{};
    std::vector<AccelerationStructureBuildData> m_blasBuildData{};
    Buffer                                      m_blasScratchBuffer{};

    AccelerationStructure          m_tlas{};
    AccelerationStructureBuildData m_tlasBuildData{};
    std::vector<vk::AccelerationStructureInstanceKHR> m_tlasInstances{};
    Buffer                         m_tlasScratchBuffer{};
    Buffer                         m_tlasInstancesBuffer{}; // need this to update tlas later on

};

AccelerationStructureGeometryInfo primitiveToGeometry(const GltfSceneVk &sceneVk, const app::RenderPrimitive &submesh, uint32_t primID);
}


#endif //VK_RAYTRACING_GLTFSCENERT_H
