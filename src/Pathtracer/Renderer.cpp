#ifndef TINYOBJLOADER_IMPLEMENTATION
#define TINYOBJLOADER_IMPLEMENTATION
#include <tinyobjloader/tiny_obj_loader.h>
#endif

#include <iostream>

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
    LoadScene();
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

void Renderer::LoadScene() {
    m_models.clear();

    // Mesh data
    tinyobj::attrib_t attrib;
    std::vector<tinyobj::shape_t> shapes;
    std::vector<tinyobj::material_t> materials;
    if (!tinyobj::LoadObj(&attrib, &shapes, &materials, nullptr, nullptr, "assets/statue.obj")) {
        std::cerr << "Failed to load obj file" << std::endl;
    }

    std::vector<Vertex> vertices{};
    std::vector<uint16_t> indices{};
    // Load vertex and index data
    for (auto& index : shapes[0].mesh.indices) {
        Vertex v{
            .pos = { attrib.vertices[index.vertex_index * 3], -attrib.vertices[index.vertex_index * 3 + 1], attrib.vertices[index.vertex_index * 3 + 2] },
            .normal = { attrib.normals[index.normal_index * 3], -attrib.normals[index.normal_index * 3 + 1], attrib.normals[index.normal_index * 3 + 2] },
            .uv = { attrib.texcoords[index.texcoord_index * 2], 1.0 - attrib.texcoords[index.texcoord_index * 2 + 1] }
        };
        vertices.push_back(v);
        indices.push_back(indices.size());
    }

    vk::DeviceSize vertexBufferSize = sizeof(Vertex) * vertices.size();
    vk::DeviceSize indexBufferSize = sizeof(uint16_t) * indices.size();
    vk::BufferCreateInfo bufferCreateInfo {
        .size = vertexBufferSize + indexBufferSize,
        .usage = vk::BufferUsageFlagBits::eVertexBuffer
            | vk::BufferUsageFlagBits::eIndexBuffer
            | vk::BufferUsageFlagBits::eStorageBuffer
            | vk::BufferUsageFlagBits::eAccelerationStructureBuildInputReadOnlyKHR,
    };
    VmaAllocationCreateInfo vmaAllocInfo{
        .flags = VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT
            | VMA_ALLOCATION_CREATE_HOST_ACCESS_ALLOW_TRANSFER_INSTEAD_BIT
            | VMA_ALLOCATION_CREATE_MAPPED_BIT,
        .usage = VMA_MEMORY_USAGE_AUTO
    };
    Buffer modelBuffer = m_vkCore->GetResourceAllocator().CreateBuffer(bufferCreateInfo, vmaAllocInfo);
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
    return;
}
}
