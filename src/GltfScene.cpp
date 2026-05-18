#include "GltfScene.h"
#include "GltfUtils.h"

namespace app {
void GltfScene::ImportGltfData(const tinygltf::Model& model, const ptvk::Core& core) const {
    const auto& allocator = core.GetResourceAllocator();

    // TODO: Load Materials

    // TODO: Load Textures

    // Load geometry data
    const uint32_t meshOffset = meshes.size();

    // Lambda for element byte size calculation
    auto getElementByteSize = [](int type) -> uint32_t {
        return type == TINYGLTF_COMPONENT_TYPE_UNSIGNED_SHORT ? 2U :
               type == TINYGLTF_COMPONENT_TYPE_UNSIGNED_INT   ? 4U :
               type == TINYGLTF_COMPONENT_TYPE_FLOAT          ? 4U :
                                                                0U;
    };

    // Lambda for type size calculation
    auto getTypeSize = [](int type) -> uint32_t {
        return type == TINYGLTF_TYPE_VEC2 ? 2U :
               type == TINYGLTF_TYPE_VEC3 ? 3U :
               type == TINYGLTF_TYPE_VEC4 ? 4U :
               type == TINYGLTF_TYPE_MAT2 ? 4U * 2U :
               type == TINYGLTF_TYPE_MAT3 ? 4U * 3U :
               type == TINYGLTF_TYPE_MAT4 ? 4U * 4U :
                                            0U;
    };

    // Lambda for extracting attributes
    auto extractAttribute = [&](const std::string& name, shaderio::BufferView& attr, const tinygltf::Primitive& primitive) {
        if(!primitive.attributes.contains(name))
        {
            attr.offset = -1;
            return;
        }
        const tinygltf::Accessor&   acc = model.accessors[primitive.attributes.at(name)];
        const tinygltf::BufferView& bv  = model.bufferViews[acc.bufferView];
        attr = {
            .offset = static_cast<uint32_t>(bv.byteOffset + acc.byteOffset),
            .count  = static_cast<uint32_t>(acc.count),
            .stride = static_cast<uint32_t>(bv.byteStride ? static_cast<uint32_t>(bv.byteStride) : getTypeSize(acc.type) * getElementByteSize(acc.componentType)),
        };
    };

    for (const auto &mesh: model.meshes) {
        for (const auto &primitive : mesh.primitives) {
            assert(primitive.mode == TINYGLTF_PRIMITIVE_TRIANGLE && "Only glTF triangle primitives are supported");

        }
    }

    vk::BufferCreateInfo bufferCreateInfo {
        .size = std::span(model.buffers[0].data).size_bytes(),
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

    ptvk::Buffer geometryData = allocator.CreateBuffer(bufferCreateInfo, vmaAllocInfo);

    /* OBJ Wavefront model loading
    // check allocation info
    //VkMemoryPropertyFlags flags;
    //m_vkCore->GetResourceAllocator().GetAllocationInfo(modelBuffer.allocation, &flags);
    //ptvk::PrintMemoryPropertyFlags(flags);

    memcpy(geometryData.mapping, model.vertices.data(), verticesSize);
    memcpy(geometryData.mapping + verticesSize, model.indices.data(), indicesSize);

    shaderio::GltfMesh modelData{};
    modelData.triangleMesh.positions = {
        .offset = 0,
        .count = static_cast<uint32_t>(model.vertices.size()),
        .stride = sizeof(Vertex)
    };

    modelData.triangleMesh.normals = {
        .offset = offsetof(Vertex, normal),
        .count = static_cast<uint32_t>(model.vertices.size()),
        .stride = sizeof(Vertex)
    };

    modelData.triangleMesh.uvs = {
        .offset = offsetof(Vertex, uv),
        .count = static_cast<uint32_t>(model.vertices.size()),
        .stride = sizeof(Vertex)
    };

    modelData.triangleMesh.indices = {
        .offset = static_cast<uint32_t>(verticesSize),
        .count = static_cast<uint32_t>(model.indices.size()),
        .stride = sizeof(uint32_t)
    };

    modelData.gltfDataAddress = reinterpret_cast<uint8_t *>(geometryData.address);
    modelData.indexType = static_cast<int>(vk::IndexType::eUint32);

    modeles.push_back(modelData);
    */
}

//TODO
void GltfScene::ImportGltfScene(const tinygltf::Model &model, const ptvk::Core& core) const {

}
}
