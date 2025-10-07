#include "VulkanApp.h"

#include <iostream>

namespace PathTracingVk {
VulkanApp::VulkanApp(int width, int height, const char *pAppName) : width(width), height(height) {
    m_mainWindow = std::make_unique<Window>(width, height, pAppName);
    m_vkCore = std::make_unique<VulkanCore>(pAppName, *m_mainWindow.get());
    m_numImages = m_vkCore->GetSwapchainImageCount();
    m_pQueue = m_vkCore->GetQueue();
    CreateCommandBuffers();
    RecordCommandBuffers();
}

VulkanApp::~VulkanApp() {
    if (m_vkCore)
        m_vkCore->DeviceWaitIdle();
}

void VulkanApp::Run() {
    MainLoop();
}

void VulkanApp::CreateCommandBuffers() {
    m_cmdBuffs.reserve(m_numImages);
    m_vkCore->CreateCommandBuffers(m_numImages, m_cmdBuffs);
}

void VulkanApp::RecordCommandBuffers() {
    Clear();
}

void VulkanApp::MainLoop() {
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

void VulkanApp::RenderFrame() {
    const uint32_t imgIndex = m_pQueue->AcquireNextImage();
    m_pQueue->SubmitAsync(*m_cmdBuffs[imgIndex]);
    m_pQueue->Present(imgIndex);
}

void VulkanApp::Clear() {
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

    for (uint32_t i = 0; i < m_cmdBuffs.size(); i++) {
        presentToClearBarrier.image = m_vkCore->GetSwapchainImage(static_cast<int>(i));
        clearToPresentBarrier.image = m_vkCore->GetSwapchainImage(static_cast<int>(i));

        m_cmdBuffs[i].begin({vk::StructureType::eCommandBufferBeginInfo, nullptr});

        m_cmdBuffs[i].pipelineBarrier(vk::PipelineStageFlagBits::eTransfer,
                                      vk::PipelineStageFlagBits::eTransfer,
                                      vk::DependencyFlags{},
                                      {},
                                      {},
                                      {presentToClearBarrier});

        m_cmdBuffs[i].clearColorImage(m_vkCore->GetSwapchainImage(static_cast<int>(i)),
                                      vk::ImageLayout::eTransferDstOptimal, clearColor, imageRange);

        m_cmdBuffs[i].pipelineBarrier(vk::PipelineStageFlagBits::eTransfer,
                                      vk::PipelineStageFlagBits::eBottomOfPipe,
                                      vk::DependencyFlags{},
                                      {},
                                      {},
                                      {clearToPresentBarrier});

        m_cmdBuffs[i].end();
    }
}
}
