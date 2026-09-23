#include "Renderer.h"
#include "GltfUtils.h"
#include "vulkan/Utils.h"
#include "vulkan/Shader.h"

#include <iostream>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtx/string_cast.hpp>

namespace app {
Renderer::Renderer(int width, int height, const char* pAppName) : m_window(width, height, pAppName),
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
        onResize(width, height);
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
    //std::string file = "assets/basicmesh.glb";
    std::string file = "assets/sponza/sponza.glb";
    if (!createScene(file)) {
        cleanupScene();
    }

    mainLoop();
}

void Renderer::mainLoop() {
    while (!glfwWindowShouldClose(m_window.getWindow())) {
        draw();
        glfwPollEvents();
    }
}

void Renderer::update() {
    m_camera.update();
    m_pScene->updateNodeWorldMatrices(); // updates render nodes that were modified
}

void Renderer::draw() {
    if (!m_pScene) {
        return; // no valid scene loaded, don't render
    }

    if (!m_vkCore.prepareFrame()) {
        handleResize();
        return;
    }

    auto& cmdBuffer = m_vkCore.beginCommandRecording();

    prepareFrameData(cmdBuffer);

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
        vk::ClearValue clearColor = vk::ClearColorValue(.0f, .0f, .0f, 1.0f);
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

        cmdBuffer.bindDescriptorSets(vk::PipelineBindPoint::eGraphics,
            m_pGraphicsPipeline->getLayout(),
            0,
            *m_textureDescriptorSet,
            nullptr);

        struct NodeData {
            int materialID;
            int renderNodeID;
            int renderPrimID;
        };

        // start drawing
        const auto& rnodes = m_pScene->getRenderNodes();
        uint32_t offset = offsetof(shaderio::RasterPushConstant, materialID);

        for (int nodeID = 0; nodeID < rnodes.size(); nodeID++) {
            const auto& rnode = rnodes[nodeID];
            const RenderPrimitive& subMesh = m_pScene->getRenderPrimitive(rnode.renderPrimID);
            NodeData pushConst = {
                .materialID = rnode.materialID,
                .renderNodeID = nodeID,
                .renderPrimID = rnode.renderPrimID
            };
            cmdBuffer.pushConstants<NodeData>(m_pGraphicsPipeline->getLayout(), vk::ShaderStageFlagBits::eAllGraphics, offset, pushConst);

            cmdBuffer.bindVertexBuffers(0, m_pVkScene->getVertexBuffers()[rnode.renderPrimID].buffer, {0});
            cmdBuffer.bindIndexBuffer(m_pVkScene->getIndexBuffers()[rnode.renderPrimID].buffer, 0, vk::IndexType::eUint32);
            cmdBuffer.drawIndexed(subMesh.indexCount, 1, 0, 0, 0);
        }

        cmdBuffer.endRendering();

    } else if (m_currentPipeline == eRaytracing) {


        pushRtDescriptors(cmdBuffer);
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

void Renderer::onResize(int width, int height) {
    m_vkCore.framebufferResized = true;
}

void Renderer::handleResize() {

}

bool Renderer::createScene(const std::filesystem::path &filename) {
    m_pScene = std::make_unique<GltfScene>(m_camera);
    m_pScene->load(filename);

    m_pVkScene = std::make_unique<ptvk::GltfSceneVulkan>(m_vkCore.getResourceAllocator(), *m_pSamplerPool, false);
    m_pRtScene = std::make_unique<ptvk::GltfSceneRt>(m_vkCore.getResourceAllocator(),
                                                     m_vkCore.getDevice().getVkDevice());

    // create vulkan resources for loaded scene

    auto cmd = m_vkCore.beginSingleTimeCommandBuffer();

    m_pVkScene->createVkResources(cmd, *m_pStaging, *m_pScene);

    m_vkCore.submitSingleTimeCommandBuffer(cmd);
    m_pStaging->releaseStaging(); // release staging resources


    //cmd = m_vkCore.beginSingleTimeCommandBuffer();
    //m_pRtScene->create(cmd, m_scene, *m_pVkScene, vk::BuildAccelerationStructureFlagBitsKHR::ePreferFastTrace);

    // populate texture and sampler descriptor set

    int imgCount = m_pVkScene->getTextureCount();
    if (imgCount > m_maxTextures) {
        std::cout << std::format("Scene requires {} textures but descriptor set supports {}\n", imgCount, m_maxTextures);
        return false;
    }

    int samplerCount = m_pVkScene->getSamplerCount();
    if (samplerCount > m_maxSamplers) {
        std::cout << std::format("Scene requires {} samplers but descriptor set supports {}\n", samplerCount, m_maxSamplers);
        return false;
    }

    std::vector<vk::WriteDescriptorSet> writes{};

    // write textures into descriptor
    std::vector<vk::DescriptorImageInfo> textureWriteInfos;
    for (size_t i = 0; i < imgCount; i++) {
        const auto &img = m_pVkScene->getTextureImage(i);
        textureWriteInfos.push_back({
            .imageView = img.view,
            .imageLayout = vk::ImageLayout::eShaderReadOnlyOptimal
        });
    }

    vk::WriteDescriptorSet imagesWrite = {
        .dstSet = m_textureDescriptorSet,
        .dstBinding = shaderio::DescriptorBindingPoints::eTextures,
        .dstArrayElement = 0,
        .descriptorCount = static_cast<uint32_t>(imgCount),
        .descriptorType = vk::DescriptorType::eSampledImage,
        .pImageInfo = textureWriteInfos.data()
    };
    writes.push_back(imagesWrite);

    // write samplers into descriptor
    std::vector<vk::DescriptorImageInfo> samplerWriteInfos;
    for (size_t i = 0; i < samplerCount; i++) {
        const auto &sampler = m_pVkScene->getSampler(i);
        samplerWriteInfos.push_back({
            .sampler = sampler,
        });
    }

    vk::WriteDescriptorSet samplerWrite = {
        .dstSet = m_textureDescriptorSet,
        .dstBinding = shaderio::DescriptorBindingPoints::eSamplers,
        .dstArrayElement = 0,
        .descriptorCount = static_cast<uint32_t>(samplerCount),
        .descriptorType = vk::DescriptorType::eSampler,
        .pImageInfo = samplerWriteInfos.data()
    };
    writes.push_back(samplerWrite);

    m_vkCore.getDevice().getVkDevice().updateDescriptorSets(writes, {});
    return true;
}

void Renderer::cleanupScene() {
    m_vkCore.deviceWaitIdle();
    // TODO clean up ui related to scene

    m_pScene.reset();
    m_pVkScene.reset();
    m_pRtScene.reset();
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

void Renderer::pushRtDescriptors(const vk::raii::CommandBuffer& cmd) {
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
        .usage = vk::BufferUsageFlagBits::eUniformBuffer | vk::BufferUsageFlagBits::eShaderDeviceAddress | vk::BufferUsageFlagBits::eTransferDst
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
    std::vector<vk::DescriptorSetLayout> layouts {m_textureDescriptorLayout};
    m_pGraphicsPipeline = std::make_unique<ptvk::GraphicsPipeline>(m_vkCore.getDevice().getVkDevice(), *m_pRasterShader, 1, colorFormat, depthFormat, m_enableDepth, layouts);
}

void Renderer::createAccelerationStructures() {

}

void Renderer::createRtPipeline() {

}

void Renderer::initializeImGui() {

}

void Renderer::prepareFrameData(const vk::raii::CommandBuffer& cmd) {
    int frame = m_vkCore.getCurrentFrameIndex();
    // sync scene changes with gpu
    m_pVkScene->updateFromScene(*m_pScene, frame);

    // update frame data buffer
    auto [width, height, depth] = m_vkCore.getDrawImage().extent;
    glm::mat4x4 projMat = glm::perspectiveRH_ZO(glm::radians(90.0f), static_cast<float>(width)/static_cast<float>(height), 0.1f, 1000.0f);
    projMat[1][1] *= -1; // flip y
    shaderio::FrameData frameData = {
        .projectionMat = projMat,
        .viewMat = m_camera.getViewMatrix(),
        .invViewProjMat = glm::inverse(projMat * m_camera.getViewMatrix()),
        .cameraPosition = glm::vec4(m_camera.position, 0),
        .backgroundColor = glm::vec4(0,0,0,0),
    };
    cmd.updateBuffer<shaderio::FrameData>(m_bFrameData[frame].buffer, 0, frameData);

    // update push constant
    if (m_currentPipeline == eRaster) {
        m_rasterPushConstant.frameData = reinterpret_cast<shaderio::FrameData *>(m_bFrameData[frame].address);
        m_rasterPushConstant.sceneInfo = reinterpret_cast<shaderio::GltfSceneInfo *>(m_pVkScene->getSceneInfo(frame).address);

        cmd.pushConstants<shaderio::RasterPushConstant>(
            m_pGraphicsPipeline->getLayout(),
            vk::ShaderStageFlagBits::eAllGraphics,
            0,
            m_rasterPushConstant);

        ptvk::utils::cmdMemoryBarrier(cmd,
                        vk::PipelineStageFlagBits2::eTransfer,
                          vk::PipelineStageFlagBits2::eRayTracingShaderKHR | vk::PipelineStageFlagBits2::eAllGraphics,
                       vk::AccessFlagBits2::eTransferWrite,
                         vk::AccessFlagBits2::eShaderRead);
    } else {
        // rt
    }

    // update tlas
}

}
