#ifndef VK_RAYTRACING_GLTFSCENEVULKAN_H
#define VK_RAYTRACING_GLTFSCENEVULKAN_H

#include "SamplerPool.h"
#include "StagingUploader.h"
#include "../GltfScene.h"


namespace ptvk {
class GltfSceneVulkan {
public:
    GltfSceneVulkan(const ResourceAllocator &allocator,
                    SamplerPool &samplerPool,
                    bool generateMipmaps);
    ~GltfSceneVulkan();

    void createVkResources(const vk::raii::CommandBuffer &cmd, StagingUploader &staging, app::GltfScene &scene);
    void updateFromScene(app::GltfScene &scene, int frameNum);
    void destroy();

    const std::vector<Buffer>&  getVertexBuffers() const {return m_bVertices;}
    const std::vector<Buffer>&  getIndexBuffers() const {return m_bIndices;}
    const Buffer&               getPrimitivesBuffer() const {return m_bRenderPrimitives;}
    const Buffer&               getMaterialsBuffer() const {return m_bMaterials;}
    const Buffer&               getTextureInfosBuffer() const {return m_bTextureInfos;}
    const Buffer&               getRenderLightsBuffer(int frame) const {return m_bRenderLights[frame];}
    const Buffer&               getRenderNodesBuffer(int frame) const {return m_bRenderNodes[frame];}

    const std::vector<vk::Sampler>& getSamplers() const {return m_samplers;}

    const Image& getTextureImage(int idx) const {return m_images[idx];}

private:
    const ResourceAllocator &m_allocator;
    SamplerPool             &m_samplerPool;
    std::vector<vk::Sampler> m_samplers;

    std::vector<Image>                       m_images{};
    std::vector<Buffer>                      m_bVertices{};
    std::vector<Buffer>                      m_bIndices{};
    Buffer                                   m_bMaterials{};
    Buffer                                   m_bTextureInfos{}; // 1 to 1 correspondence with gltf textures for material and img id
    std::array<Buffer, MAX_FRAMES_IN_FLIGHT> m_bRenderLights{}; // currently the only editable scene elements are transforms
    std::array<Buffer, MAX_FRAMES_IN_FLIGHT> m_bRenderNodes{};
    Buffer                                   m_bRenderPrimitives{};
    std::array<Buffer, MAX_FRAMES_IN_FLIGHT> m_bSceneInfo{};

    bool m_generateMipmaps{false}; // TODO

    void uploadTextureImages(const vk::raii::CommandBuffer &cmd, StagingUploader &staging, tinygltf::Model &model);
    void createSamplers(const tinygltf::Model &model);
    void uploadTextureInfos(const tinygltf::Model &model);

    void uploadMaterials(const tinygltf::Model &model);

    void createDefaultImage(StagingUploader &staging, int id); // create 1x1 dummy image
    void createVertexIndexBuffers(const app::GltfScene &scene);
    void uploadRenderNodes(const app::GltfScene &scene, const std::unordered_set<int> &dirtyNodes, int frameNum);
    void uploadRenderLights(const app::GltfScene &scene, const std::unordered_set<int> &dirtyLights, int frameNum);

    void uploadSceneInfo(const app::GltfScene &scene, int frameNum);
};
}

#endif //VK_RAYTRACING_GLTFSCENEVULKAN_H
