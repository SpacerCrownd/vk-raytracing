#include "GltfSceneVulkan.h"
#include "Utils.h"
#include "../shaders/gltfio.h.slang"

namespace ptvk {
GltfSceneVulkan::GltfSceneVulkan(const ResourceAllocator &allocator,
                                 const SamplerPool &samplerPool,
                                 bool generateMipmaps) : m_allocator(allocator),
                                                         m_samplerPool(samplerPool),
                                                         m_generateMipmaps(generateMipmaps)
{}

void GltfSceneVulkan::createVkResources(vk::raii::CommandBuffer &cmd, app::GltfScene &scene) {
    uploadTextureImages(cmd, scene);
    createTextures(cmd, scene);
}

void GltfSceneVulkan::updateFromScene(vk::raii::CommandBuffer &cmd, app::GltfScene &scene) {

}

void GltfSceneVulkan::destroy() {
    m_bMaterials = {};
    m_bLights = {};
    m_bRenderNodes = {};
    m_bRenderPrimitives = {};
    m_bSceneInfo = {};

    m_vertexBuffers.clear();
    m_bIndices.clear();
    m_images.clear();
}

void GltfSceneVulkan::uploadTextureImages(vk::raii::CommandBuffer &cmd, app::GltfScene &scene) {
    const auto& model = scene.getModel();
    // load all images in the scene
    for (size_t i = 0; i < model.images.size(); i++) {
        auto& image = model.images[i];

        vk::ImageCreateInfo imageInfo {
            .imageType = vk::ImageType::e2D,
            .format = vk::Format::eR8G8B8A8Srgb,
            .extent = vk::Extent3D{ static_cast<uint32_t>(image.width),static_cast<uint32_t>(image.height), 1 },
            .mipLevels = 1,
            .arrayLayers = 1,
            .samples = vk::SampleCountFlagBits::e1,
            .tiling = vk::ImageTiling::eOptimal,
            .usage = vk::ImageUsageFlagBits::eTransferDst | vk::ImageUsageFlagBits::eSampled,
            .sharingMode = vk::SharingMode::eExclusive,
            .initialLayout = vk::ImageLayout::eUndefined
        };

        vk::ImageViewCreateInfo imageViewInfo = {
            .viewType = vk::ImageViewType::e2D,
            .subresourceRange = {
                .aspectMask = vk::ImageAspectFlagBits::eColor,
                .baseMipLevel = 0,
                .levelCount = 1,
                .baseArrayLayer = 0,
                .layerCount = 1,
            }
        };

        VmaAllocationCreateInfo allocInfo = {
            .flags = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
            .usage = VMA_MEMORY_USAGE_AUTO,
        };

        m_images[i] = m_allocator.createImage(imageInfo, imageViewInfo, allocInfo);

        // copy data to allocated image
        // create staging buffer
        vk::BufferCreateInfo bufferInfo = {
            .size = image.image.size(),
            .usage = vk::BufferUsageFlagBits::eTransferSrc
        };

        allocInfo = {
            .usage = VMA_MEMORY_USAGE_CPU_TO_GPU
        };

        Buffer stagingBuffer = m_allocator.createBuffer(bufferInfo, allocInfo);
        memcpy(stagingBuffer.pMapping, image.image.data(), image.image.size());

        vk::ImageSubresourceRange imgSubRange = {
            .aspectMask = vk::ImageAspectFlagBits::eColor,
            .baseMipLevel = 0,
            .levelCount = vk::RemainingMipLevels,
            .baseArrayLayer = 0,
            .layerCount = vk::RemainingArrayLayers
        };
        // transition image to transfer dst optimal
        imageLayoutTransition(cmd,
            m_images[i].image,
            vk::PipelineStageFlagBits2::eNone,
            vk::PipelineStageFlagBits2::eTransfer,
            vk::AccessFlagBits2::eNone,
            vk::AccessFlagBits2::eTransferWrite,
            vk::ImageLayout::eUndefined,
            vk::ImageLayout::eTransferDstOptimal,
            imgSubRange);
        // record commands for data upload
        vk::BufferImageCopy2 copyRegion = {
            .bufferOffset = 0,
            .bufferRowLength = 0,
            .bufferImageHeight = 0,
            .imageSubresource = {
                .aspectMask = vk::ImageAspectFlagBits::eColor,
                .mipLevel = 0,
                .baseArrayLayer = 0,
                .layerCount = 1
            },
            .imageExtent = m_images[i].extent,
        };
        vk::CopyBufferToImageInfo2 copyInfo2 = {
            .srcBuffer = stagingBuffer.buffer,
            .dstImage = m_images[i].image,
            .dstImageLayout = vk::ImageLayout::eTransferDstOptimal,
            .regionCount = 1,
        };
        cmd.copyBufferToImage2(copyInfo2);
        // transition back to shader read only
        imageLayoutTransition(cmd,
            m_images[i].image,
            vk::PipelineStageFlagBits2::eTransfer,
            vk::PipelineStageFlagBits2::eFragmentShader | vk::PipelineStageFlagBits2::eComputeShader | vk::PipelineStageFlagBits2::eRayTracingShaderKHR,
            vk::AccessFlagBits2::eTransferWrite,
            vk::AccessFlagBits2::eShaderRead,
            vk::ImageLayout::eTransferDstOptimal,
            vk::ImageLayout::eShaderReadOnlyOptimal,
            imgSubRange);
    }
}

void GltfSceneVulkan::createTextures(vk::raii::CommandBuffer &cmd, app::GltfScene &scene) {
    // create texture info that references imageID and samplerID

}

void GltfSceneVulkan::createVertexIndexBuffers() {

}

static std::vector<shaderio::GltfLight> createGltfLights(const std::vector<app::RenderLight> &renderLights,
                                                         const std::vector<tinygltf::Light> &lights)
{
    std::vector<shaderio::GltfLight> gltfLights;
    gltfLights.reserve(lights.size());

    for(auto& renderLight : renderLights) {
        const auto& light = lights[renderLight.lightID];

        shaderio::GltfLight gltfLight{};
        gltfLight.position = renderLight.worldMatrix[3];
        gltfLight.direction = -renderLight.worldMatrix[2];  // glm::vec3(l.worldMatrix * glm::vec4(0, 0, -1, 0)), see gltf point light extension
        gltfLight.innerAngle = static_cast<float>(light.spot.innerConeAngle);
        gltfLight.outerAngle = static_cast<float>(light.spot.outerConeAngle);
        if(light.color.size() == 3) {
            gltfLight.color = glm::vec3(gltfLight.color[0], gltfLight.color[1], gltfLight.color[2]);
        }
        else{
            gltfLight.color = glm::vec3(1, 1, 1);  // default color (white)
        }
        gltfLight.intensity = static_cast<float>(light.intensity);
        gltfLight.type = light.type == "point" ? shaderio::ePoint
                        : light.type == "spot"  ? shaderio::eSpot
                        : shaderio::eDirectional;

        gltfLight.radius = light.extras.Has("radius") ? static_cast<float>(light.extras.Get("radius").GetNumberAsDouble()) : 0.0f;

        if(gltfLight.type == shaderio::eDirectional) {
            constexpr double sun_distance = 149597870.0;
            double angularSizeRad = 2.0 * std::atan(gltfLight.radius / sun_distance);
            gltfLight.angularSizeOrInvRange = static_cast<float>(angularSizeRad);
        }
        else {
            gltfLight.angularSizeOrInvRange = (light.range > 0.0) ? 1.0f / static_cast<float>(light.range) : 0.0f;
        }

        gltfLights.emplace_back(gltfLight);
    }
    return gltfLights;
}
}
