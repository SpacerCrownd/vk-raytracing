#include "Renderer.h"
#include "vulkan/Utils.h"
#include "GltfUtils.h"
#include "vulkan/Shader.h"

namespace app {
Renderer::Renderer(int width, int height, const char* pAppName) : width(width), height(height),
                                                                  m_window(width, height, pAppName),
                                                                  m_vkCore(pAppName, m_window),
                                                                  m_camera(glm::vec3(0.0f, 0.0f, 0.0f))
{
    m_window.addOnKeyChanged([this](int key, int scancode, int action, int mods){
        m_camera.onKeyChanged(key, scancode, action, mods);
    });

    m_window.addOnMouseButtonChanged([this](int button, int action, int mods) {
        m_camera.onMouseButtonChanged(this->m_window.getWindow(), button, action, mods);
    });

    m_window.addOnCursorPositionChanged([this](double x, double y) {
        m_camera.onCursorPositionChanged(x, y);
    });

    m_window.addOnFramebufferSizeChanged([this](int width, int height) {
        m_vkCore.framebufferResized = true;
        OnResize();
    });

    m_pSamplerPool = std::make_unique<ptvk::SamplerPool>(m_vkCore.getDevice().getVkDevice());
    m_pVkScene = std::make_unique<ptvk::GltfSceneVulkan>(m_vkCore.getResourceAllocator(), m_pSamplerPool, false);
}

Renderer::~Renderer() {
    m_vkCore.deviceWaitIdle();
}

void Renderer::Run() {
    CreateScene();
    LoadShaders();
    CreateGraphicsPipeline();
    MainLoop();
}

void Renderer::MainLoop() {
    while (!glfwWindowShouldClose(m_window.getWindow())) {
        PrepareFrameData();
        Draw();
        glfwPollEvents();
    }
}

void Renderer::CreateScene() {
    m_scene.load("assets/sponza/NewSponza_Main_glTF_003.gltf");
    auto cmd = m_vkCore.beginSingleTimeCommandBuffer();

    //m_scene.UploadToGpu(model, m_vkCore);
}

void Renderer::LoadShaders() {
    m_pRasterShader = std::make_unique<ptvk::Shader>(m_vkCore.getDevice().getVkDevice(), "testRaster.spv");
    // m_rtShader.emplace(m_vkCore.GetDevice().GetVkDevice(), "raytracing.spv");
    printf("[INFO] Shaders Loaded\n");
}

void Renderer::CreateGraphicsPipeline() {
    auto colorFormat = m_vkCore.getDrawImage().format;
    auto depthFormat = m_vkCore.getDepthFormat();
    m_pGraphicsPipeline = std::make_unique<ptvk::GraphicsPipeline>(m_vkCore.getDevice().getVkDevice(), m_window.getWindow(), *m_pRasterShader.get(), 1,
                               colorFormat, depthFormat, m_enableDepth);
}

void Renderer::PrepareFrameData() {

}

void Renderer::Draw() {
    m_vkCore.prepareFrame();

    auto& cmdBuffer = m_vkCore.beginCommandRecording();

    vk::ImageSubresourceRange subresourceRange = {
        .aspectMask = vk::ImageAspectFlagBits::eColor,
        .baseMipLevel = 0,
        .levelCount = 1,
        .baseArrayLayer = 0,
        .layerCount = 1,
    };

    // get swapchain image
    uint32_t imgIndex = m_vkCore.getCurrentImageIndex();
    const auto& swapchainImage = m_vkCore.getSwapchain().GetSwapchainImage(static_cast<int>(imgIndex));
    auto swapchainExtent = m_vkCore.getSwapchain().GetExtent();
    // get draw image
    auto& drawImage = m_vkCore.getDrawImage();

    if (m_currentPipeline == eRaster) {
        // prepare to start dynamic rendering
        //cmdBuffer.clearColorImage(swapchainImage, vk::ImageLayout::eTransferDstOptimal, clearColor, imageRange);
        vk::ClearValue clearColor = vk::ClearColorValue(1.0f, 1.0f, 1.0f, 1.0f);
        vk::ClearValue depthValue = vk::ClearDepthStencilValue(1.0f, 0);
        vk::RenderingAttachmentInfo colorAttachmentInfo = {
            .imageView = drawImage.view,
            .imageLayout = vk::ImageLayout::eGeneral,
            .loadOp = vk::AttachmentLoadOp::eClear,
            .storeOp = vk::AttachmentStoreOp::eStore,
            .clearValue = clearColor,
        };
        vk::RenderingAttachmentInfo depthAttachmentInfo = {
            .imageView = m_vkCore.getDepthImage().view,
            .imageLayout = vk::ImageLayout::eDepthAttachmentOptimal,
            .loadOp = vk::AttachmentLoadOp::eClear,
            .storeOp = vk::AttachmentStoreOp::eDontCare,
            .clearValue = depthValue
        };
        vk::RenderingInfo renderingInfo = {
            .renderArea = {
                .offset = {.x = 0,.y = 0},
                .extent = {
                    .width = drawImage.extent.width,
                    .height = drawImage.extent.height
                }
            },
            .layerCount = 1,
            .colorAttachmentCount = 1,
            .pColorAttachments = &colorAttachmentInfo,
            .pDepthAttachment = &depthAttachmentInfo
        };

        // begin dynamic rendering
        cmdBuffer.beginRendering(renderingInfo);

        // bind pipeline
        m_pGraphicsPipeline->bind(cmdBuffer);

        cmdBuffer.setViewport(0, vk::Viewport(0.0f, 0.0f, static_cast<float>(swapchainExtent.width), static_cast<float>(swapchainExtent.height), 0.0f, 1.0f));
        cmdBuffer.setScissor(0, vk::Rect2D(vk::Offset2D(0, 0), swapchainExtent));

        // start drawing
        cmdBuffer.draw(6, 1, 0, 0);

        cmdBuffer.endRendering();
    } else if (m_currentPipeline == eRaytracing) {

    }

    // --
    // Copy draw image into swapchain image
    // --
    // transition swapchain image to transfer dst
    ptvk::utils::imageLayoutTransition(cmdBuffer,
                                 swapchainImage,
                                 vk::PipelineStageFlagBits2::eColorAttachmentOutput,
                                 vk::PipelineStageFlagBits2::eTransfer,
                                 {},
                                 vk::AccessFlagBits2::eTransferWrite,
                                 vk::ImageLayout::eUndefined,
                                 vk::ImageLayout::eTransferDstOptimal,
                                 subresourceRange);

    // transition draw image to transfer src
    ptvk::utils::imageLayoutTransition(cmdBuffer,
                                 drawImage.image,
                                 vk::PipelineStageFlagBits2::eRayTracingShaderKHR | vk::PipelineStageFlagBits2::eColorAttachmentOutput,
                                 vk::PipelineStageFlagBits2::eTransfer,
                                 vk::AccessFlagBits2::eShaderWrite | vk::AccessFlagBits2::eColorAttachmentWrite,
                                 vk::AccessFlagBits2::eTransferRead,
                                 vk::ImageLayout::eGeneral,
                                 vk::ImageLayout::eTransferSrcOptimal,
                                 subresourceRange);

    // copy draw image to swapchain for presentation
    vk::Extent2D drawExtent = {drawImage.extent.width, drawImage.extent.height};
    ptvk::utils::blitImage(cmdBuffer, drawImage.image, swapchainImage, drawExtent, swapchainExtent);

    // transition swapchain image for presentation
    ptvk::utils::imageLayoutTransition(
        cmdBuffer,
        swapchainImage,
        vk::ImageLayout::eTransferDstOptimal,
        vk::ImageLayout::ePresentSrcKHR);

    // transition draw image back to general layout for raytracing/rasterization in next draw call
    ptvk::utils::imageLayoutTransition(
        cmdBuffer,
        drawImage.image,
        vk::PipelineStageFlagBits2::eTransfer,
        vk::PipelineStageFlagBits2::eAllCommands,
        vk::AccessFlagBits2::eTransferRead, // last access was for copy transfer src
        {},
        vk::ImageLayout::eTransferSrcOptimal,
        vk::ImageLayout::eGeneral,
        subresourceRange);

    m_vkCore.submitFrame();
    m_vkCore.presentFrame();
}

void Renderer::OnResize() {
}
}
