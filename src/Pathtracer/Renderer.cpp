#include "Renderer.h"

namespace PathTracingVk {
Renderer::Renderer(int width, int height, const char *pAppName) : width(width), height(height) {
    m_mainWindow = std::make_unique<VulkanWindow>(width, height, pAppName);

    m_camera = std::make_unique<Camera>();
    m_mainWindow->AddOnKeyChanged(
    [cam = m_camera.get()](int key, int scancode, int action, int mods)
    {
        cam->OnKeyChanged(key, scancode, action, mods);
    }
);

    m_vkCore = std::make_unique<VulkanCore>(pAppName, *m_mainWindow);
}

Renderer::~Renderer() {
    if (m_vkCore)
        m_vkCore->DeviceWaitIdle();
}

void Renderer::Run() {
    MainLoop();
}

void Renderer::MainLoop() {
    /*
    auto curTime = static_cast<float>(glfwGetTime());
    int frames = 0;
    float fpsTime = 0.0f;
    */
    while (!glfwWindowShouldClose(m_mainWindow->GetWindow())) {
        Draw();
        glfwPollEvents();
    }
}

void Renderer::Draw() {
    m_vkCore->PrepareFrame();

    auto& cmdBuffer = m_vkCore->BeginCommandRecording();
    constexpr vk::ClearColorValue clearColor = {1.0f, .0f, .0f, .0f};

    constexpr vk::ImageSubresourceRange imageRange = {
        .aspectMask = vk::ImageAspectFlagBits::eColor,
        .baseMipLevel = 0,
        .levelCount = 1,
        .baseArrayLayer = 0,
        .layerCount = 1,
    };

    vk::ImageMemoryBarrier2 presentToWriteBarrier = {
        .srcStageMask = vk::PipelineStageFlagBits2::eTransfer,
        .srcAccessMask = vk::AccessFlagBits2::eMemoryRead,
        .dstStageMask = vk::PipelineStageFlagBits2::eTransfer,
        .dstAccessMask = vk::AccessFlagBits2::eTransferWrite,
        .oldLayout = vk::ImageLayout::eUndefined,
        .newLayout = vk::ImageLayout::eTransferDstOptimal,
        .srcQueueFamilyIndex = vk::QueueFamilyIgnored,
        .dstQueueFamilyIndex = vk::QueueFamilyIgnored,
        .subresourceRange = imageRange,
    };

    vk::ImageMemoryBarrier2 writeToPresentBarrier = {
        .srcStageMask = vk::PipelineStageFlagBits2::eTransfer,
        .srcAccessMask = vk::AccessFlagBits2::eTransferWrite,
        .dstStageMask = vk::PipelineStageFlagBits2::eNone,
        .dstAccessMask = vk::AccessFlagBits2::eMemoryRead,
        .oldLayout = vk::ImageLayout::eTransferDstOptimal,
        .newLayout = vk::ImageLayout::ePresentSrcKHR,
        .srcQueueFamilyIndex = vk::QueueFamilyIgnored,
        .dstQueueFamilyIndex = vk::QueueFamilyIgnored,
        .subresourceRange = imageRange,
    };

    uint32_t imgIndex = m_vkCore->GetCurrentImageIndex();
    presentToWriteBarrier.image = m_vkCore->GetSwapchain()->GetSwapchainImage(static_cast<int>(imgIndex));
    writeToPresentBarrier.image = m_vkCore->GetSwapchain()->GetSwapchainImage(static_cast<int>(imgIndex));

    vk::DependencyInfoKHR dependencyInfo = {
        .dependencyFlags = {},
        .imageMemoryBarrierCount = 1,
        .pImageMemoryBarriers = &presentToWriteBarrier,
    };

    cmdBuffer.pipelineBarrier2(dependencyInfo);

    cmdBuffer.clearColorImage(m_vkCore->GetSwapchain()->GetSwapchainImage(static_cast<int>(imgIndex)),
                                      vk::ImageLayout::eTransferDstOptimal, clearColor, imageRange);

    dependencyInfo = {
        .dependencyFlags = {},
        .imageMemoryBarrierCount = 1,
        .pImageMemoryBarriers = &writeToPresentBarrier,
    };

    cmdBuffer.pipelineBarrier2(dependencyInfo);

    m_vkCore->SubmitFrame();
    m_vkCore->PresentFrame();
}

void Renderer::PrepareFrameData() {

}
}
