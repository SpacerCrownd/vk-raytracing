#include <iostream>

#include "GltfSceneVulkan.h"
#include "Utils.h"
#include "../shaders/gltfio.h.slang"
#include "../GltfUtils.h"
#include "glm/gtc/type_ptr.hpp"

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
            gltfLight.color = glm::vec3(light.color[0], light.color[1], light.color[2]);
        } else {
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
        } else {
            gltfLight.angularSizeOrInvRange = (light.range > 0.0) ? 1.0f / static_cast<float>(light.range) : 0.0f;
        }

        gltfLights.emplace_back(gltfLight);
    }
    return gltfLights;
}

static vk::SamplerCreateInfo getSampler(const tinygltf::Model& model, int id) {
    vk::SamplerCreateInfo samplerInfo{
        .magFilter = vk::Filter::eLinear,
        .minFilter = vk::Filter::eLinear,
        .mipmapMode = vk::SamplerMipmapMode::eLinear,
        .anisotropyEnable = vk::True,
        .maxAnisotropy = 8.0f,
        .maxLod = vk::LodClampNone,
    };

    if (id < 0) {
        return samplerInfo;
    }

    const auto& sampler = model.samplers[id];

    if (sampler.minFilter > -1) {
        samplerInfo.minFilter = app::gltfutils::extractFilter(sampler.minFilter);
        samplerInfo.mipmapMode = app::gltfutils::extractMipmapMode(sampler.minFilter);
    }

    if (sampler.magFilter > -1) {
        samplerInfo.magFilter = app::gltfutils::extractFilter(sampler.magFilter);
    }

    samplerInfo.addressModeU = app::gltfutils::extractWrapMode(sampler.wrapS);
    samplerInfo.addressModeV = app::gltfutils::extractWrapMode(sampler.wrapT);

    return samplerInfo;
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
    m_bRenderLights = {};
    m_bRenderNodes = {};
    m_bRenderPrimitives = {};
    m_bSceneInfo = {};

    m_bVertices.clear();
    m_bIndices.clear();
    m_images.clear();
    m_samplers.clear();
}

void GltfSceneVulkan::createVkResources(const vk::raii::CommandBuffer &cmd, StagingUploader &staging, app::GltfScene &scene) {
    auto& model = scene.getModel();
    createSamplers(model);
    uploadTextureImages(cmd, staging, model);
    uploadTextureInfos(model);
    uploadMaterials(model);
    createVertexIndexBuffers(scene);
    for (int i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
        uploadRenderNodes(scene, {}, i);
        uploadRenderLights(scene, {}, i);
        uploadSceneInfo(scene, i);
    }
}

void GltfSceneVulkan::updateFromScene(app::GltfScene &scene, int frameNum) {
    auto& dirtyFlags = scene.getDirtyFlags();

    // update lights
    const auto& dirtyLights = dirtyFlags.lightIDs;
    const std::vector<app::RenderLight>& renderLights = scene.getRenderLights();

    if(!renderLights.empty() && (!dirtyLights.empty() || m_bRenderLights[frameNum].buffer == VK_NULL_HANDLE)) {
        uploadRenderLights(scene, dirtyLights, frameNum);
        dirtyFlags.lightIDs.clear();
    }

    const auto& dirtyNodes = dirtyFlags.renderNodesVkIDs;
    if(!dirtyNodes.empty() || m_bRenderNodes[frameNum].buffer == VK_NULL_HANDLE) {
        uploadRenderNodes(scene, dirtyNodes, frameNum);
        dirtyFlags.renderNodesVkIDs.clear();
    }
}

