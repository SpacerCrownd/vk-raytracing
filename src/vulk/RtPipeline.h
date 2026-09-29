#ifndef VK_RAYTRACING_RTPIPELINE_H
#define VK_RAYTRACING_RTPIPELINE_H

#include "Vulkan.h"
#include "Shader.h"
#include "Resources.h"
#include "ResourceAllocator.h"

namespace ptvk {
class RtPipeline {
public:
    vk::StridedDeviceAddressRegionKHR raygenRegion{};
    vk::StridedDeviceAddressRegionKHR missRegion{};
    vk::StridedDeviceAddressRegionKHR hitRegion{};
    vk::StridedDeviceAddressRegionKHR callableRegion{};

    RtPipeline(const Device &device,
                       const ResourceAllocator &allocator,
                       const Shader &shader,
                       std::vector<vk::DescriptorSetLayout> descLayouts);

    void bind(const vk::raii::CommandBuffer& cmdBuffer);

    vk::PipelineLayout getLayout() { return m_pipelineLayout; }

private:
    const Device&            m_device;
    const ResourceAllocator& m_allocator;
    vk::raii::Pipeline       m_pipeline{VK_NULL_HANDLE};
    vk::raii::PipelineLayout m_pipelineLayout{VK_NULL_HANDLE};

    Buffer m_sbtBuffer;


    void createShaderBindingTable(vk::RayTracingPipelineCreateInfoKHR& createInfo);
};
}


#endif //VK_RAYTRACING_RTPIPELINE_H
