#ifndef GRAPHICS_PIPELINE_H
#define GRAPHICS_PIPELINE_H

#include "Vulkan.h"

namespace ptvk {
class GraphicsPipeline {
public:
    GraphicsPipeline(vk::raii::Device& device, GLFWwindow* window, vk::raii::RenderPass& renderPass, vk::raii::ShaderModule& shader);

    void Bind(vk::raii::CommandBuffer& cmdBuffer);

private:
    vk::raii::Device& m_device;
    vk::raii::Pipeline graphicsPipeline{VK_NULL_HANDLE};
    vk::raii::PipelineLayout pipelineLayout{VK_NULL_HANDLE};
};
}

#endif //GRAPHICS_PIPELINE_H