void GltfSceneVulkan::uploadTextureImages(const vk::raii::CommandBuffer &cmd, StagingUploader &staging, tinygltf::Model &model) {
    // if no images create default image for default texture
    if (model.images.empty()) {
        std::cout << "[INFO] No texture images found in glTF file, creating default texture image" << std::endl;
        m_images.resize(1);
        createDefaultImage(staging, 0);
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
            .usage = VMA_MEMORY_USAGE_AUTO,
            .requiredFlags = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
        };

        m_images.push_back(m_allocator.createImage(imageInfo, imageViewInfo, allocInfo));

        unsigned char *buffer = nullptr;
        VkDeviceSize bufferSize = 0;

        // convert rgb images to rgba
        bool deleteBuffer = false;
        if (image.component == 3) {
            bufferSize = image.width * image.height * 4;
            buffer = new unsigned char[bufferSize];

            unsigned char* rgba = buffer;
            const unsigned char* rgb = &image.image[0];

            for (size_t j = 0; j < image.width * image.height; ++j) {
                memcpy(rgba, rgb, sizeof(unsigned char) * 3);
                // go to next pixel (bytes)
                rgba += 4;
                rgb += 3;
            }

            deleteBuffer = true;
        } else {
            buffer = &image.image[0];
            bufferSize = image.image.size();
        }

        staging.appendImage(m_images[i], buffer, bufferSize, vk::ImageLayout::eUndefined, vk::ImageLayout::eShaderReadOnlyOptimal);

        if (deleteBuffer) {
            delete[] buffer;
        }
    }

    staging.uploadAppendedCmd(cmd);
    std::cout << "[INFO] glTF scene texture images upload appended successfully" << std::endl;
}

void GltfSceneVulkan::createSamplers(const tinygltf::Model &model) {
    if(m_samplers.empty()) {
        m_samplers.push_back(m_samplerPool.acquireSampler());
    }

    for(size_t j = m_samplers.size() - 1; j < model.samplers.size(); ++j) {
        const vk::SamplerCreateInfo samplerInfo = getSampler(model, static_cast<int>(j));
        m_samplers.push_back(m_samplerPool.acquireSampler(samplerInfo));
    }
    std::cout << "[INFO] Samplers created from glTF scene" << std::endl;
}

void GltfSceneVulkan::uploadTextureInfos(const tinygltf::Model &model) {
    std::vector<shaderio::GltfTextureInfo> textureInfos;

    // add default texture if no textures
    if (model.textures.empty()) {
        textureInfos.emplace_back(0, 0);
    }

    // create texture info that references imageID and samplerID
    for (const auto & texture : model.textures) {
        textureInfos.emplace_back(texture.source, texture.sampler + 1);
    }

    vk::BufferCreateInfo bufferInfo = {
        .size = std::span(textureInfos).size_bytes(),
        .usage = vk::BufferUsageFlagBits::eTransferDst
               | vk::BufferUsageFlagBits::eStorageBuffer
             | vk::BufferUsageFlagBits::eShaderDeviceAddress,
    };

    VmaAllocationCreateInfo allocCreateInfo = {
        .flags = VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT
               | VMA_ALLOCATION_CREATE_MAPPED_BIT
               | VMA_ALLOCATION_CREATE_HOST_ACCESS_ALLOW_TRANSFER_INSTEAD_BIT,
        .usage = VMA_MEMORY_USAGE_AUTO,
        .requiredFlags = VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
    };

    m_bTextureInfos = m_allocator.createBuffer(bufferInfo, allocCreateInfo);
    VkMemoryPropertyFlags memFlags;
    m_allocator.getAllocationInfo(m_bTextureInfos.allocation, &memFlags);
    if (!(memFlags & VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT) || !(memFlags & VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT))
        throw std::runtime_error("[ERROR] Allocated buffer not host visible and device local (currently unsupported)");
    memcpy(m_bTextureInfos.pMapping, textureInfos.data(), std::span(textureInfos).size_bytes());

    std::cout << "[INFO] Texture infos created and uploaded to gpu buffer" << std::endl;
}

