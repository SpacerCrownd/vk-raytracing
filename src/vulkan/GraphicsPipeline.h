#ifndef GRAPHICS_PIPELINE_H
#define GRAPHICS_PIPELINE_H

#include "Vulkan.h"
#include "Shader.h"

namespace ptvk {
class GraphicsPipeline {
public:
    GraphicsPipeline(
        vk::raii::Device& device,
        GLFWwindow* window,
        Shader& shader,
        uint32_t numImages,
        vk::Format colorFormat,
        vk::Format depthFormat
        );

    void Bind(vk::raii::CommandBuffer& cmdBuffer);

private:
    vk::raii::Device& m_device;
    vk::raii::Pipeline m_pipeline{VK_NULL_HANDLE};
    vk::raii::PipelineLayout m_pipelineLayout{VK_NULL_HANDLE};

    int m_numImages;
};
}

#endif //GRAPHICS_PIPELINE_H
