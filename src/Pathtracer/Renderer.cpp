#include "Renderer.h"

#include <iostream>

namespace PathTracingVk {
Renderer::Renderer(int width, int height, const char *pAppName) : width(width), height(height) {
    m_mainWindow = std::make_unique<VulkanWindow>(width, height, pAppName);
    m_renderer = std::make_unique<VulkanCore>(pAppName, *m_mainWindow);
}

Renderer::~Renderer() {
    if (m_renderer)
        m_renderer->DeviceWaitIdle();
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
    // TODO transfer fence waiting and presenting to command buffer submission in vulkan_core, only record commands here
    // The core exposes the current command buffer
    // One buffer per frame in flight, per queue type
    /** Command pool/buffer lifecycle
     * Begin
     *  Take existing buffer and pool for the specified queue type
     *  flush pool
     * Record commands
     * Submit commands
     *  synchronization
     */
    while (m_device->GetDevice().waitForFences(*m_inFlightFences[m_inFlightFrameIndex], vk::True, UINT64_MAX) == vk::Result::eTimeout);
    m_device->GetDevice().resetFences(*m_inFlightFences[m_inFlightFrameIndex]);

    uint32_t imgIndex;
    vk::Result res = m_swapchain->AcquireNextImage(m_presentFinishedSemaphores[m_inFlightFrameIndex], imgIndex);

    ResetCommandPool(m_inFlightFrameIndex);
    RecordCommandBuffer(imgIndex);
    m_queue->SubmitAsync(*m_cmdBuffs[m_queue->GetCurrentFrameIndex()]);
    m_queue->Present(imgIndex);
}

void Renderer::Clear(uint32_t imgIndex) {
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

    uint32_t currentFrame = m_queue->GetCurrentFrameIndex();
    presentToClearBarrier.image = m_swapchain->GetSwapchainImage(static_cast<int>(imgIndex));
    clearToPresentBarrier.image = m_swapchain->GetSwapchainImage(static_cast<int>(imgIndex));

    m_cmdBuffs[currentFrame].begin({vk::StructureType::eCommandBufferBeginInfo, nullptr});

    m_cmdBuffs[currentFrame].pipelineBarrier(vk::PipelineStageFlagBits::eTransfer,
                                      vk::PipelineStageFlagBits::eTransfer,
                                      vk::DependencyFlags{},
                                      {},
                                      {},
                                      {presentToClearBarrier});

    m_cmdBuffs[currentFrame].clearColorImage(m_renderer->GetSwapchainImage(static_cast<int>(imgIndex)),
                                      vk::ImageLayout::eTransferDstOptimal, clearColor, imageRange);

    m_cmdBuffs[currentFrame].pipelineBarrier(vk::PipelineStageFlagBits::eTransfer,
                                      vk::PipelineStageFlagBits::eBottomOfPipe,
                                      vk::DependencyFlags{},
                                      {},
                                      {},
                                      {clearToPresentBarrier});

    m_cmdBuffs[currentFrame].end();
}
}