void GltfSceneVulkan::uploadMaterials(const tinygltf::Model &model) {
    // default material evaluation is in-shader
    // create gltf materials and upload to gpu buffer
    std::vector<shaderio::GltfMaterial> gltfMaterials;

    // there is at least one material
    for (const auto& srcMat : model.materials) {
        shaderio::GltfMaterial dstMat{};

        const auto& pbr = srcMat.pbrMetallicRoughness;

        // PBR factors
        dstMat.baseColor = glm::vec4(
            static_cast<float>(pbr.baseColorFactor[0]),
            static_cast<float>(pbr.baseColorFactor[1]),
            static_cast<float>(pbr.baseColorFactor[2]),
            static_cast<float>(pbr.baseColorFactor[3])
        );

        dstMat.metallic = static_cast<float>(pbr.metallicFactor);
        dstMat.roughness = static_cast<float>(pbr.roughnessFactor);

        if (pbr.baseColorTexture.index >= 0)
        {
            dstMat.baseColorTextureID = pbr.baseColorTexture.index;
        }

        if (pbr.metallicRoughnessTexture.index >= 0)
        {
            dstMat.metallicTextureID = pbr.metallicRoughnessTexture.index;
            dstMat.roughnessTextureID = pbr.metallicRoughnessTexture.index;
        }

        if (srcMat.normalTexture.index >= 0)
        {
            dstMat.normalTextureID = srcMat.normalTexture.index;
        }
        gltfMaterials.push_back(dstMat);
    }

    vk::BufferCreateInfo bufferInfo = {
        .size = std::span(gltfMaterials).size_bytes(),
        .usage = vk::BufferUsageFlagBits::eTransferDst
               | vk::BufferUsageFlagBits::eStorageBuffer
               | vk::BufferUsageFlagBits::eShaderDeviceAddress,
    };

    VmaAllocationCreateInfo allocCreateInfo = {
        .flags = VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT
               | VMA_ALLOCATION_CREATE_MAPPED_BIT
               | VMA_ALLOCATION_CREATE_HOST_ACCESS_ALLOW_TRANSFER_INSTEAD_BIT,
        .usage = VMA_MEMORY_USAGE_AUTO,
        .requiredFlags = VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
    };

    m_bMaterials = m_allocator.createBuffer(bufferInfo, allocCreateInfo);

    VkMemoryPropertyFlags memFlags;
    m_allocator.getAllocationInfo(m_bMaterials.allocation, &memFlags);

    if (!(memFlags & VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT) || !(memFlags & VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT)) {
        throw std::runtime_error("[ERROR] Allocated material buffer not host visible and device local (currently unsupported)");
    }

    memcpy(m_bMaterials.pMapping, gltfMaterials.data(), std::span(gltfMaterials).size_bytes());
    std::cout << "[INFO] Materials created and uploaded to gpu buffer" << std::endl;
}

void GltfSceneVulkan::createDefaultImage(StagingUploader &staging, int id) {
    //checkerboard image
    uint32_t black = glm::packUnorm4x8(glm::vec4(0, 0, 0, 0));
    uint32_t magenta = glm::packUnorm4x8(glm::vec4(1, 0, 1, 1));
    std::array<uint32_t, 16 * 16> pixels{}; //16x16 checkerboard texture
    for (int x = 0; x < 16; x++) {
        for (int y = 0; y < 16; y++) {
            pixels[y*16 + x] = ((x % 2) ^ (y % 2)) ? magenta : black;
        }
    }

    vk::ImageCreateInfo imgInfo = {
        .format = vk::Format::eR8G8B8A8Unorm,
        .extent = {16, 16, 1},
        .usage = vk::ImageUsageFlagBits::eSampled | vk::ImageUsageFlagBits::eTransferDst,
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
        .flags = VMA_ALLOCATION_CREATE_DEDICATED_MEMORY_BIT,
        .usage = VMA_MEMORY_USAGE_AUTO,
    };

    m_images[id] = m_allocator.createImage(imgInfo, imageViewInfo, allocInfo);
    size_t size = imgInfo.extent.width * imgInfo.extent.height * imgInfo.extent.depth * 4;
    // append image to staging
    staging.appendImage(m_images[id], pixels.data(), size, vk::ImageLayout::eUndefined, vk::ImageLayout::eShaderReadOnlyOptimal);
}

