#include "Renderer.h"

#include <iostream>

#include "vulkan/Utils.h"
#include "vulkan/Shader.h"
#include "shaders/shaderio.h.slang"
#include "GltfUtils.h"

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
        onResize();
    });

    m_pSamplerPool = std::make_unique<ptvk::SamplerPool>(m_vkCore.getDevice().getVkDevice());
    m_pStaging = std::make_unique<ptvk::StagingUploader>(m_vkCore.getResourceAllocator());
}

Renderer::~Renderer() {
    m_vkCore.deviceWaitIdle();
}

void Renderer::run() {
    createDescriptors();
    createFrameDataBuffers();
    loadShaders();
    createGraphicsPipeline();
    //createAccelerationStructures();
    //createRtPipeline();

    //initializeImGui();
    createScene();

    mainLoop();
}

void Renderer::mainLoop() {
    while (!glfwWindowShouldClose(m_window.getWindow())) {
        prepareFrameData();
        draw();
        glfwPollEvents();
    }
}

void Renderer::draw() {
    if (!m_vkCore.prepareFrame()) {
        handleResize();
        return;
    }

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

    // transition draw image for use depending on pipeline used + synchronize with fif for image reuse
    ptvk::utils::imageLayoutTransition(
        cmdBuffer,
        drawImage.image,
        vk::PipelineStageFlagBits2::eTransfer | vk::PipelineStageFlagBits2::eRayTracingShaderKHR | vk::PipelineStageFlagBits2::eColorAttachmentOutput,
        vk::PipelineStageFlagBits2::eRayTracingShaderKHR | vk::PipelineStageFlagBits2::eColorAttachmentOutput,
        vk::AccessFlagBits2::eTransferRead | vk::AccessFlagBits2::eShaderWrite | vk::AccessFlagBits2::eColorAttachmentWrite,
        vk::AccessFlagBits2::eShaderWrite | vk::AccessFlagBits2::eColorAttachmentWrite,
        vk::ImageLayout::eUndefined,
        m_currentPipeline == eRaster ? vk::ImageLayout::eColorAttachmentOptimal : vk::ImageLayout::eGeneral,
        subresourceRange);

    if (m_currentPipeline == eRaster) {
        // prepare to start dynamic rendering
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


        pushRtDescriptorSet(cmdBuffer);
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
                                 m_currentPipeline == eRaster ? vk::ImageLayout::eColorAttachmentOptimal : vk::ImageLayout::eGeneral,
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

    m_vkCore.submitFrame();
    m_vkCore.presentFrame();
}

void Renderer::onResize() {
    m_vkCore.framebufferResized = true;
}

void Renderer::handleResize() {

}

void Renderer::createScene() {
    m_scene.load("assets/sponza/sponza.glb");

    m_pVkScene = std::make_unique<ptvk::GltfSceneVulkan>(m_vkCore.getResourceAllocator(), *m_pSamplerPool, false);
    m_pRtScene = std::make_unique<ptvk::GltfSceneRt>(m_vkCore.getResourceAllocator(), m_vkCore.getDevice().getVkDevice());

    auto cmd = m_vkCore.beginSingleTimeCommandBuffer();
    m_pVkScene->createVkResources(cmd, *m_pStaging, m_scene);
    m_vkCore.submitSingleTimeCommandBuffer(cmd);
    m_pStaging->releaseStaging(); // release staging resources

    //cmd = m_vkCore.beginSingleTimeCommandBuffer();
    //m_pRtScene->create(cmd, m_scene, *m_pVkScene, vk::BuildAccelerationStructureFlagBitsKHR::ePreferFastTrace);

    finalizeScene();
}

// call this after loading/creating a new scene, when descriptors are available
void Renderer::finalizeScene() {
    // TODO: update texture descriptor

    // TODO: scene ui integration
}

void Renderer::loadShaders() {
    m_pRasterShader = std::make_unique<ptvk::Shader>(m_vkCore.getDevice().getVkDevice(), "raster.spv");
    m_pRtShader = std::make_unique<ptvk::Shader>(m_vkCore.getDevice().getVkDevice(), "pathtrace.spv");
    printf("[INFO] Shaders Loaded\n");
}

void Renderer::createDescriptors() {
    auto& device = m_vkCore.getDevice().getVkDevice();

    auto& deviceProperties = m_vkCore.getDevice().getPhysicalDevice().m_devProperties2.properties;
    m_maxTextures = std::min(m_maxTextures, deviceProperties.limits.maxDescriptorSetSampledImages - 1);
    m_maxSamplers = std::min(m_maxTextures, deviceProperties.limits.maxDescriptorSetSamplers - 1);

    // descriptor set for textures and samplers
    // no need for update flags since we only update textures and samplers on scene load and not during rendering (no add/remove ops, currently)
    vk::DescriptorBindingFlags descVariableFlags[2] {
        { // textures
            vk::DescriptorBindingFlagBits::ePartiallyBound
        },
        { // samplers
            vk::DescriptorBindingFlagBits::ePartiallyBound
        }
    };
    vk::DescriptorSetLayoutBindingFlagsCreateInfo descBindingFlags = {
        .bindingCount = 2,
        .pBindingFlags = descVariableFlags
    };

    // textures binding
    m_textureDescriptorBindings[0] = {
        .binding = shaderio::DescriptorBindingPoints::eTextures,
        .descriptorType = vk::DescriptorType::eSampledImage,
        .descriptorCount = m_maxTextures,
        .stageFlags = vk::ShaderStageFlagBits::eAll,
    };

    // samplers binding
    m_textureDescriptorBindings[1] = {
        .binding = shaderio::DescriptorBindingPoints::eSamplers,
        .descriptorType = vk::DescriptorType::eSampler,
        .descriptorCount = m_maxSamplers,
        .stageFlags = vk::ShaderStageFlagBits::eAll
    };

    vk::DescriptorSetLayoutCreateInfo layoutInfo = {
        .pNext = &descBindingFlags,
        .bindingCount = static_cast<uint32_t>(m_textureDescriptorBindings.size()),
        .pBindings = m_textureDescriptorBindings.data()
    };

    m_textureDescriptorLayout = device.createDescriptorSetLayout(layoutInfo);

    std::vector<vk::DescriptorPoolSize> poolSizes{
        vk::DescriptorPoolSize(vk::DescriptorType::eSampledImage, m_maxTextures),
        vk::DescriptorPoolSize(vk::DescriptorType::eSampler, m_maxSamplers),
    };

    vk::DescriptorPoolCreateInfo poolInfo = {
        .flags = vk::DescriptorPoolCreateFlagBits::eFreeDescriptorSet,
        .maxSets = 1,
        .poolSizeCount = static_cast<uint32_t>(poolSizes.size()),
        .pPoolSizes = poolSizes.data(),
    };

    m_descriptorPool = device.createDescriptorPool(poolInfo);

    vk::DescriptorSetAllocateInfo descAllocInfo = {
        .descriptorPool = m_descriptorPool,
        .descriptorSetCount = 1,
        .pSetLayouts = &*m_textureDescriptorLayout
    };
    m_textureDescriptorSet = std::move(device.allocateDescriptorSets(descAllocInfo).front());

    // descriptor set for tlas and output image (will be constructed as push descriptors)
    // tlas
    m_rtDescriptorBindings[0] = vk::DescriptorSetLayoutBinding(
        shaderio::DescriptorBindingPoints::eTlas,
        vk::DescriptorType::eAccelerationStructureKHR,
        1,
        vk::ShaderStageFlagBits::eAll);
    // out storage image
    m_rtDescriptorBindings[1] = vk::DescriptorSetLayoutBinding(
        shaderio::DescriptorBindingPoints::eOutImage,
        vk::DescriptorType::eStorageImage,
        1,
        vk::ShaderStageFlagBits::eAll);

    vk::DescriptorSetLayoutCreateInfo rtLayoutInfo = {
        .flags = vk::DescriptorSetLayoutCreateFlagBits::ePushDescriptor,
        .bindingCount = static_cast<uint32_t>(m_rtDescriptorBindings.size()),
        .pBindings = m_rtDescriptorBindings.data()
    };
    m_rtDescriptorLayout = device.createDescriptorSetLayout(rtLayoutInfo);
}

void Renderer::pushRtDescriptorSet(const vk::raii::CommandBuffer& cmd) {
    vk::WriteDescriptorSetAccelerationStructureKHR descAccel {
        .accelerationStructureCount = 1,
        .pAccelerationStructures = &*m_pRtScene->getTlas()
    };

    std::array<vk::WriteDescriptorSet, 2> writes{};

    // TLAS
    writes[0] = vk::WriteDescriptorSet{
        .pNext = &descAccel,
        .dstBinding = shaderio::DescriptorBindingPoints::eTlas,
        .dstArrayElement = 0,
        .descriptorCount = 1,
        .descriptorType = vk::DescriptorType::eAccelerationStructureKHR,
    };

    // Output image
    vk::DescriptorImageInfo outImageDescriptor{
        .imageView = m_vkCore.getDrawImage().view,
        .imageLayout = vk::ImageLayout::eGeneral
    };

    writes[1] = vk::WriteDescriptorSet{
        .dstBinding = shaderio::DescriptorBindingPoints::eOutImage,
        .dstArrayElement = 0,
        .descriptorCount = 1,
        .descriptorType = vk::DescriptorType::eStorageImage,
        .pImageInfo = &outImageDescriptor,
    };

    cmd.pushDescriptorSetKHR(vk::PipelineBindPoint::eRayTracingKHR, m_pRtPipeline->getLayout(), 1, writes);
}

void Renderer::createFrameDataBuffers() {
    // create frame data buffer
    vk::BufferCreateInfo bufferInfo = {
        .size = sizeof(shaderio::FrameData),
        .usage = vk::BufferUsageFlagBits::eUniformBuffer | vk::BufferUsageFlagBits::eShaderDeviceAddress
    };

    VmaAllocationCreateInfo allocInfo = {
        .flags = VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT
               | VMA_ALLOCATION_CREATE_MAPPED_BIT,
        .usage = VMA_MEMORY_USAGE_AUTO,
        .requiredFlags = VK_MEMORY_PROPERTY_HOST_COHERENT_BIT | VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT
    };

    for (int i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
        m_bFrameData[i] = m_vkCore.getResourceAllocator().createBuffer(bufferInfo, allocInfo);
    }
}

void Renderer::createGraphicsPipeline() {
    auto colorFormat = m_vkCore.getDrawImage().format;
    auto depthFormat = m_vkCore.getDepthFormat();
    m_pGraphicsPipeline = std::make_unique<ptvk::GraphicsPipeline>(m_vkCore.getDevice().getVkDevice(), *m_pRasterShader, 1, colorFormat, depthFormat, m_enableDepth);
}

void Renderer::createAccelerationStructures() {

}

void Renderer::createRtPipeline() {

}

void Renderer::prepareFrameData() {
    // sync scene changes with gpu
}

void Renderer::initializeImGui() {

}
}
