#include "GltfSceneRt.h"

namespace ptvk {
GltfSceneRt::GltfSceneRt(const ResourceAllocator& allocator, const vk::raii::Device &device) : m_allocator(allocator), m_device(device) {}

void GltfSceneRt::create(vk::raii::CommandBuffer &cmd,
                        app::GltfScene &scene,
                        GltfSceneVulkan &sceneVk,
                        vk::BuildAccelerationStructureFlagsKHR flags)
{

}
}