void GltfSceneVulkan::createVertexIndexBuffers(const app::GltfScene &scene) {
    std::vector<shaderio::GltfRenderPrimitive> renderPrimitives;

    size_t numPrimitives = scene.getNumRenderPrimitives();
    m_bIndices.reserve(numPrimitives);
    m_bVertices.reserve(numPrimitives);
    renderPrimitives.reserve(numPrimitives);

    const auto& model = scene.getModel();

    // create vertex and index buffers for all unique render primitives (submesh)
    for (size_t primID = 0; primID < scene.getNumRenderPrimitives(); primID++) {
        auto& primitive = *scene.getRenderPrimitive(primID).pPrimitive;

        //////
        // Vertices
        //////
        std::vector<shaderio::Vertex> vertices{};
        const float* positionsBuffer = nullptr;
        const float* normalsBuffer = nullptr;
        const float* texCoordsBuffer = nullptr;
        const float* colorsBuffer = nullptr;
        const float* tangentsBuffer = nullptr;
        size_t vertexCount = 0;

        // assumptions:
        //  only floats (stride and gltf component type ignored)
        //  tightly packed (doesn't handle sparse
        if (primitive.attributes.contains("POSITION")) {
            const tinygltf::Accessor& accessor = model.accessors[primitive.attributes.find("POSITION")->second];
            const tinygltf::BufferView& view = model.bufferViews[accessor.bufferView];
            positionsBuffer = reinterpret_cast<const float*>(&model.buffers[view.buffer].data[accessor.byteOffset + view.byteOffset]);
            vertexCount = accessor.count;
        }

        if (primitive.attributes.contains("NORMAL")) {
            const tinygltf::Accessor& accessor = model.accessors[primitive.attributes.find("NORMAL")->second];
            const tinygltf::BufferView& view = model.bufferViews[accessor.bufferView];
            normalsBuffer = reinterpret_cast<const float*>(&model.buffers[view.buffer].data[accessor.byteOffset + view.byteOffset]);
        }

        if (primitive.attributes.contains("TEXCOORD_0")) {
            const tinygltf::Accessor& accessor = model.accessors[primitive.attributes.find("TEXCOORD_0")->second];
            const tinygltf::BufferView& view = model.bufferViews[accessor.bufferView];
            texCoordsBuffer = reinterpret_cast<const float*>(&(model.buffers[view.buffer].data[accessor.byteOffset + view.byteOffset]));
        }

        if (primitive.attributes.contains("COLOR_0")) {
            const tinygltf::Accessor& accessor = model.accessors[primitive.attributes.find("COLOR_0")->second];
            const tinygltf::BufferView& view = model.bufferViews[accessor.bufferView];
            colorsBuffer = reinterpret_cast<const float*>(&(model.buffers[view.buffer].data[accessor.byteOffset + view.byteOffset]));
        }

        if (primitive.attributes.contains("TANGENT")) {
            const tinygltf::Accessor& accessor = model.accessors[primitive.attributes.find("TANGENT")->second];
            const tinygltf::BufferView& view = model.bufferViews[accessor.bufferView];
            tangentsBuffer = reinterpret_cast<const float*>(&(model.buffers[view.buffer].data[accessor.byteOffset + view.byteOffset]));
        }

        for (size_t v = 0; v < vertexCount; v++) {
            shaderio::Vertex vertex = {
                .position = glm::vec3(glm::make_vec3(&positionsBuffer[v * 3])),
                .normal = glm::normalize(glm::vec3(normalsBuffer ? glm::make_vec3(&normalsBuffer[v * 3]) : glm::vec3(0.0f))),
                .texCoords = texCoordsBuffer ? glm::make_vec2(&texCoordsBuffer[v * 2]) : glm::vec3(0.0f),
                .color = glm::vec4(1.0f),
                .tangent = tangentsBuffer ? glm::make_vec4(&tangentsBuffer[v * 4]) : glm::vec4(0.0f),
            };

            if (colorsBuffer) {
                const tinygltf::Accessor& accessor =
                    model.accessors[primitive.attributes.at("COLOR_0")];

                if (accessor.type == TINYGLTF_TYPE_VEC3) {
                    vertex.color = glm::vec4(
                        colorsBuffer[v * 3 + 0],
                        colorsBuffer[v * 3 + 1],
                        colorsBuffer[v * 3 + 2],
                        1.0f
                    );
                } else if (accessor.type == TINYGLTF_TYPE_VEC4) {
                    vertex.color = glm::vec4(
                        colorsBuffer[v * 4 + 0],
                        colorsBuffer[v * 4 + 1],
                        colorsBuffer[v * 4 + 2],
                        colorsBuffer[v * 4 + 3]
                    );
                } else {
                    std::cout << "[ERROR] Unknown color attribute type" << std::endl;
                }
            }

            vertices.push_back(vertex);
        }

        // create vertex buffer and copy data to it
        vk::BufferCreateInfo bufferInfo = {
            .size = std::span(vertices).size_bytes(),
            .usage = vk::BufferUsageFlagBits::eVertexBuffer
                   | vk::BufferUsageFlagBits::eStorageBuffer
                 | vk::BufferUsageFlagBits::eShaderDeviceAddress
                 | vk::BufferUsageFlagBits::eAccelerationStructureBuildInputReadOnlyKHR
        };

        VmaAllocationCreateInfo allocInfo = {
            .flags = VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT
                   | VMA_ALLOCATION_CREATE_MAPPED_BIT,
            .usage = VMA_MEMORY_USAGE_AUTO,
            .requiredFlags = VK_MEMORY_PROPERTY_HOST_COHERENT_BIT | VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT
        };
        m_bVertices.push_back(m_allocator.createBuffer(bufferInfo, allocInfo));
        memcpy(m_bVertices[primID].pMapping, vertices.data(), std::span(vertices).size_bytes());

        //////
        // Indices
        //////
        std::vector<uint32_t> indices{};
        size_t indexCount = 0;

        if (primitive.indices > -1) {
            const tinygltf::Accessor& accessor = model.accessors[primitive.indices];
            const tinygltf::BufferView& bufferView = model.bufferViews[accessor.bufferView];
            const tinygltf::Buffer& buffer = model.buffers[bufferView.buffer];

            indexCount = accessor.count;

            switch (accessor.componentType) {
                case TINYGLTF_PARAMETER_TYPE_UNSIGNED_INT: {
                    const uint32_t* buf = reinterpret_cast<const uint32_t*>(&buffer.data[accessor.byteOffset + bufferView.byteOffset]);
                    for (size_t index = 0; index < accessor.count; index++) {
                        indices.push_back(buf[index]);
                    }
                    break;
                }
                case TINYGLTF_PARAMETER_TYPE_UNSIGNED_SHORT: {
                    const uint16_t* buf = reinterpret_cast<const uint16_t*>(&buffer.data[accessor.byteOffset + bufferView.byteOffset]);
                    for (size_t index = 0; index < accessor.count; index++) {
                        indices.push_back(buf[index]);
                    }
                    break;
                }
                case TINYGLTF_PARAMETER_TYPE_UNSIGNED_BYTE: {
                    const uint8_t* buf = reinterpret_cast<const uint8_t*>(&buffer.data[accessor.byteOffset + bufferView.byteOffset]);
                    for (size_t index = 0; index < accessor.count; index++) {
                        indices.push_back(buf[index]);
                    }
                    break;
                }
                default:
                    std::cout << "[ERROR] Index component type " << accessor.componentType << " not supported!" << std::endl;
                    return;
            }
        } else {
            // create indices
            const tinygltf::Accessor& accessor = model.accessors[primitive.attributes.at("POSITION")];

            indices.resize(accessor.count);
            for(auto i = 0; i < accessor.count; i++)
                indices[i] = i;
        }

        // create index buffer and copy data to it
        bufferInfo = {
            .size = std::span(indices).size_bytes(),
            .usage = vk::BufferUsageFlagBits::eIndexBuffer
                   | vk::BufferUsageFlagBits::eStorageBuffer
                 | vk::BufferUsageFlagBits::eShaderDeviceAddress
                 | vk::BufferUsageFlagBits::eAccelerationStructureBuildInputReadOnlyKHR
        };
        m_bIndices.push_back(m_allocator.createBuffer(bufferInfo, allocInfo));
        memcpy(m_bIndices[primID].pMapping, indices.data(), std::span(indices).size_bytes());

        shaderio::GltfRenderPrimitive prim = {
            .indices = reinterpret_cast<glm::uvec3 *>(m_bIndices[primID].address),
            .vertices = reinterpret_cast<shaderio::Vertex *>(m_bVertices[primID].address),
        };

        renderPrimitives.push_back(prim);
    }

    vk::BufferCreateInfo bufferInfo = {
        .size = std::span(renderPrimitives).size_bytes(),
        .usage = vk::BufferUsageFlagBits::eVertexBuffer
               | vk::BufferUsageFlagBits::eStorageBuffer
             | vk::BufferUsageFlagBits::eShaderDeviceAddress
             | vk::BufferUsageFlagBits::eAccelerationStructureBuildInputReadOnlyKHR
    };

    VmaAllocationCreateInfo allocInfo = {
        .flags = VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT
               | VMA_ALLOCATION_CREATE_MAPPED_BIT,
        .usage = VMA_MEMORY_USAGE_AUTO,
        .requiredFlags = VK_MEMORY_PROPERTY_HOST_COHERENT_BIT | VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT
    };
    m_bRenderPrimitives = m_allocator.createBuffer(bufferInfo, allocInfo);
    memcpy(m_bRenderPrimitives.pMapping, renderPrimitives.data(), std::span(renderPrimitives).size_bytes());
}

