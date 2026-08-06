#include "Renderer.h"
#include "vulkan/Utils.h"
#include "GltfUtils.h"
#include "vulkan/Shader.h"

namespace app {
Renderer::Renderer(int width, int height, const char* pAppName) : width(width), height(height),
                                                                  m_window(width, height, pAppName),
                                                                  m_vkCore(pAppName, m_window),
                                                                  m_scene(),
                                                                  m_camera(glm::vec3(0.0f, 0.0f, 0.0f))
{
    m_window.AddOnKeyChanged([this](int key, int scancode, int action, int mods){
        m_camera.OnKeyChanged(key, scancode, action, mods);
    });

    m_window.AddOnMouseButtonChanged([this](int button, int action, int mods) {
        m_camera.OnMouseButtonChanged(this->m_window.GetWindow(), button, action, mods);
    });

    m_window.AddOnCursorPositionChanged([this](double x, double y) {
        m_camera.OnCursorPositionChanged(x, y);
    });

    m_window.AddOnFramebufferSizeChanged([this](int width, int height) {
        m_vkCore.framebufferResized = true;
    });
}

Renderer::~Renderer() {
    m_vkCore.DeviceWaitIdle();
}

void Renderer::Run() {
    CreateScene();
    LoadShaders();
    CreateGraphicsPipeline();
    MainLoop();
}

void Renderer::MainLoop() {
    /*
    auto curTime = static_cast<float>(glfwGetTime());
    int frames = 0;
    float fpsTime = 0.0f;
    */
    while (!glfwWindowShouldClose(m_window.GetWindow())) {
        PrepareFrameData();
        Draw();
        glfwPollEvents();
    }
}

void Renderer::CreateScene() {
    tinygltf::Model model = LoadGltfResource("assets/test model/scene.gltf");
    m_scene.ImportGltfData(model, m_vkCore);
}

void Renderer::LoadShaders() {
    m_rasterShader.emplace(m_vkCore.GetDevice().GetVkDevice(), "testRaster.spv");
    // m_rtShader.emplace(m_vkCore.GetDevice().GetVkDevice(), "raytracing.spv");
    printf("[INFO] Shaders Loaded\n");
}

void Renderer::CreateGraphicsPipeline() {
    auto colorFormat = m_vkCore.GetDrawImage().format;
    auto depthFormat = m_vkCore.GetDepthFormat();
    m_graphicsPipeline.emplace(m_vkCore.GetDevice().GetVkDevice(), m_window.GetWindow(), m_rasterShader.value(), 1, colorFormat, depthFormat);
}

void Renderer::PrepareFrameData() {

}

void Renderer::Draw() {
    m_vkCore.PrepareFrame();

    auto& cmdBuffer = m_vkCore.BeginCommandRecording();

    vk::ImageSubresourceRange imageRange = {
        .aspectMask = vk::ImageAspectFlagBits::eColor,
        .baseMipLevel = 0,
        .levelCount = 1,
        .baseArrayLayer = 0,
        .layerCount = 1,
    };

    vk::ImageMemoryBarrier2 undefinedToColorAttachmentBarrier = {
        .srcStageMask = vk::PipelineStageFlagBits2::eColorAttachmentOutput,
        .srcAccessMask = vk::AccessFlagBits2::eNone,
        .dstStageMask = vk::PipelineStageFlagBits2::eColorAttachmentOutput,
        .dstAccessMask = vk::AccessFlagBits2::eColorAttachmentWrite,
        .oldLayout = vk::ImageLayout::eUndefined,
        .newLayout = vk::ImageLayout::eColorAttachmentOptimal,
        .srcQueueFamilyIndex = vk::QueueFamilyIgnored,
        .dstQueueFamilyIndex = vk::QueueFamilyIgnored,
        .subresourceRange = imageRange,
    };

    vk::ImageMemoryBarrier2 undefinedToTransferDstBarrier = {
        .srcStageMask = vk::PipelineStageFlagBits2::eTransfer,
        .srcAccessMask = vk::AccessFlagBits2::eNone,
        .dstStageMask = vk::PipelineStageFlagBits2::eTransfer,
        .dstAccessMask = vk::AccessFlagBits2::eTransferWrite,
        .oldLayout = vk::ImageLayout::eUndefined,
        .newLayout = vk::ImageLayout::eTransferDstOptimal,
        .srcQueueFamilyIndex = vk::QueueFamilyIgnored,
        .dstQueueFamilyIndex = vk::QueueFamilyIgnored,
        .subresourceRange = imageRange,
    };

    vk::ImageMemoryBarrier2 transferToPresentBarrier = {
        .srcStageMask = vk::PipelineStageFlagBits2::eTransfer,
        .srcAccessMask = vk::AccessFlagBits2::eTransferWrite,
        .dstStageMask = vk::PipelineStageFlagBits2::eAllCommands,
        .dstAccessMask = vk::AccessFlagBits2::eNone,
        .oldLayout = vk::ImageLayout::eTransferDstOptimal,
        .newLayout = vk::ImageLayout::ePresentSrcKHR,
        .srcQueueFamilyIndex = vk::QueueFamilyIgnored,
        .dstQueueFamilyIndex = vk::QueueFamilyIgnored,
        .subresourceRange = imageRange,
    };

    vk::ImageMemoryBarrier2 colorToTransferSrcBarrier = {
        .srcStageMask = vk::PipelineStageFlagBits2::eColorAttachmentOutput,
        .srcAccessMask = vk::AccessFlagBits2::eColorAttachmentWrite,
        .dstStageMask = vk::PipelineStageFlagBits2::eTransfer,
        .dstAccessMask = vk::AccessFlagBits2::eTransferRead,
        .oldLayout = vk::ImageLayout::eColorAttachmentOptimal,
        .newLayout = vk::ImageLayout::eTransferSrcOptimal,
        .srcQueueFamilyIndex = vk::QueueFamilyIgnored,
        .dstQueueFamilyIndex = vk::QueueFamilyIgnored,
        .subresourceRange = imageRange,
    };

    // get swapchain image
    uint32_t imgIndex = m_vkCore.GetCurrentImageIndex();
    const auto& swapchainImage = m_vkCore.GetSwapchain().GetSwapchainImage(static_cast<int>(imgIndex));
    auto swapchainExtent = m_vkCore.GetSwapchain().GetExtent();

    // swapchain layout transitions
    undefinedToTransferDstBarrier.image = swapchainImage;
    transferToPresentBarrier.image = swapchainImage;

    // get draw image
    auto& drawImage = m_vkCore.GetDrawImage();

    // draw layout transitions
    undefinedToColorAttachmentBarrier.image = drawImage.image;
    colorToTransferSrcBarrier.image = drawImage.image;

    vk::ImageMemoryBarrier2 blitBarriers[2] = { undefinedToTransferDstBarrier, undefinedToColorAttachmentBarrier };
    vk::DependencyInfoKHR dependencyInfo = {
        .dependencyFlags = {},
        .imageMemoryBarrierCount = 2,
        .pImageMemoryBarriers = blitBarriers,
    };

    cmdBuffer.pipelineBarrier2(dependencyInfo);

    // Prepare Rendering
    //cmdBuffer.clearColorImage(swapchainImage, vk::ImageLayout::eTransferDstOptimal, clearColor, imageRange);
    vk::ClearValue clearColor = vk::ClearColorValue(1.0f, 1.0f, 1.0f, 1.0f);
    vk::ClearValue depthValue = vk::ClearDepthStencilValue(1.0f, 0);

    vk::RenderingAttachmentInfo colorAttachmentInfo = {
        .imageView = drawImage.view,
        .imageLayout = vk::ImageLayout::eColorAttachmentOptimal,
        .loadOp = vk::AttachmentLoadOp::eClear,
        .storeOp = vk::AttachmentStoreOp::eStore,
        .clearValue = clearColor,
    };
    vk::RenderingAttachmentInfo depthAttachmentInfo = {
        .imageView = m_vkCore.GetDepthImage(imgIndex).view,
        .imageLayout = vk::ImageLayout::eDepthAttachmentOptimal,
        .loadOp = vk::AttachmentLoadOp::eClear,
        .storeOp = vk::AttachmentStoreOp::eStore,
        .clearValue = depthValue
    };
    vk::RenderingInfo renderingInfo = {
        .renderArea = {.offset = {0,0}, .extent = {drawImage.extent.width, drawImage.extent.height}},
        .layerCount = 1,
        .colorAttachmentCount = 1,
        .pColorAttachments = &colorAttachmentInfo,
        .pDepthAttachment = &depthAttachmentInfo
    };

    // Draw here
    cmdBuffer.beginRendering(renderingInfo);

    m_graphicsPipeline->Bind(cmdBuffer);
    cmdBuffer.setViewport(0, vk::Viewport(0.0f, 0.0f, static_cast<float>(swapchainExtent.width), static_cast<float>(swapchainExtent.height), 0.0f, 1.0f));
    cmdBuffer.setScissor(0, vk::Rect2D(vk::Offset2D(0, 0), swapchainExtent));
    cmdBuffer.draw(3, 1, 0, 0);

    cmdBuffer.endRendering();

    // transfer draw image to swapchain image
    dependencyInfo = {
        .dependencyFlags = {},
        .imageMemoryBarrierCount = 1,
        .pImageMemoryBarriers = &colorToTransferSrcBarrier,
    };
    cmdBuffer.pipelineBarrier2(dependencyInfo);

    // copy draw image to swapchain for presentation
    vk::Extent2D drawExtent = {drawImage.extent.width, drawImage.extent.height};
    ptvk::CopyImage(cmdBuffer, drawImage.image, swapchainImage, drawExtent, swapchainExtent);

    // End frame draw commands recording
    dependencyInfo = {
        .dependencyFlags = {},
        .imageMemoryBarrierCount = 1,
        .pImageMemoryBarriers = &transferToPresentBarrier,
    };
    cmdBuffer.pipelineBarrier2(dependencyInfo);

    m_vkCore.SubmitFrame();
    m_vkCore.PresentFrame();
}
}
