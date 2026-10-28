#include "Renderer.h"
#include "GltfUtils.h"
#include "vulk/Utils.h"
#include "vulk/Shader.h"

#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtx/string_cast.hpp>
#include <glm/gtx/quaternion.hpp>
#include <nfd.h>
#include <iostream>

namespace app {
Renderer::Renderer(int width, int height, const char *pAppName) : m_window(width, height, pAppName),
                                                                  m_vkCore(pAppName, m_window) {
    m_window.addOnKeyChanged([this](int key, int scancode, int action, int mods) {
        m_camera.onKeyChanged(key, scancode, action, mods);
        if (key == GLFW_KEY_SPACE && action == GLFW_PRESS) {
            m_showGUI = !m_showGUI;
            std::cout << m_showGUI << std::endl;
        }
    });

    m_window.addOnMouseButtonChanged([this](int button, int action, int mods) {
        if (!IsMouseControlledByImGUI()) {
            m_camera.onMouseButtonChanged(this->m_window.getWindow(), button, action, mods);
        }
    });

    m_window.addOnCursorPositionChanged([this](double x, double y) {
        m_camera.onCursorPositionChanged(x, y);
    });

    m_window.addOnFramebufferSizeChanged([this](int width, int height) {
        onResize(width, height);
    });

    m_window.addOnScrollChanged([this](double xoffset, double yoffset) {
        m_camera.onScroll(xoffset, yoffset);
    });

    m_pSamplerPool = std::make_unique<ptvk::SamplerPool>(m_vkCore.getDevice().getVkDevice());
    m_pStaging = std::make_unique<ptvk::StagingUploader>(m_vkCore.getResourceAllocator());
    m_pImgui = std::make_unique<ptvk::ImguiVulkan>(m_vkCore);
    m_pDenoiser = std::make_unique<ptvk::OidnDenoiser>(width, height, m_vkCore.getResourceAllocator());
}

Renderer::~Renderer() {
    m_vkCore.deviceWaitIdle();
}

void Renderer::run() {
    createDescriptors();

    createFrameDataBuffers();
    loadShaders();
    createGraphicsPipeline();
    createRtPipeline();

    mainLoop();
}

void Renderer::mainLoop() {
    while (!glfwWindowShouldClose(m_window.getWindow())) {
        update();
        draw();
        glfwPollEvents();
    }
}

void Renderer::update() {
    double currentFrameTime = glfwGetTime();
    m_deltaTime = currentFrameTime - m_lastFrameTime;
    m_lastFrameTime = currentFrameTime;

    /* Node update test
    auto& node = m_pScene->getModel().nodes[0];
    glm::vec3 translation, scale;
    glm::quat rotation;
    gltfutils::getNodeTRS(node, translation, rotation, scale);

    glm::quat deltaRotation = glm::angleAxis(glm::radians(360.0f * static_cast<float>(m_deltaTime)) , glm::vec3(0.0f, 1.0f, 0.0f));
    rotation = rotation * deltaRotation;
    m_pScene->setNodeTRS(0, translation, rotation, scale);
    */

    m_camera.update(m_deltaTime);
}

void Renderer::draw() {
    if (m_vkCore.resizeRequested) {
        m_vkCore.recreateSwapchain();
        resetAccumulation();
    }

    if (!m_vkCore.prepareFrame()) {
        return;
    }

    auto *pCmd = &m_vkCore.beginCommandRecording();

    updateGUI();

    vk::ImageSubresourceRange subresourceRange = {
        .aspectMask = vk::ImageAspectFlagBits::eColor,
        .baseMipLevel = 0,
        .levelCount = 1,
        .baseArrayLayer = 0,
        .layerCount = 1,
    };

    // get swapchain image
    uint32_t imgIndex = m_vkCore.getCurrentImageIndex();
    const auto &swapchainImage = m_vkCore.getSwapchain().GetSwapchainImage(static_cast<int>(imgIndex));
    const auto &swapchainImageView = m_vkCore.getSwapchain().GetSwapchainImageView(static_cast<int>(imgIndex));
    auto swapchainExtent = m_vkCore.getSwapchain().GetExtent();

    // transition swapchain image to attachment
    ptvk::utils::imageLayoutTransition(*pCmd,
                                       swapchainImage,
                                       {},
                                       vk::PipelineStageFlagBits2::eColorAttachmentOutput,
                                       {},
                                       vk::AccessFlagBits2::eColorAttachmentWrite,
                                       vk::ImageLayout::eUndefined,
                                       vk::ImageLayout::eColorAttachmentOptimal,
                                       subresourceRange);

    if (m_pSceneVk) {
        prepareFrameData(*pCmd);

        // get draw image
        auto &drawImage = m_vkCore.getDrawImage();
        auto &accImage = m_vkCore.getAccumulationImage();
        auto &albedoImage = m_vkCore.getAlbedoImage();
        auto &normalImage = m_vkCore.getNormalImage();
        //////
        //  Main Draw Pass
        //////
        {
            if (currentPipeline == eRaster) {
                // transition draw image for use depending on pipeline used + synchronize with fif for image reuse
                ptvk::utils::imageLayoutTransition(
                    *pCmd,
                    drawImage.image,
                    vk::PipelineStageFlagBits2::eFragmentShader,
                    vk::PipelineStageFlagBits2::eColorAttachmentOutput,
                    vk::AccessFlagBits2::eShaderRead,
                    vk::AccessFlagBits2::eColorAttachmentWrite,
                    vk::ImageLayout::eUndefined,
                    vk::ImageLayout::eColorAttachmentOptimal,
                    subresourceRange);

                // synchronize depth image usage
                ptvk::utils::imageLayoutTransition(
                    *pCmd,
                    m_vkCore.getDepthImage().image,
                    vk::PipelineStageFlagBits2::eEarlyFragmentTests | vk::PipelineStageFlagBits2::eLateFragmentTests,
                    vk::PipelineStageFlagBits2::eEarlyFragmentTests | vk::PipelineStageFlagBits2::eLateFragmentTests,
                    vk::AccessFlagBits2::eDepthStencilAttachmentRead |
                    vk::AccessFlagBits2::eDepthStencilAttachmentWrite,
                    vk::AccessFlagBits2::eDepthStencilAttachmentRead |
                    vk::AccessFlagBits2::eDepthStencilAttachmentWrite,
                    vk::ImageLayout::eDepthAttachmentOptimal,
                    vk::ImageLayout::eDepthAttachmentOptimal,
                    {
                        .aspectMask = vk::ImageAspectFlagBits::eDepth,
                        .baseMipLevel = 0,
                        .levelCount = 1,
                        .baseArrayLayer = 0,
                        .layerCount = 1,
                    });

                // prepare to start dynamic rendering
                //cmdBuffer.clearColorImage(swapchainImage, vk::ImageLayout::eTransferDstOptimal, clearColor, imageRange);
                vk::ClearValue clearColor = vk::ClearColorValue(backgroundColor.x, backgroundColor.y, backgroundColor.z,
                                                                1.0f);
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
                        .offset = {.x = 0, .y = 0},
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
                pCmd->beginRendering(renderingInfo);

                // bind pipeline
                m_pOpaqueRasterPipeline->bind(*pCmd);

                pCmd->setViewport(0, vk::Viewport(0.0f, 0.0f, static_cast<float>(swapchainExtent.width),
                                                      static_cast<float>(swapchainExtent.height), 0.0f, 1.0f));
                pCmd->setScissor(0, vk::Rect2D(vk::Offset2D(0, 0), swapchainExtent));

                pCmd->bindDescriptorSets(vk::PipelineBindPoint::eGraphics,
                                             m_pOpaqueRasterPipeline->getLayout(),
                                             0,
                                             *m_textureDescriptorSet,
                                             nullptr);

                struct NodeData {
                    int materialID;
                    int renderNodeID;
                    int renderPrimID;
                };

                // start drawing
                const auto &rnodes = m_pScene->getRenderNodes();
                uint32_t offset = offsetof(shaderio::RasterPushConstant, materialID);

                for (int nodeID = 0; nodeID < rnodes.size(); nodeID++) {
                    const auto &rnode = rnodes[nodeID];
                    const RenderPrimitive &subMesh = m_pScene->getRenderPrimitive(rnode.renderPrimID);
                    NodeData pushConst = {
                        .materialID = rnode.materialID,
                        .renderNodeID = nodeID,
                        .renderPrimID = rnode.renderPrimID
                    };
                    pCmd->pushConstants<NodeData>(m_pOpaqueRasterPipeline->getLayout(),
                                                      vk::ShaderStageFlagBits::eAllGraphics, offset, pushConst);

                    pCmd->bindVertexBuffers(0, m_pSceneVk->getVertexBuffers()[rnode.renderPrimID].buffer, {0});
                    pCmd->bindIndexBuffer(m_pSceneVk->getIndexBuffers()[rnode.renderPrimID].buffer, 0,
                                              vk::IndexType::eUint32);
                    pCmd->drawIndexed(subMesh.indexCount, 1, 0, 0, 0);
                }

                pCmd->endRendering();
            }
            else if (currentPipeline == eRaytracing) {
                ptvk::utils::imageLayoutTransition( // last usage shader read from post processing
                    *pCmd,
                    drawImage.image,
                    vk::PipelineStageFlagBits2::eFragmentShader,
                    vk::PipelineStageFlagBits2::eRayTracingShaderKHR,
                    vk::AccessFlagBits2::eShaderRead,
                    vk::AccessFlagBits2::eShaderWrite,
                    vk::ImageLayout::eShaderReadOnlyOptimal,
                    vk::ImageLayout::eGeneral,
                    subresourceRange);

                m_pRtPipeline->bind(*pCmd);
                // bind texture descriptor set
                pCmd->bindDescriptorSets(vk::PipelineBindPoint::eRayTracingKHR,
                                             m_pRtPipeline->getLayout(),
                                             0,
                                             *m_textureDescriptorSet,
                                             nullptr);

                pushRtDescriptors(*pCmd);

                const vk::Extent2D extent = m_vkCore.getSwapchain().GetExtent();
                pCmd->traceRaysKHR(m_pRtPipeline->raygenRegion, m_pRtPipeline->missRegion, m_pRtPipeline->hitRegion,
                                       m_pRtPipeline->callableRegion, extent.width, extent.height, 1);

                if (enableDenoise) {
                    m_vkCore.submitSingleTimeCommandBuffer(*pCmd); // submit rendering commands early and wait for completion with a fence

                    auto cmd = m_vkCore.beginSingleTimeCommandBuffer(); // single time cmd buffer for copy ops
                    // transition denoise attachments to transfer src
                    ptvk::utils::imageLayoutTransition(cmd,
                                                       accImage.image,
                                                       vk::PipelineStageFlagBits2::eRayTracingShaderKHR,
                                                       vk::PipelineStageFlagBits2::eTransfer,
                                                       vk::AccessFlagBits2::eShaderWrite,
                                                       vk::AccessFlagBits2::eTransferRead,
                                                       vk::ImageLayout::eGeneral,
                                                       vk::ImageLayout::eTransferSrcOptimal,
                                                       subresourceRange);

                    ptvk::utils::imageLayoutTransition(cmd,
                                                       albedoImage.image,
                                                       vk::PipelineStageFlagBits2::eRayTracingShaderKHR,
                                                       vk::PipelineStageFlagBits2::eTransfer,
                                                       vk::AccessFlagBits2::eShaderWrite,
                                                       vk::AccessFlagBits2::eTransferRead,
                                                       vk::ImageLayout::eGeneral,
                                                       vk::ImageLayout::eTransferSrcOptimal,
                                                       subresourceRange);

                    ptvk::utils::imageLayoutTransition(cmd,
                                                       normalImage.image,
                                                       vk::PipelineStageFlagBits2::eRayTracingShaderKHR,
                                                       vk::PipelineStageFlagBits2::eTransfer,
                                                       vk::AccessFlagBits2::eShaderWrite,
                                                       vk::AccessFlagBits2::eTransferRead,
                                                       vk::ImageLayout::eGeneral,
                                                       vk::ImageLayout::eTransferSrcOptimal,
                                                       subresourceRange);

                    // copy output images to vulkan buffers
                    m_pDenoiser->cmdCopyImageToBuffer(cmd, accImage, shaderio::OutputImage::eResult);
                    m_pDenoiser->cmdCopyImageToBuffer(cmd, albedoImage, shaderio::OutputImage::eAlbedo);
                    m_pDenoiser->cmdCopyImageToBuffer(cmd, normalImage, shaderio::OutputImage::eNormal);
                    m_vkCore.submitSingleTimeCommandBuffer(cmd);
                    m_pDenoiser->synchronousDenoise();

                    // upload denoised image to draw image
                    cmd = m_vkCore.beginSingleTimeCommandBuffer();
                    m_pStaging->appendImage(drawImage, m_pDenoiser->output(), m_pDenoiser->outputSize(), vk::ImageLayout::eGeneral, vk::ImageLayout::eGeneral);
                    m_pStaging->uploadAppendedCmd(cmd);
                    ptvk::utils::imageLayoutTransition(cmd,
                                                       accImage.image,
                                                       vk::PipelineStageFlagBits2::eTransfer,
                                                       vk::PipelineStageFlagBits2::eRayTracingShaderKHR,
                                                       vk::AccessFlagBits2::eTransferRead,
                                                       vk::AccessFlagBits2::eShaderWrite,
                                                       vk::ImageLayout::eTransferSrcOptimal,
                                                       vk::ImageLayout::eGeneral,
                                                       subresourceRange);

                    ptvk::utils::imageLayoutTransition(cmd,
                                                       albedoImage.image,
                                                       vk::PipelineStageFlagBits2::eTransfer,
                                                       vk::PipelineStageFlagBits2::eRayTracingShaderKHR,
                                                       vk::AccessFlagBits2::eTransferRead,
                                                       vk::AccessFlagBits2::eShaderWrite,
                                                       vk::ImageLayout::eTransferSrcOptimal,
                                                       vk::ImageLayout::eGeneral,
                                                       subresourceRange);

                    ptvk::utils::imageLayoutTransition(cmd,
                                                       normalImage.image,
                                                       vk::PipelineStageFlagBits2::eTransfer,
                                                       vk::PipelineStageFlagBits2::eRayTracingShaderKHR,
                                                       vk::AccessFlagBits2::eTransferRead,
                                                       vk::AccessFlagBits2::eShaderWrite,
                                                       vk::ImageLayout::eTransferSrcOptimal,
                                                       vk::ImageLayout::eGeneral,
                                                       subresourceRange);

                    m_vkCore.submitSingleTimeCommandBuffer(cmd);
                    m_pStaging->releaseStaging();

                    pCmd = &m_vkCore.beginCommandRecording(); // reset pool and start recording again
                }
            }
        }

        /////
        // Postprocessing pass
        /////
        // postprocessing pass, draw image input -> swapchain image color attachment
        {
            // transition draw image to shader read
            ptvk::utils::imageLayoutTransition(*pCmd,
                                               drawImage.image,
                                               vk::PipelineStageFlagBits2::eRayTracingShaderKHR |
                                               vk::PipelineStageFlagBits2::eColorAttachmentOutput,
                                               vk::PipelineStageFlagBits2::eFragmentShader,
                                               vk::AccessFlagBits2::eShaderWrite |
                                               vk::AccessFlagBits2::eColorAttachmentWrite,
                                               vk::AccessFlagBits2::eShaderRead,
                                               currentPipeline == eRaster ? vk::ImageLayout::eColorAttachmentOptimal : vk::ImageLayout::eGeneral,
                                               vk::ImageLayout::eShaderReadOnlyOptimal,
                                               subresourceRange);

            // prepare to start dynamic rendering
            vk::ClearValue clearColor = vk::ClearColorValue(.0f, .0f, .0f, 1.0f);
            vk::RenderingAttachmentInfo colorAttachmentInfo = {
                .imageView = swapchainImageView,
                .imageLayout = vk::ImageLayout::eColorAttachmentOptimal,
                .loadOp = vk::AttachmentLoadOp::eClear,
                .storeOp = vk::AttachmentStoreOp::eStore,
                .clearValue = clearColor,
            };
            vk::RenderingInfo renderingInfo = {
                .renderArea = {
                    .offset = {.x = 0, .y = 0},
                    .extent = {
                        .width = swapchainExtent.width,
                        .height = swapchainExtent.height
                    }
                },
                .layerCount = 1,
                .colorAttachmentCount = 1,
                .pColorAttachments = &colorAttachmentInfo,
            };

            // begin dynamic rendering
            pCmd->beginRendering(renderingInfo);

            pCmd->pushConstants<shaderio::RasterPushConstant>(
                m_pPostprocessPipeline->getLayout(),
                vk::ShaderStageFlagBits::eAllGraphics,
                0,
                m_rasterPushConstant);

            m_pPostprocessPipeline->bind(*pCmd);
            pCmd->setViewport(0, vk::Viewport(0.0f, 0.0f, static_cast<float>(swapchainExtent.width), static_cast<float>(swapchainExtent.height), 0.0f, 1.0f));
            pCmd->setScissor(0, vk::Rect2D(vk::Offset2D(0, 0), swapchainExtent));
            pushPPDescriptors(*pCmd);
            pCmd->draw(3, 1, 0, 0);

            pCmd->endRendering();
        }
    }
    // else only render UI

    /////
    // Begin UI pass
    /////
    {
        vk::ClearValue clearColor = vk::ClearColorValue(backgroundColor.x, backgroundColor.y, backgroundColor.z, 1.0f);
        vk::RenderingAttachmentInfo colorAttachmentInfo = {
            .imageView = swapchainImageView,
            .imageLayout = vk::ImageLayout::eColorAttachmentOptimal,
            .loadOp = m_pSceneVk ? vk::AttachmentLoadOp::eLoad : vk::AttachmentLoadOp::eClear,
            .storeOp = vk::AttachmentStoreOp::eStore,
            .clearValue = clearColor
        };

        vk::RenderingInfo renderingInfo = {
            .renderArea = {
                .offset = {.x = 0, .y = 0},
                .extent = {
                    .width = swapchainExtent.width,
                    .height = swapchainExtent.height
                }
            },
            .layerCount = 1,
            .colorAttachmentCount = 1,
            .pColorAttachments = &colorAttachmentInfo,
        };

        pCmd->beginRendering(renderingInfo);
        ImGui_ImplVulkan_NewFrame();
        ImGui_ImplVulkan_RenderDrawData(ImGui::GetDrawData(), *(*pCmd));
        pCmd->endRendering();
    }

    // transition swapchain image for presentation
    ptvk::utils::imageLayoutTransition(
        *pCmd,
        swapchainImage,
        vk::ImageLayout::eColorAttachmentOptimal,
        vk::ImageLayout::ePresentSrcKHR);

    m_vkCore.submitFrame();
    m_vkCore.presentFrame();
}

void Renderer::onResize(int width, int height) {
    m_vkCore.resizeRequested = true;
    m_pDenoiser = std::make_unique<ptvk::OidnDenoiser>(width, height, m_vkCore.getResourceAllocator());
}

bool Renderer::createScene(const std::filesystem::path &filename) {
    m_pScene = std::make_unique<GltfScene>();

    if (!m_pScene->load(filename)) {
        return false;
    }

    m_pSceneVk = std::make_unique<ptvk::GltfSceneVk>(m_vkCore.getResourceAllocator(), *m_pSamplerPool, true);
    m_pSceneRt = std::make_unique<ptvk::GltfSceneRt>(m_vkCore.getResourceAllocator(), m_vkCore.getDevice());

    // create vulk resources for loaded scene

    auto cmd = m_vkCore.beginSingleTimeCommandBuffer();

    m_pSceneVk->createVkResources(cmd, *m_pStaging, *m_pScene);

    m_vkCore.submitSingleTimeCommandBuffer(cmd);
    m_pStaging->releaseStaging(); // release staging resources

    //cmd = m_vkCore.beginSingleTimeCommandBuffer();
    //m_pRtScene->create(cmd, m_scene, *m_pVkScene, vk::BuildAccelerationStructureFlagBitsKHR::ePreferFastTrace);

    // populate texture and sampler descriptor set

    int imgCount = static_cast<int>(m_pSceneVk->getTextureCount());
    if (static_cast<uint32_t>(imgCount) > m_maxTextures) {
        std::cout << std::format("Scene requires {} textures but descriptor set supports {}\n", imgCount,
                                 m_maxTextures);
        return false;
    }

    int samplerCount = static_cast<int>(m_pSceneVk->getSamplerCount());
    if (static_cast<uint32_t>(samplerCount) > m_maxSamplers) {
        std::cout << std::format("Scene requires {} samplers but descriptor set supports {}\n", samplerCount,
                                 m_maxSamplers);
        return false;
    }

    std::vector<vk::WriteDescriptorSet> writes{};

    // write textures into descriptor
    std::vector<vk::DescriptorImageInfo> textureWriteInfos;
    for (int i = 0; i < imgCount; i++) {
        const auto &img = m_pSceneVk->getTextureImage(i);
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
    for (int i = 0; i < samplerCount; i++) {
        const auto &sampler = m_pSceneVk->getSampler(i);
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

    createAccelerationStructures();
    return true;
}

void Renderer::changeScene(const std::filesystem::path &filename) {
    m_vkCore.deviceWaitIdle();
    resetAccumulation();
    if (!createScene(filename)) {
        cleanupScene();
    }
}

void Renderer::changePipeline(PipelineType type) {
    m_vkCore.deviceWaitIdle();
    resetAccumulation();
    currentPipeline = type;
}

void Renderer::cleanupScene() {
    m_vkCore.deviceWaitIdle();
    // TODO clean up ui related to scene
    m_pScene.reset();
    m_pSceneVk.reset();
    m_pSceneRt.reset();
}

void Renderer::loadShaders() {
    m_pRasterShader = std::make_unique<ptvk::Shader>(m_vkCore.getDevice().getVkDevice(), "raster.spv");
    m_pRtShader = std::make_unique<ptvk::Shader>(m_vkCore.getDevice().getVkDevice(), "pathtrace.spv");
    m_pTonemapShader = std::make_unique<ptvk::Shader>(m_vkCore.getDevice().getVkDevice(), "tonemap.spv");
    printf("[INFO] Shaders Loaded\n");
}

void Renderer::createDescriptors() {
    auto &device = m_vkCore.getDevice().getVkDevice();

    auto &deviceProperties = m_vkCore.getDevice().getPhysicalDevice().m_devProperties2.properties;
    m_maxTextures = std::min(m_maxTextures, deviceProperties.limits.maxDescriptorSetSampledImages - 1);
    m_maxSamplers = std::min(m_maxTextures, deviceProperties.limits.maxDescriptorSetSamplers - 1);

    // descriptor set for textures and samplers
    // no need for update flags since we only update textures and samplers on scene load and not during rendering (no add/remove ops, currently)
    vk::DescriptorBindingFlags descVariableFlags[2]{
        {
            // textures
            vk::DescriptorBindingFlagBits::ePartiallyBound
        },
        {
            // samplers
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

    // allocate the actual descriptor sets
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
        4,
        vk::ShaderStageFlagBits::eAll);

    vk::DescriptorSetLayoutCreateInfo rtLayoutInfo = {
        .flags = vk::DescriptorSetLayoutCreateFlagBits::ePushDescriptor,
        .bindingCount = static_cast<uint32_t>(m_rtDescriptorBindings.size()),
        .pBindings = m_rtDescriptorBindings.data()
    };
    m_rtDescriptorLayout = device.createDescriptorSetLayout(rtLayoutInfo);

    // postprocessing descriptor layout
    m_postprocessDescriptorBindings[0] = vk::DescriptorSetLayoutBinding(
        shaderio::DescriptorBindingPoints::eOutImage,
        vk::DescriptorType::eSampledImage,
        1,
        vk::ShaderStageFlagBits::eAll);

    vk::DescriptorSetLayoutCreateInfo ppDescLayoutInfo = {
        .flags = vk::DescriptorSetLayoutCreateFlagBits::ePushDescriptor,
        .bindingCount = static_cast<uint32_t>(m_postprocessDescriptorBindings.size()),
        .pBindings = m_postprocessDescriptorBindings.data()
    };
    m_postprocessDescriptorLayout = device.createDescriptorSetLayout(ppDescLayoutInfo);
}

void Renderer::pushRtDescriptors(const vk::raii::CommandBuffer &cmd) {
    std::array<vk::WriteDescriptorSet, 2> writes{};

    // TLAS
    vk::WriteDescriptorSetAccelerationStructureKHR descAccel{
        .accelerationStructureCount = 1,
        .pAccelerationStructures = &*m_pSceneRt->getTlas()
    };

    writes[0] = vk::WriteDescriptorSet{
        .pNext = &descAccel,
        .dstBinding = shaderio::DescriptorBindingPoints::eTlas,
        .dstArrayElement = 0,
        .descriptorCount = 1,
        .descriptorType = vk::DescriptorType::eAccelerationStructureKHR,
    };

    // Output image
    std::vector<vk::DescriptorImageInfo> outImages{
        {
            .imageView = m_vkCore.getDrawImage().view,
            .imageLayout = vk::ImageLayout::eGeneral
        },
        {
            .imageView = m_vkCore.getAlbedoImage().view,
            .imageLayout = vk::ImageLayout::eGeneral
        },
        {
            .imageView = m_vkCore.getNormalImage().view,
            .imageLayout = vk::ImageLayout::eGeneral
        },
        {
            .imageView = m_vkCore.getAccumulationImage().view,
            .imageLayout = vk::ImageLayout::eGeneral
        }
    };

    writes[1] = vk::WriteDescriptorSet{
        .dstBinding = shaderio::DescriptorBindingPoints::eOutImage,
        .dstArrayElement = 0,
        .descriptorCount = 4,
        .descriptorType = vk::DescriptorType::eStorageImage,
        .pImageInfo = outImages.data(),
    };

    cmd.pushDescriptorSetKHR(vk::PipelineBindPoint::eRayTracingKHR, m_pRtPipeline->getLayout(), 1, writes);
}

void Renderer::pushPPDescriptors(const vk::raii::CommandBuffer &cmd) {
    std::array<vk::WriteDescriptorSet, 1> writes{};
    vk::DescriptorImageInfo hdrImageDescriptor{
        .imageView = m_vkCore.getDrawImage().view,
        .imageLayout = vk::ImageLayout::eShaderReadOnlyOptimal
    };
    writes[0] = vk::WriteDescriptorSet{
        .dstBinding = shaderio::DescriptorBindingPoints::eOutImage,
        .dstArrayElement = 0,
        .descriptorCount = 1,
        .descriptorType = vk::DescriptorType::eSampledImage,
        .pImageInfo = &hdrImageDescriptor,
    };
    cmd.pushDescriptorSetKHR(vk::PipelineBindPoint::eGraphics, m_pPostprocessPipeline->getLayout(), 0, writes);
}

void Renderer::createFrameDataBuffers() {
    // create frame data buffer
    vk::BufferCreateInfo bufferInfo = {
        .size = sizeof(shaderio::FrameData),
        .usage = vk::BufferUsageFlagBits::eUniformBuffer | vk::BufferUsageFlagBits::eShaderDeviceAddress |
                 vk::BufferUsageFlagBits::eTransferDst
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
    std::vector<vk::DescriptorSetLayout> rasterDescLayouts{m_textureDescriptorLayout};
    m_pOpaqueRasterPipeline = std::make_unique<ptvk::GraphicsPipeline>(m_vkCore.getDevice().getVkDevice(),
                                                                       *m_pRasterShader, 1, colorFormat, depthFormat,
                                                                       enableDepth, false, true, rasterDescLayouts);

    std::vector<vk::DescriptorSetLayout> postProcessDescLayouts{m_postprocessDescriptorLayout};
    m_pPostprocessPipeline = std::make_unique<ptvk::GraphicsPipeline>(m_vkCore.getDevice().getVkDevice(),
                                                                      *m_pTonemapShader, 1,
                                                                      m_vkCore.getSwapchain().GetSwapchainFormat(),
                                                                      depthFormat, false, false, false,
                                                                      postProcessDescLayouts);
}

void Renderer::createAccelerationStructures() {
    m_pSceneRt->createBLAS(m_vkCore, *m_pScene, *m_pSceneVk,
                           enableDebugging
                               ? vk::BuildAccelerationStructureFlagBitsKHR::ePreferFastBuild
                               : vk::BuildAccelerationStructureFlagBitsKHR::ePreferFastTrace);
    std::cout << "[INFO] All Mesh Blas created and built" << std::endl;

    vk::raii::CommandBuffer cmd = m_vkCore.beginSingleTimeCommandBuffer();
    m_pSceneRt->createTLAS(cmd, *m_pScene);
    m_vkCore.submitSingleTimeCommandBuffer(cmd);
    std::cout << "[INFO] Tlas created and built" << std::endl;
}

void Renderer::createRtPipeline() {
    std::vector<vk::DescriptorSetLayout> layouts{m_textureDescriptorLayout, m_rtDescriptorLayout};
    m_pRtPipeline = std::make_unique<ptvk::RtPipeline>(m_vkCore.getDevice(), m_vkCore.getResourceAllocator(),
                                                       *m_pRtShader, layouts);
}

// should be called whenever the scene or renderer configuration is changed
void Renderer::resetAccumulation() {
    m_frameCount = -1;
    m_totalSamples = 0;
}

void Renderer::prepareFrameData(const vk::raii::CommandBuffer &cmd) {
    int frame = static_cast<int>(m_vkCore.getCurrentFrameIndex());
    // sync scene changes with gpu
    bool changed = false;
    changed |= m_pScene->updateNodeWorldMatrices(); // updates render nodes that were modified
    changed |= m_pSceneVk->updateFromScene(*m_pScene, frame);
    changed |= m_pSceneRt->syncTLAS(cmd, *m_pScene);
    changed |= m_camera.changed;
    changed |= m_configChanged;
    m_camera.changed = false;
    m_configChanged = false;
    if (changed) {
        resetAccumulation();
    }
    m_frameCount++;

    // update frame data buffer
    auto [width, height, depth] = m_vkCore.getDrawImage().extent;
    glm::mat4x4 projMat = glm::perspective(glm::radians(m_camera.fov),
                                           static_cast<float>(width) / static_cast<float>(height), 0.01f, 1000.0f);
    projMat[1][1] *= -1; // flip y
    shaderio::FrameData frameData = {
        .viewProjMatrix = projMat * m_camera.getViewMatrix(),
        .projInvMatrix = glm::inverse(projMat),
        .viewInvMatrix = glm::inverse(m_camera.getViewMatrix()),
        .cameraPosition = glm::vec4(m_camera.position, 0),
        .exposureEV = m_camera.exposureEV,
        .backgroundColor = backgroundColor,
        .lightDir = glm::vec3(0, -1, 0) * glm::quat(glm::radians(lightRotationEuler)),
        .useLight = enableDirLight,
        .lightColor = glm::vec4(lightColor, lightIntensity),
    };
    cmd.updateBuffer<shaderio::FrameData>(m_bFrameData[frame].buffer, 0, frameData);

    m_rasterPushConstant.frameData = reinterpret_cast<shaderio::FrameData *>(m_bFrameData[frame].address);
    m_rasterPushConstant.sceneInfo = reinterpret_cast<shaderio::GltfSceneInfo *>(m_pSceneVk->getSceneInfo(frame).address);

    // update push constant
    if (currentPipeline == eRaster) {
        cmd.pushConstants<shaderio::RasterPushConstant>(
            m_pOpaqueRasterPipeline->getLayout(),
            vk::ShaderStageFlagBits::eAllGraphics,
            0,
            m_rasterPushConstant);
    } else {
        // rt
        m_rtPushConstant.frameData = reinterpret_cast<shaderio::FrameData *>(m_bFrameData[frame].address);
        m_rtPushConstant.sceneInfo = reinterpret_cast<shaderio::GltfSceneInfo *>(m_pSceneVk->getSceneInfo(frame).
            address);
        m_rtPushConstant.maxDepth = bounces;
        m_rtPushConstant.maxSamples = maxSamples;
        m_rtPushConstant.numSamples = samplesPerPixel;
        m_rtPushConstant.totalSamples = m_totalSamples;
        if (maxSamples == 0 || m_totalSamples < maxSamples) {
            m_totalSamples += m_rtPushConstant.numSamples;
        }

        cmd.pushConstants<shaderio::RtPushConstant>(
            m_pRtPipeline->getLayout(),
            vk::ShaderStageFlagBits::eAll,
            0,
            m_rtPushConstant
        );
    }

    ptvk::utils::cmdMemoryBarrier(cmd,
                                  vk::PipelineStageFlagBits2::eTransfer,
                                  vk::PipelineStageFlagBits2::eRayTracingShaderKHR |
                                  vk::PipelineStageFlagBits2::eAllGraphics,
                                  vk::AccessFlagBits2::eTransferWrite,
                                  vk::AccessFlagBits2::eShaderRead);
}

void Renderer::updateGUI() {
    ImGui_ImplVulkan_NewFrame();
    ImGui_ImplGlfw_NewFrame();

    int frame = m_vkCore.getCurrentFrameIndex();

    ImGui::NewFrame();
    if (m_showGUI) {
        ImGui::DockSpaceOverViewport(0, ImGui::GetMainViewport(), ImGuiDockNodeFlags_PassthruCentralNode);
        if (ImGui::Begin("Renderer Settings", &m_showGUI, ImGuiWindowFlags_AlwaysAutoResize)) {
            const char *pipelineNames[] = {
                "Rasterization",
                "Raytracing"
            };
            constexpr PipelineType pipelineTypes[] = {
                PipelineType::eRaster,
                PipelineType::eRaytracing
            };
            static int selected = 1;
            if (ImGui::BeginCombo("Rendering Pipeline", pipelineNames[selected])) {
                for (int i = 0; i < IM_ARRAYSIZE(pipelineNames); ++i) {
                    const bool isSelected = (selected == i);

                    if (ImGui::Selectable(pipelineNames[i], isSelected)) {
                        selected = i;
                        changePipeline(pipelineTypes[i]);
                    }

                    if (isSelected)
                        ImGui::SetItemDefaultFocus();
                }
                ImGui::EndCombo();
            }

            if (ImGui::CollapsingHeader("Performance", ImGuiTreeNodeFlags_DefaultOpen)) {
                float fps = ImGui::GetIO().Framerate;
                ImGui::Text("FPS: %.1f", fps);
                ImGui::Text("Frametime: %.2f ms", 1000.0f / fps);
                ImGui::Text("Frame Count: %d", m_frameCount);
                ImGui::Text("Total Samples Accumulated: %d ", m_totalSamples);
            }

            if (ImGui::CollapsingHeader("Raytracing Options", ImGuiTreeNodeFlags_DefaultOpen)) {
                if (ImGui::SliderInt("Samples per Pixel (spp)", &samplesPerPixel, 1, 50)) {
                    resetAccumulation();
                }
                if (ImGui::SliderInt("Max Ray Bounces", &bounces, 0, 12)) {
                    resetAccumulation();
                }
                ImGui::SliderInt("Max Samples Accumulated", &maxSamples, 0, 3000);
                ImGui::Checkbox("Enable OIDN CPU Denoising", &enableDenoise);
            }
        }
        ImGui::End();

        if (ImGui::Begin("Inspector", &m_showGUI, ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_MenuBar)) {
            if (ImGui::BeginMenuBar()) {
                if (ImGui::BeginMenu("Files")) {
                    if (ImGui::MenuItem("Load Scene...")) {
                        nfdu8char_t *path = nullptr;

                        nfdu8filteritem_t filters[] = {
                            {.name = "glTF models", .spec = "gltf,glb"}
                        };

                        nfdresult_t result = NFD_OpenDialogU8(
                            &path,
                            filters, // filters
                            1, // filter count
                            reinterpret_cast<const nfdu8char_t *>(std::filesystem::current_path().u8string().c_str())
                            // default path
                        );

                        if (result == NFD_OKAY) {
                            if (!createScene(path)) {
                                cleanupScene();
                            }
                            resetAccumulation();
                            NFD_FreePathU8(path);
                        }
                    }
                    ImGui::EndMenu();
                }
                ImGui::EndMenuBar();
            }

            if (ImGui::CollapsingHeader("Camera")) {
                if (ImGui::SliderFloat("Camera vFOV", &m_camera.fov, 10.0f, 90.0f)) {
                    resetAccumulation();
                }
                if (ImGui::SliderInt("Camera Exposure EV", &m_camera.exposureEV, -5, 5)) {
                    resetAccumulation();
                }
                ImGui::SliderFloat("Camera Speed", &m_camera.speed, 0.1f, 100.0f);
                ImGui::DragFloat("Camera Sensitivity", &m_camera.sensitivity, 0.001f, 0.0f, 10.0f);
            }

            ImVec4 color(backgroundColor.r, backgroundColor.g, backgroundColor.b, 1.0f);
            ImGui::Text("Background Color:");
            if (ImGui::ColorEdit3("Background Color", reinterpret_cast<float *>(&color), ImGuiColorEditFlags_Float)) {
                backgroundColor = glm::vec3(color.x, color.y, color.z);
                resetAccumulation();
            }

            if (m_pSceneVk && ImGui::CollapsingHeader("Scene")) {
                auto &model = m_pScene->getModel();
                if (ImGui::CollapsingHeader("Nodes")) {
                    ImGui::PushID("Nodes");
                    for (int i = 0; i < model.nodes.size(); i++) {
                        auto& node = model.nodes[i];
                        glm::vec3 translation;
                        glm::quat q;
                        glm::vec3 scale;
                        gltfutils::getNodeTRS(node, translation, q, scale);

                        glm::vec3 rotation = glm::degrees(glm::eulerAngles(q));

                        ImGui::PushID(i);

                        if(ImGui::TreeNode(node.name.c_str())) {
                            // Position
                            ImGui::Separator();
                            ImGui::Text("Position");

                            bool changed = false;

                            ImGui::SetNextItemWidth(50);
                            changed |= ImGui::DragFloat("X##pos", &translation.x, 0.01f);

                            ImGui::SameLine();
                            ImGui::SetNextItemWidth(50);
                            changed |= ImGui::DragFloat("Y##pos", &translation.y, 0.01f);

                            ImGui::SameLine();
                            ImGui::SetNextItemWidth(50);
                            changed |= ImGui::DragFloat("Z##pos", &translation.z, 0.01f);

                            // Rotation
                            ImGui::Separator();
                            ImGui::Text("Rotation (degrees)");

                            ImGui::SetNextItemWidth(50);
                            changed |= ImGui::DragFloat("X##rot", &rotation.x, 0.1f, -180.0f, 180.0f);

                            ImGui::SameLine();
                            ImGui::SetNextItemWidth(50);
                            changed |= ImGui::DragFloat("Y##rot", &rotation.y, 0.1f, -180.0f, 180.0f);

                            ImGui::SameLine();
                            ImGui::SetNextItemWidth(50);
                            changed |= ImGui::DragFloat("Z##rot", &rotation.z, 0.1f, -180.0f, 180.0f);

                            // Scale
                            ImGui::Separator();
                            ImGui::Text("Scale");

                            ImGui::SetNextItemWidth(50);
                            changed |= ImGui::DragFloat("X##scale", &scale.x, 0.01f);

                            ImGui::SameLine();
                            ImGui::SetNextItemWidth(50);
                            changed |= ImGui::DragFloat("Y##scale", &scale.y, 0.01f);

                            ImGui::SameLine();
                            ImGui::SetNextItemWidth(50);
                            changed |= ImGui::DragFloat("Z##scale", &scale.z, 0.01f);

                            // Apply changes
                            if (changed) {
                                m_pScene->setNodeTRS(i, translation, glm::radians(rotation), scale, frame);
                                resetAccumulation();
                            }
                            ImGui::TreePop();
                        }

                        ImGui::Separator();
                        ImGui::PopID();
                    }
                    ImGui::PopID();
                }

                if (ImGui::CollapsingHeader("Lights")) {
                    ImGui::PushID("Lights");
                    for (int i = 0; i < model.lights.size(); i++) {
                        auto& light = model.lights[i];
                        ImGui::PushID(i);

                        if (ImGui::TreeNode(light.name.c_str())) {
                            bool changed = false;

                            // Color
                            ImGui::Separator();
                            ImGui::Text("Color");

                            float color[3] = {
                                static_cast<float>(light.color[0]),
                                static_cast<float>(light.color[1]),
                                static_cast<float>(light.color[2])
                            };

                            if (ImGui::ColorEdit3("Color", color)) {
                                light.color[0] = color[0];
                                light.color[1] = color[1];
                                light.color[2] = color[2];
                                changed = true;
                            }

                            // Range
                            ImGui::Separator();
                            ImGui::Text("Range");

                            float range = static_cast<float>(light.range);

                            if (ImGui::DragFloat("Range", &range, 0.1f, 0.01f, 10000.0f)){
                                light.range = static_cast<double>(range);
                                changed = true;
                            }

                            // Intensity
                            ImGui::Separator();
                            ImGui::Text("Intensity");
                            float intensity = static_cast<float>(light.intensity);
                            if(ImGui::DragFloat("Intensity", &intensity, 0.1f, 0.0f, 100000.0f)) {
                                light.intensity = static_cast<double>(intensity);
                                changed = true;
                            }

                            // Type
                            ImGui::Separator();
                            ImGui::Text("Type");

                            const char* lightTypes[] = {
                                "directional",
                                "point",
                                "spot"
                            };

                            int type;
                            if (light.type == "directional")
                                type = 0;
                            else if (light.type == "point")
                                type = 1;
                            else if (light.type == "spot")
                                type = 2;

                            if (ImGui::Combo("Type", &type, lightTypes, IM_ARRAYSIZE(lightTypes))) {
                                switch (type) {
                                    case 0: light.type = "directional";
                                        break;
                                    case 1: light.type = "point";
                                        break;
                                    case 2: light.type = "spot";
                                        break;
                                    default:
                                        break;
                                }
                                changed = true;
                            }

                            float innerConeAngle = glm::degrees(static_cast<float>(light.spot.innerConeAngle));
                            float outerConeAngle = glm::degrees(static_cast<float>(light.spot.outerConeAngle));

                            if (light.type == "spot" && ImGui::DragFloat("Inner Cone Size (degrees)", &innerConeAngle, 0.01, 0, 360)) {
                                light.spot.innerConeAngle = glm::radians(static_cast<double>(innerConeAngle));
                                changed = true;
                            }

                            if (light.type == "spot" && ImGui::DragFloat("Outer Cone Size (degrees)", &outerConeAngle, 0.01, 0, 360)) {
                                light.spot.outerConeAngle = glm::radians(static_cast<double>(outerConeAngle));
                                changed = true;
                            }

                            if (changed) {
                                //std::cout << i << " changed" << std::endl;
                                m_pScene->markLightDirty(i);
                                resetAccumulation();
                            }
                            ImGui::TreePop();
                        }
                        ImGui::Separator();
                        ImGui::PopID();
                    }
                    ImGui::PopID();
                }

                if (ImGui::CollapsingHeader("Sunlight")) {
                    if (ImGui::Checkbox("Enable Sunlight", &enableDirLight)) {
                        resetAccumulation();
                    }

                    ImGui::Text("Rotation");
                    ImGui::Separator();

                    ImGui::SetNextItemWidth(50);
                    if (ImGui::DragFloat("X ", &lightRotationEuler.x, 0.1f, -180.0f, 180.0f)) {
                        resetAccumulation();
                    }
                    ImGui::SameLine();
                    ImGui::SetNextItemWidth(50);
                    if (ImGui::DragFloat("Y ", &lightRotationEuler.y, 0.1f, -180.0f, 180.0f)) {
                        resetAccumulation();
                    }
                    ImGui::SameLine();
                    ImGui::SetNextItemWidth(50);
                    if (ImGui::DragFloat("Z ", &lightRotationEuler.z, 0.1f, -180.0f, 180.0f)) {
                        resetAccumulation();
                    }

                    ImGui::Text("Intensity");
                    ImGui::Separator();
                    if (ImGui::DragFloat("##1", &lightIntensity, 0.1f, 0, 20.0f)) {
                        resetAccumulation();
                    }

                    ImGui::Text("Color");
                    ImGui::Separator();
                    if (ImGui::ColorPicker3("##2", glm::value_ptr(lightColor))) {
                        resetAccumulation();
                    }
                }
            }
        }
        ImGui::End();
    }

    ImGui::Render();
}

}