void GltfSceneVulkan::uploadRenderNodes(const app::GltfScene &scene, const std::unordered_set<int>& dirtyNodes, int frameNum) {
    const auto& renderNodes = scene.getRenderNodes();
    if (renderNodes.empty()) {
        std::cout << "[INFO] Scene contains no render nodes" << std::endl;
        return;
    }

    if (m_bRenderNodes[frameNum].buffer == VK_NULL_HANDLE) {
        vk::BufferCreateInfo bufferInfo = {
            .size = std::span(renderNodes).size_bytes(),
            .usage = vk::BufferUsageFlagBits::eStorageBuffer
                   | vk::BufferUsageFlagBits::eShaderDeviceAddress
        };

        VmaAllocationCreateInfo allocInfo = {
            .flags = VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT
                   | VMA_ALLOCATION_CREATE_MAPPED_BIT,
            .usage = VMA_MEMORY_USAGE_AUTO,
            .requiredFlags = VK_MEMORY_PROPERTY_HOST_COHERENT_BIT | VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT
        };
        m_bRenderNodes[frameNum] = m_allocator.createBuffer(bufferInfo, allocInfo);
    }

    if (dirtyNodes.empty()) {
        // if no specified dirty nodes, upload all nodes
        std::vector<shaderio::GltfRenderNode> gltfRenderNodes;
        for (const auto& renderNode : renderNodes) {
            gltfRenderNodes.emplace_back(renderNode.worldMatrix,
                             glm::inverse(renderNode.worldMatrix),
                             renderNode.renderPrimID,
                             renderNode.materialID);
        }
        memcpy(m_bRenderNodes[frameNum].pMapping, gltfRenderNodes.data(), std::span(gltfRenderNodes).size_bytes());
    } else {
        for (int nodeID : dirtyNodes) {
            shaderio::GltfRenderNode gltfRenderNode = {
                .objectToWorld = renderNodes[nodeID].worldMatrix,
                .worldToObject = glm::inverse(renderNodes[nodeID].worldMatrix),
                .renderPrimID = renderNodes[nodeID].renderPrimID,
                .materialID = renderNodes[nodeID].materialID,
            };
            const size_t offset = static_cast<size_t>(nodeID) * sizeof(shaderio::GltfRenderNode);

            memcpy(m_bRenderNodes[frameNum].pMapping + offset, &gltfRenderNode, sizeof(shaderio::GltfRenderNode));
        }
    }
}

