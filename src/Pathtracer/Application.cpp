#include "Application.h"

#include <iostream>

namespace PathTracingVk {
Application::Application(int width, int height, const char *pAppName) : width(width), height(height) {
    m_mainWindow = std::make_unique<VulkanWindow>(width, height, pAppName);
    m_vkCore = std::make_unique<Renderer>(pAppName, *m_mainWindow);
    m_queue = m_vkCore->GetQueue();
    m_swapchain = m_vkCore->GetSwapchain();
    m_device = m_vkCore->GetDevice();
    m_swapchainImageCount = m_swapchain->GetSwapchainImageCount();
    CreateCommandBuffers();
    CreateSyncObjs();
}

Application::~Application() {
    if (m_vkCore)
        m_vkCore->DeviceWaitIdle();
}

void Application::Run() {
    MainLoop();
}

void Application::CreateCommandBuffers() {
    m_cmdBuffs.reserve(MAX_FRAMES_IN_FLIGHT);
    m_vkCore->CreateCommandBuffers(MAX_FRAMES_IN_FLIGHT, m_cmdBuffs);
}

void Application::RecordCommandBuffer(uint32_t imgIndex) {
    Clear(imgIndex);
}

void Application::CreateSyncObjs() {
    m_inFlightFences.clear();
    m_presentFinishedSemaphores.clear();
    m_renderFinishedSemaphores.clear();

    // create one acquisition semaphore for each swapchain image
    for (int i = 0; i < m_swapchainImageCount; i++) {
        m_renderFinishedSemaphores.emplace_back(m_device->GetDevice(), vk::SemaphoreCreateInfo());
    }

    // for each in-flight frame create submit semaphores and acquisition fences
    for (int i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
        m_presentFinishedSemaphores.emplace_back(m_device->GetDevice(), vk::SemaphoreCreateInfo());
        m_inFlightFences.emplace_back(m_device->GetDevice(), vk::FenceCreateInfo{.flags = vk::FenceCreateFlagBits::eSignaled});
    }
}

void Application::MainLoop() {
    /*
    auto curTime = static_cast<float>(glfwGetTime());
    int frames = 0;
    float fpsTime = 0.0f;
    */
    while (!glfwWindowShouldClose(m_mainWindow->GetWindow())) {
        RenderFrame();
        glfwPollEvents();
    }
}

void Application::RenderFrame() {
    while (m_device->GetDevice().waitForFences(*m_inFlightFences[m_inFlightFrameIndex], vk::True, UINT64_MAX) == vk::Result::eTimeout);
    m_device->GetDevice().resetFences(*m_inFlightFences[m_inFlightFrameIndex]);

    uint32_t imgIndex;
    vk::Result res = m_swapchain->AcquireNextImage(m_presentFinishedSemaphores[m_inFlightFrameIndex], imgIndex);

    m_vkCore->ResetCommandPool(m_inFlightFrameIndex);
    RecordCommandBuffer(imgIndex);
    m_queue->SubmitAsync(*m_cmdBuffs[m_queue->GetCurrentFrameIndex()]);
    m_queue->Present(imgIndex);
}

void Application::Clear(uint32_t imgIndex) {
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

    m_cmdBuffs[currentFrame].clearColorImage(m_vkCore->GetSwapchainImage(static_cast<int>(imgIndex)),
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