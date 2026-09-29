#include "GltfSceneRt.h"

#include <iostream>

#include "../shaders/shaderio.h.slang"

namespace ptvk {
GltfSceneRt::GltfSceneRt(const ResourceAllocator& allocator, const Device &device) : m_allocator(allocator), m_device(device) {}

void GltfSceneRt::createBLAS(Core &vkCore,
                             const app::GltfScene &scene,
                             const GltfSceneVk &sceneVk,
                             vk::BuildAccelerationStructureFlagsKHR flags)
{
    std::cout << "[INFO] Blas building started" << std::endl;
    auto alignUp = [](auto value, size_t alignment) noexcept { return ((value + alignment - 1) & ~(alignment - 1)); };

    const vk::raii::Device& device = m_device.getVkDevice();
    const auto& renderPrimitives = scene.getRenderPrimitives();

    m_blasBuildData.resize(renderPrimitives.size());
    m_blas.resize(renderPrimitives.size());

    vk::PhysicalDeviceAccelerationStructurePropertiesKHR asProps = m_device.getPhysicalDevice().m_asProperties;
    vk::DeviceSize scratchAlignment = asProps.minAccelerationStructureScratchOffsetAlignment;

    vk::DeviceSize maxSize{512'000'000}; // parallel scratch buffer usage hard limit for simplicity's sake

    vk::DeviceSize totalScratch = 0;
    vk::DeviceSize maxScratch = 0;

    for (uint32_t primID = 0; primID < renderPrimitives.size(); primID++) {
        auto& currBuildData = m_blasBuildData[primID];
        currBuildData.type = vk::AccelerationStructureTypeKHR::eBottomLevel;

        AccelerationStructureGeometryInfo geoInfo = primitiveToGeometry(sceneVk, renderPrimitives[primID], primID);
        currBuildData.addGeometry(geoInfo);
        vk::AccelerationStructureBuildSizesInfoKHR sizeInfo = currBuildData.finalizeGeometry(device, flags);

        vk::DeviceSize alignedSize = alignUp(sizeInfo.buildScratchSize, scratchAlignment);
        totalScratch += alignedSize;
        maxScratch = std::max(maxScratch, alignedSize);

        vk::AccelerationStructureCreateInfoKHR createInfo = currBuildData.makeCreateInfo();
        m_blas[primID] = m_allocator.createAccelerationStructure(createInfo);
    }

    std::cout << "[INFO] Required scratch buffer size to build all blas in parallel: " << totalScratch << std::endl;

    if (totalScratch <= maxSize) {
        std::cout << "[INFO] Total scratch buffer size <= 512MB, parallel blas build path" << std::endl;

        vk::DeviceSize scratchOffset = 0;

        vk::BufferCreateInfo buffInfo = {
            .size = totalScratch,
            .usage = vk::BufferUsageFlagBits::eStorageBuffer | vk::BufferUsageFlagBits::eShaderDeviceAddress | vk::BufferUsageFlagBits::eAccelerationStructureStorageKHR,
        };

        VmaAllocationCreateInfo allocInfo = {
            .usage = VMA_MEMORY_USAGE_AUTO_PREFER_DEVICE
        };

        m_blasScratchBuffer = m_allocator.createBufferWithAlignment(buffInfo, allocInfo, asProps.minAccelerationStructureScratchOffsetAlignment);

        vk::raii::CommandBuffer cmd = vkCore.beginSingleTimeCommandBuffer();

        for (uint32_t primID = 0; primID < renderPrimitives.size(); primID++) {
            m_blasBuildData[primID].cmdBuildAccelerationStructure(cmd, m_blas[primID].accel, m_blasScratchBuffer.address + scratchOffset);
            scratchOffset += alignUp(m_blasBuildData[primID].buildSizeInfo.buildScratchSize, scratchAlignment);
        }

        vkCore.submitSingleTimeCommandBuffer(cmd);
    } else {
        std::cout << "[INFO] Total scratch buffer size > 512MB, sequential blas build path" << std::endl;

        vk::BufferCreateInfo buffInfo = {
            .size = maxScratch,
            .usage = vk::BufferUsageFlagBits::eStorageBuffer | vk::BufferUsageFlagBits::eShaderDeviceAddress | vk::BufferUsageFlagBits::eAccelerationStructureStorageKHR
        };
        VmaAllocationCreateInfo allocInfo = {
            .usage = VMA_MEMORY_USAGE_AUTO_PREFER_DEVICE
        };

        m_blasScratchBuffer = m_allocator.createBufferWithAlignment(buffInfo, allocInfo, asProps.minAccelerationStructureScratchOffsetAlignment);

        for (uint32_t primID = 0; primID < renderPrimitives.size(); primID++) {
            vk::raii::CommandBuffer cmd = vkCore.beginSingleTimeCommandBuffer();
            m_blasBuildData[primID].cmdBuildAccelerationStructure(cmd, m_blas[primID].accel, m_blasScratchBuffer.address);

            vkCore.submitSingleTimeCommandBuffer(cmd);
        }
    }
}

void GltfSceneRt::createTLAS(const vk::raii::CommandBuffer &cmd,
                             const app::GltfScene &scene,
                             const GltfSceneVk &sceneVk,
                             vk::BuildAccelerationStructureFlagsKHR flags)
{
    std::cout << "[INFO] Tlas building started" << std::endl;

    const auto& renderNodes = scene.getRenderNodes();
    const auto& materials = scene.getModel().materials;

    const uint32_t instanceCount = static_cast<uint32_t>(renderNodes.size());
    m_tlasInstances.reserve(instanceCount);

    for (const auto& renderNode : renderNodes) {
        VkGeometryInstanceFlagsKHR instanceFlags{};

        if (materials[renderNode.materialID].alphaMode == "OPAQUE") {
            instanceFlags |= VK_GEOMETRY_INSTANCE_FORCE_OPAQUE_BIT_KHR;
        }

        vk::AccelerationStructureInstanceKHR instance = {
            .transform = toTransformMatrixKHR(renderNode.worldMatrix),
            .instanceCustomIndex = static_cast<uint32_t>(renderNode.renderPrimID), // will be used in the shader to access render primitive data (material, texture, vertex/index buffers) InstanceID()
            .mask = 0xFF,
            .instanceShaderBindingTableRecordOffset = 0,
            .flags = instanceFlags,
            .accelerationStructureReference = m_blas[renderNode.renderPrimID].address
        };

        m_tlasInstances.push_back(instance);
    }

    // create buffer to store instances on gpu
    vk::BufferCreateInfo buffInfo = {
        .size = std::span<vk::AccelerationStructureInstanceKHR const>(m_tlasInstances).size_bytes(),
        .usage = vk::BufferUsageFlagBits::eShaderDeviceAddress | vk::BufferUsageFlagBits::eAccelerationStructureBuildInputReadOnlyKHR
    };
    VmaAllocationCreateInfo allocInfo = {
        .flags = VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT
               | VMA_ALLOCATION_CREATE_MAPPED_BIT,
        .usage = VMA_MEMORY_USAGE_AUTO,
        .requiredFlags = VK_MEMORY_PROPERTY_HOST_COHERENT_BIT | VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT
    };
    m_tlasInstancesBuffer = m_allocator.createBufferWithAlignment(buffInfo, allocInfo, 16); // must be 16 byte aligned VUID-vkCmdBuildAccelerationStructuresKHR-pInfos-03715
    memcpy(m_tlasInstancesBuffer.pMapping, m_tlasInstances.data(), std::span<vk::AccelerationStructureInstanceKHR const>(m_tlasInstances).size_bytes());

    // create geometry
    vk::BuildAccelerationStructureFlagsKHR buildFlags{enableDebugging ? vk::BuildAccelerationStructureFlagBitsKHR::ePreferFastBuild : vk::BuildAccelerationStructureFlagBitsKHR::ePreferFastTrace};
    buildFlags |= vk::BuildAccelerationStructureFlagBitsKHR::eAllowUpdate;

    m_tlasBuildData.type = vk::AccelerationStructureTypeKHR::eTopLevel;
    AccelerationStructureGeometryInfo geometry = m_tlasBuildData.makeInstanceGeometry(m_tlasInstances.size(), m_tlasInstancesBuffer.address);
    m_tlasBuildData.addGeometry(geometry);
    auto sizeInfo = m_tlasBuildData.finalizeGeometry(m_device.getVkDevice(), buildFlags);

    // create scratch buffer
    vk::PhysicalDeviceAccelerationStructurePropertiesKHR asProps = m_device.getPhysicalDevice().m_asProperties;
    vk::DeviceSize scratchAlignment = asProps.minAccelerationStructureScratchOffsetAlignment;

    vk::BufferCreateInfo scratchBuffInfo = {
        .size = sizeInfo.buildScratchSize,
        .usage = vk::BufferUsageFlagBits::eStorageBuffer | vk::BufferUsageFlagBits::eShaderDeviceAddress | vk::BufferUsageFlagBits::eAccelerationStructureStorageKHR
    };
    VmaAllocationCreateInfo scratchAllocInfo = {
        .usage = VMA_MEMORY_USAGE_AUTO_PREFER_DEVICE
    };

    m_tlasScratchBuffer = m_allocator.createBufferWithAlignment(scratchBuffInfo, scratchAllocInfo, scratchAlignment);

    // create tlas
    auto tlasCreateInfo = m_tlasBuildData.makeCreateInfo();
    m_tlas = m_allocator.createAccelerationStructure(tlasCreateInfo);

    m_tlasBuildData.cmdBuildAccelerationStructure(cmd, m_tlas.accel, m_tlasScratchBuffer.address);
}

bool GltfSceneRt::syncTLAS(const vk::raii::CommandBuffer &cmd, app::GltfScene &scene) {
    auto& df = scene.getDirtyFlags();
    const auto& dirtyNodes = df.renderNodesRtIDs;

    const auto& renderNodes = scene.getRenderNodes();
    const auto& materials = scene.getModel().materials;

    if (dirtyNodes.empty())
        return false;

    for (int idx : dirtyNodes) {
        const auto& renderNode = renderNodes[idx];
        VkGeometryInstanceFlagsKHR instanceFlags{};

        if (materials[renderNode.materialID].alphaMode == "OPAQUE") {
            instanceFlags |= VK_GEOMETRY_INSTANCE_FORCE_OPAQUE_BIT_KHR;
        }

        vk::AccelerationStructureInstanceKHR instance = {
            .transform = toTransformMatrixKHR(renderNode.worldMatrix),
            .instanceCustomIndex = static_cast<uint32_t>(renderNode.renderPrimID), // will be used in the shader to access render primitive data (material, texture, vertex/index buffers) InstanceID()
            .mask = 0xFF,
            .instanceShaderBindingTableRecordOffset = 0,
            .flags = instanceFlags,
            .accelerationStructureReference = m_blas[renderNode.renderPrimID].address
        };

        m_tlasInstances[idx] = instance;
        vk::DeviceAddress offset = static_cast<vk::DeviceAddress>(idx) * sizeof(vk::AccelerationStructureInstanceKHR);
        memcpy(m_tlasInstancesBuffer.pMapping + offset, &m_tlasInstances[idx], sizeof(vk::AccelerationStructureInstanceKHR));
    }

    df.renderNodesRtIDs.clear();
    m_tlasBuildData.cmdUpdateAccelerationStructure(cmd, m_tlas.accel, m_tlasScratchBuffer.address);
    return true;
}

AccelerationStructureGeometryInfo primitiveToGeometry(const GltfSceneVk &sceneVk,
                                                                   const app::RenderPrimitive &submesh,
                                                                   uint32_t primID)
{
    uint32_t numTriangles = submesh.indexCount / 3;

    AccelerationStructureGeometryInfo geometryInfo{};

    const auto& vertexBuffers = sceneVk.getVertexBuffers();
    const auto& indexBuffers = sceneVk.getIndexBuffers();

    vk::AccelerationStructureGeometryTrianglesDataKHR trianglesData = {
        .vertexFormat = vk::Format::eR32G32B32Sfloat,
        .vertexData = vertexBuffers[primID].address,
        .vertexStride = sizeof(shaderio::Vertex),
        .maxVertex = static_cast<uint32_t>(submesh.vertexCount - 1),
        .indexType = vk::IndexType::eUint32,
        .indexData = indexBuffers[primID].address,
    };

    vk::AccelerationStructureGeometryDataKHR geometryData(trianglesData);

    geometryInfo.geometry = {
        .geometryType = vk::GeometryTypeKHR::eTriangles,
        .geometry = geometryData,
        .flags = vk::GeometryFlagBitsKHR::eOpaque | vk::GeometryFlagBitsKHR::eNoDuplicateAnyHitInvocation
    };

    geometryInfo.rangeInfo = {
        .primitiveCount = numTriangles
    };

    return geometryInfo;
}
}