void GltfSceneVulkan::uploadRenderLights(const app::GltfScene &scene, const std::unordered_set<int>& dirtyLights, int frameNum) {
    const auto& renderLights = scene.getRenderLights();
    if (renderLights.empty()) {
        std::cout << "[INFO] Scene contains no render lights" << std::endl;
        return;
    }

    std::vector<shaderio::GltfLight> gltfLights = createGltfLights(renderLights, scene.getModel().lights);

    if (m_bRenderLights[frameNum].buffer == VK_NULL_HANDLE) {
        vk::BufferCreateInfo bufferInfo = {
            .size = std::span(gltfLights).size_bytes(),
            .usage = vk::BufferUsageFlagBits::eStorageBuffer
                   | vk::BufferUsageFlagBits::eShaderDeviceAddress
        };

        VmaAllocationCreateInfo allocInfo = {
            .flags = VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT
                   | VMA_ALLOCATION_CREATE_MAPPED_BIT,
            .usage = VMA_MEMORY_USAGE_AUTO,
            .requiredFlags = VK_MEMORY_PROPERTY_HOST_COHERENT_BIT | VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT
        };
        m_bRenderLights[frameNum] = m_allocator.createBuffer(bufferInfo, allocInfo);
    }

    if (dirtyLights.empty()) {
        // if no specified dirty nodes, upload all nodes
        memcpy(m_bRenderLights[frameNum].pMapping, gltfLights.data(), std::span(gltfLights).size_bytes());
    } else {
        for (int lightID : dirtyLights) {

            const size_t offset = static_cast<size_t>(lightID) * sizeof(shaderio::GltfLight);

            memcpy(m_bRenderLights[frameNum].pMapping + offset, &gltfLights[lightID], sizeof(shaderio::GltfLight));
        }
    }
}

