#include "GltfSceneVulkan.h"
#include "Utils.h"
#include "../shaders/gltfio.h.slang"
#include "../GltfUtils.h"

namespace ptvk {
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

static vk::SamplerCreateInfo getSampler(const tinygltf::Model model, int id) {
    vk::SamplerCreateInfo samplerInfo{
        .magFilter = vk::Filter::eLinear,
        .minFilter = vk::Filter::eLinear,
        .mipmapMode = vk::SamplerMipmapMode::eLinear,
        .maxLod = vk::LodClampNone
    };

    if (id < 0) {
        return samplerInfo;
    }

    const auto& sampler = model.samplers[id];

    if (sampler.minFilter > -1) {
        samplerInfo.minFilter = app::gltfutils::extractFilter(sampler.minFilter);
    }

    if (sampler.magFilter > -1) {
        samplerInfo.magFilter = app::gltfutils::extractFilter(sampler.magFilter);
        samplerInfo.mipmapMode = app::gltfutils::extractMipmapMode(sampler.magFilter);
    }

    samplerInfo.addressModeU = app::gltfutils::extractWrapMode(sampler.wrapS);
    samplerInfo.addressModeV = app::gltfutils::extractWrapMode(sampler.wrapT);

    return samplerInfo;
}

void GltfSceneVulkan::copyImage(const vk::raii::CommandBuffer& cmd, const void* data, vk::DeviceSize size, Image& image, vk::ImageLayout& newLayout) {
    // create staging buffer
    vk::BufferCreateInfo bufferInfo = {
        .size = size,
        .usage = vk::BufferUsageFlagBits::eTransferSrc
    };

    VmaAllocationCreateInfo allocInfo = {
        .flags = VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT | VMA_ALLOCATION_CREATE_MAPPED_BIT,
        .usage = VMA_MEMORY_USAGE_AUTO,
    };

    Buffer stagingBuffer = m_allocator.createBuffer(bufferInfo, allocInfo);
    memcpy(stagingBuffer.pMapping, data, size);

    vk::ImageSubresourceRange imgSubRange = {
        .aspectMask = vk::ImageAspectFlagBits::eColor,
        .baseMipLevel = 0,
        .levelCount = vk::RemainingMipLevels,
        .baseArrayLayer = 0,
        .layerCount = vk::RemainingArrayLayers
    };
    // transition image to transfer dst optimal


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
        .imageExtent = image.extent,
    };

    vk::CopyBufferToImageInfo2 copyInfo2 = {
        .srcBuffer = stagingBuffer.buffer,
        .dstImage = image.image,
        .dstImageLayout = vk::ImageLayout::eTransferDstOptimal,
        .regionCount = 1,
        .pRegions = &copyRegion
    };

    cmd.copyBufferToImage2(copyInfo2);

    // transition back to shader read only
    utils::imageLayoutTransition(cmd,
                          image.image,
                          vk::PipelineStageFlagBits2::eTransfer,
                          vk::PipelineStageFlagBits2::eFragmentShader | vk::PipelineStageFlagBits2::eComputeShader |
                          vk::PipelineStageFlagBits2::eRayTracingShaderKHR,
                          vk::AccessFlagBits2::eTransferWrite,
                          vk::AccessFlagBits2::eShaderRead,
                          vk::ImageLayout::eTransferDstOptimal,
                          newLayout,
                          imgSubRange);
}

GltfSceneVulkan::GltfSceneVulkan(const ResourceAllocator &allocator,
                                 SamplerPool &samplerPool,
                                 bool generateMipmaps) : m_allocator(allocator),
                                                         m_samplerPool(samplerPool),
                                                         m_generateMipmaps(generateMipmaps) {}

GltfSceneVulkan::~GltfSceneVulkan() {
    destroy();
}

void GltfSceneVulkan::destroy() {
    for (auto sampler : m_samplers) {
        m_samplerPool.releaseSampler(sampler);
    }

    m_bMaterials = {};
    m_bLights = {};
    m_bRenderNodes = {};
    m_bRenderPrimitives = {};
    m_bSceneInfo = {};

    m_vertexBuffers.clear();
    m_bIndices.clear();
    m_images.clear();
    m_samplers.clear();
}

void GltfSceneVulkan::createVkResources(const vk::raii::CommandBuffer &cmd, const app::GltfScene &scene) {
    createSamplers(scene.getModel());
    uploadTextureImages(cmd, scene);
    createTextures(cmd, scene);
}

void GltfSceneVulkan::updateFromScene(const vk::raii::CommandBuffer &cmd, const app::GltfScene &scene) {

}

void GltfSceneVulkan::uploadTextureImages(const vk::raii::CommandBuffer &cmd, const app::GltfScene &scene) {
    const auto& model = scene.getModel();

    // if no images create default image for default texture
    if (model.images.empty()) {
        createDefaultImage(0, {255,255,255,255});
    }

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

    }
}

void GltfSceneVulkan::createSamplers(const tinygltf::Model &model) {
    if(m_samplers.empty())
    {
        m_samplers.push_back(m_samplerPool.acquireSampler());
    }

    for(size_t j = m_samplers.size() - 1; j < model.samplers.size(); ++j)
    {
        const vk::SamplerCreateInfo samplerInfo = getSampler(model, static_cast<int>(j));
        m_samplers.push_back(m_samplerPool.acquireSampler(samplerInfo));
    }
}

void GltfSceneVulkan::createTextures(const vk::raii::CommandBuffer &cmd, const app::GltfScene &scene) {
    // create texture info that references imageID and samplerID
    const auto& textures = scene.getModel().textures;
    for (size_t i = 0; i < textures.size(); i++) {

    }


}

void GltfSceneVulkan::createDefaultImage(int id, const std::array<uint8_t, 4> &color) {
    vk::ImageCreateInfo imgInfo = {
        .extent = {1, 1, 1},
        .usage = vk::ImageUsageFlagBits::eSampled | vk::ImageUsageFlagBits::eTransferSrc,
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

    m_images[id] = m_allocator.createImage(imgInfo, imageViewInfo, allocInfo);

    copyImage()
}

void GltfSceneVulkan::createVertexIndexBuffers() {

}
}
