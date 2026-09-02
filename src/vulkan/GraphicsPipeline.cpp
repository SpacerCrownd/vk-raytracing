#include "GraphicsPipeline.h"

namespace ptvk {
GraphicsPipeline::GraphicsPipeline(
        const vk::raii::Device &device,
        GLFWwindow *window,
        const Shader& shader,
        uint32_t numImages,
        vk::Format colorFormat,
        vk::Format depthFormat,
        bool enableDepthTesting
    ) : m_device(device), m_numImages(numImages)
{
    vk::PipelineShaderStageCreateInfo shaderStages[] {
        shader.createShaderStage(vk::ShaderStageFlagBits::eVertex, "vertMain"),
        shader.createShaderStage(vk::ShaderStageFlagBits::eFragment, "fragMain")
    };

    vk::PipelineVertexInputStateCreateInfo   vertexInputInfo;
    vk::PipelineInputAssemblyStateCreateInfo inputAssembly { .topology = vk::PrimitiveTopology::eTriangleList};

    vk::PipelineRasterizationStateCreateInfo rasterizer {
        .depthClampEnable        = vk::False,
        .rasterizerDiscardEnable = vk::False,
        .polygonMode             = vk::PolygonMode::eFill,
        .cullMode                = vk::CullModeFlagBits::eBack,
        .frontFace               = vk::FrontFace::eClockwise,
        .depthBiasEnable         = vk::False,
        .lineWidth               = 1.0f
    };

    vk::PipelineMultisampleStateCreateInfo multisampling {
        .rasterizationSamples = vk::SampleCountFlagBits::e1,
        .sampleShadingEnable = vk::False
    };

    vk::PipelineColorBlendAttachmentState colorBlendAttachment {
        .blendEnable    = vk::False,
        .colorWriteMask = vk::ColorComponentFlagBits::eR | vk::ColorComponentFlagBits::eG | vk::ColorComponentFlagBits::eB | vk::ColorComponentFlagBits::eA};

    vk::PipelineColorBlendStateCreateInfo colorBlending {
        .logicOpEnable = vk::False,
        .logicOp = vk::LogicOp::eCopy,
        .attachmentCount = 1,
        .pAttachments = &colorBlendAttachment
    };

    std::vector<vk::DynamicState> dynamicStates = {vk::DynamicState::eViewport, vk::DynamicState::eScissor};
    vk::PipelineDynamicStateCreateInfo dynamicState{ .dynamicStateCount = static_cast<uint32_t>(dynamicStates.size()), .pDynamicStates = dynamicStates.data() };
    vk::PipelineViewportStateCreateInfo viewportState { .viewportCount = 1, .scissorCount = 1 };

    vk::PipelineLayoutCreateInfo pipelineLayoutInfo{.setLayoutCount = 0, .pushConstantRangeCount = 0};
    m_pipelineLayout = vk::raii::PipelineLayout(device, pipelineLayoutInfo);

    vk::PipelineDepthStencilStateCreateInfo depthStencil {
        .depthTestEnable       = enableDepthTesting,
        .depthWriteEnable      = enableDepthTesting,
        .depthCompareOp        = vk::CompareOp::eLess,
        .depthBoundsTestEnable = vk::False,
        .stencilTestEnable     = vk::False
    };

    vk::StructureChain<vk::GraphicsPipelineCreateInfo, vk::PipelineRenderingCreateInfo> pipelineCreateInfoChain = {
            vk::GraphicsPipelineCreateInfo{
                .stageCount = 2,
                .pStages = shaderStages,
                .pVertexInputState = &vertexInputInfo,
                .pInputAssemblyState = &inputAssembly,
                .pViewportState = &viewportState,
                .pRasterizationState = &rasterizer,
                .pMultisampleState = &multisampling,
                .pDepthStencilState = &depthStencil,
                .pColorBlendState = &colorBlending,
                .pDynamicState = &dynamicState,
                .layout = m_pipelineLayout,
                .renderPass = nullptr,
            },
            vk::PipelineRenderingCreateInfo{
                .colorAttachmentCount = 1,
                .pColorAttachmentFormats = &colorFormat,
                .depthAttachmentFormat = depthFormat,
                .stencilAttachmentFormat = vk::Format::eUndefined
            }
    };

    m_pipeline = vk::raii::Pipeline(device, nullptr, pipelineCreateInfoChain.get<vk::GraphicsPipelineCreateInfo>());
}

void GraphicsPipeline::bind(const vk::raii::CommandBuffer &cmdBuffer) {
    cmdBuffer.bindPipeline(vk::PipelineBindPoint::eGraphics, m_pipeline);
}
}