void GltfSceneVulkan::uploadSceneInfo(const app::GltfScene &scene, int frameNum) {
    // Buffer references
    shaderio::GltfSceneInfo sceneInfo = {
        .gltfPrimitives = reinterpret_cast<shaderio::GltfRenderPrimitive *>(m_bRenderPrimitives.address),
        .gltfNodes = reinterpret_cast<shaderio::GltfRenderNode *>(m_bRenderNodes[frameNum].address),
        .gltfMaterials = reinterpret_cast<shaderio::GltfMaterial *>(m_bMaterials.address),
        .gltfLights = reinterpret_cast<shaderio::GltfLight *>(m_bRenderLights[frameNum].address),
        .gltfTexturesInfos = reinterpret_cast<shaderio::GltfTextureInfo *>(m_bTextureInfos.address),
        .numLights = static_cast<int>(scene.getRenderLights().size()),
    };

    if(m_bSceneInfo[frameNum].buffer == VK_NULL_HANDLE) {
        vk::BufferCreateInfo bufferInfo = {
            .size = sizeof(sceneInfo),
            .usage = vk::BufferUsageFlagBits::eStorageBuffer
                   | vk::BufferUsageFlagBits::eShaderDeviceAddress
        };

        VmaAllocationCreateInfo allocInfo = {
            .flags = VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT
                   | VMA_ALLOCATION_CREATE_MAPPED_BIT,
            .usage = VMA_MEMORY_USAGE_AUTO,
            .requiredFlags = VK_MEMORY_PROPERTY_HOST_COHERENT_BIT | VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT
        };

        m_bSceneInfo[frameNum] = m_allocator.createBuffer(bufferInfo, allocInfo);
    }

    memcpy(m_bSceneInfo[frameNum].pMapping, &sceneInfo, sizeof(shaderio::GltfSceneInfo));
}
}
