#ifndef GRAPHICS_PIPELINE_H
#define GRAPHICS_PIPELINE_H

#include "Vulkan.h"
#include "Shader.h"

namespace ptvk {
class GraphicsPipeline {
public:
    GraphicsPipeline(const vk::raii::Device& device,
                    const Shader& shader,
                    uint32_t numImages,
                    vk::Format colorFormat,
                    vk::Format depthFormat,
                    bool enableDepthTesting);

    vk::PipelineLayout getLayout() { return m_pipelineLayout; }

    void bind(const vk::raii::CommandBuffer& cmdBuffer);
private:
    const vk::raii::Device&  m_device;
    vk::raii::Pipeline       m_pipeline{VK_NULL_HANDLE};
    vk::raii::PipelineLayout m_pipelineLayout{VK_NULL_HANDLE};

    int m_numImages;
};
}

#endif //GRAPHICS_PIPELINE_H
