#ifndef VK_RAYTRACING_RTPIPELINE_H
#define VK_RAYTRACING_RTPIPELINE_H

#include "Vulkan.h"

namespace ptvk {
class RtPipeline {
public:
    RtPipeline(const vk::raii::Device& device,
               GLFWwindow* window);
    void bind(const vk::raii::CommandBuffer& cmdBuffer);

    vk::PipelineLayout getLayout() { return m_pipelineLayout; }

private:
    const vk::raii::Device&  m_device;
    vk::raii::Pipeline       m_pipeline{VK_NULL_HANDLE};
    vk::raii::PipelineLayout m_pipelineLayout{VK_NULL_HANDLE};
};
}


#endif //VK_RAYTRACING_RTPIPELINE_H
