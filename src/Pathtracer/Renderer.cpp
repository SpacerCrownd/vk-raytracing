#include "Renderer.h"

namespace PathTracingVk {
Renderer::Renderer(int width, int height, const char *pAppName) : width(width), height(height) {
    m_mainWindow = std::make_unique<VulkanWindow>(width, height, pAppName);
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
    auto& cmdBuffer = m_vkCore->BeginCommandBuffer();
    ClearCmd(cmdBuffer);
    m_vkCore->SubmitFrame();
}

void Renderer::ClearCmd(vk::raii::CommandBuffer &cmdBuffer) {
    constexpr vk::ClearColorValue clearColor = {1.0f, .0f, .0f, .0f};

    constexpr vk::ImageSubresourceRange imageRange = {
        .aspectMask = vk::ImageAspectFlagBits::eColor,
        .baseMipLevel = 0,
        .levelCount = 1,
        .baseArrayLayer = 0,
        .layerCount = 1,
    };

    vk::ImageMemoryBarrier presentToClearBarrier = {
        .sType = vk::StructureType::eImageMemoryBarrier,
        .pNext = nullptr,
        .srcAccessMask = vk::AccessFlagBits::eMemoryRead,
        .dstAccessMask = vk::AccessFlagBits::eTransferWrite,
        .oldLayout = vk::ImageLayout::eUndefined,
        .newLayout = vk::ImageLayout::eTransferDstOptimal,
        .srcQueueFamilyIndex = vk::QueueFamilyIgnored,
        .dstQueueFamilyIndex = vk::QueueFamilyIgnored,
        .subresourceRange = imageRange,
    };

    vk::ImageMemoryBarrier clearToPresentBarrier = {
        .sType = vk::StructureType::eImageMemoryBarrier,
        .pNext = nullptr,
        .srcAccessMask = vk::AccessFlagBits::eTransferWrite,
        .dstAccessMask = vk::AccessFlagBits::eMemoryRead,
        .oldLayout = vk::ImageLayout::eTransferDstOptimal,
        .newLayout = vk::ImageLayout::ePresentSrcKHR,
        .srcQueueFamilyIndex = vk::QueueFamilyIgnored,
        .dstQueueFamilyIndex = vk::QueueFamilyIgnored,
        .subresourceRange = imageRange,
    };
    uint32_t imgIndex = m_vkCore->GetCurrentImageIndex();
    uint32_t currentFrame = m_vkCore->GetCurrentFrameIndex();
    presentToClearBarrier.image = m_vkCore->GetSwapchain()->GetSwapchainImage(static_cast<int>(imgIndex));
    clearToPresentBarrier.image = m_vkCore->GetSwapchain()->GetSwapchainImage(static_cast<int>(imgIndex));

    cmdBuffer.pipelineBarrier(vk::PipelineStageFlagBits::eTransfer,
                                      vk::PipelineStageFlagBits::eTransfer,
                                      vk::DependencyFlags{},
                                      {},
                                      {},
                                      {presentToClearBarrier});

    cmdBuffer.clearColorImage(m_vkCore->GetSwapchain()->GetSwapchainImage(static_cast<int>(imgIndex)),
                                      vk::ImageLayout::eTransferDstOptimal, clearColor, imageRange);

    cmdBuffer.pipelineBarrier(vk::PipelineStageFlagBits::eTransfer,
                                      vk::PipelineStageFlagBits::eBottomOfPipe,
                                      vk::DependencyFlags{},
                                      {},
                                      {},
                                      {clearToPresentBarrier});
}
}
