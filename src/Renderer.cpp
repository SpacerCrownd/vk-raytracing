#include "Renderer.h"
#include "vulkan/Utils.h"
#include "GltfUtils.h"
#include "vulkan/Shader.h"

namespace app {
Renderer::Renderer(int width, int height, const char* pAppName) : width(width), height(height),
                                                                  m_mainWindow(width, height, pAppName),
                                                                  m_vkCore(pAppName, m_mainWindow),
                                                                  m_scene(),
                                                                  m_camera(glm::vec3(0.0f, 0.0f, 0.0f))
{
    m_mainWindow.AddOnKeyChanged([this](int key, int scancode, int action, int mods){
        m_camera.OnKeyChanged(key, scancode, action, mods);
    });

    m_mainWindow.AddOnMouseButtonChanged([this](int button, int action, int mods) {
        m_camera.OnMouseButtonChanged(this->m_mainWindow.GetWindow(), button, action, mods);
    });

    m_mainWindow.AddOnCursorPositionChanged([this](double x, double y) {
        m_camera.OnCursorPositionChanged(x, y);
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
    while (!glfwWindowShouldClose(m_mainWindow.GetWindow())) {
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
    
}

void Renderer::Draw() {
    m_vkCore.PrepareFrame();

    auto& cmdBuffer = m_vkCore.BeginCommandRecording();
    vk::ClearColorValue clearColor = {.0f, .0f, 1.0f, .0f};

    vk::ImageSubresourceRange imageRange = {
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

    uint32_t imgIndex = m_vkCore.GetCurrentImageIndex();
    presentToWriteBarrier.image = m_vkCore.GetSwapchain().GetSwapchainImage(static_cast<int>(imgIndex));
    writeToPresentBarrier.image = m_vkCore.GetSwapchain().GetSwapchainImage(static_cast<int>(imgIndex));

    vk::DependencyInfoKHR dependencyInfo = {
        .dependencyFlags = {},
        .imageMemoryBarrierCount = 1,
        .pImageMemoryBarriers = &presentToWriteBarrier,
    };

    cmdBuffer.pipelineBarrier2(dependencyInfo);

    cmdBuffer.clearColorImage(m_vkCore.GetSwapchain().GetSwapchainImage(static_cast<int>(imgIndex)),
                              vk::ImageLayout::eTransferDstOptimal, clearColor, imageRange);

    dependencyInfo = {
        .dependencyFlags = {},
        .imageMemoryBarrierCount = 1,
        .pImageMemoryBarriers = &writeToPresentBarrier,
    };

    cmdBuffer.pipelineBarrier2(dependencyInfo);

    m_vkCore.SubmitFrame();
    m_vkCore.PresentFrame();
}

void Renderer::PrepareFrameData() {
    return;
}

}
