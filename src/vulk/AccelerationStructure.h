#ifndef VK_RAYTRACING_ACCELERATIONSTRUCTURE_H
#define VK_RAYTRACING_ACCELERATIONSTRUCTURE_H

#include "Vulkan.h"
#include "Resources.h"

#include <glm/glm.hpp>

namespace ptvk {
// srcAccess is set to transfer for build sync, build for AS build for update sync
inline void cmdAccelerationStructureBarrier(const vk::raii::CommandBuffer& cmd, vk::AccessFlags2 srcAccess, vk::AccessFlags2 dstAccess) {
    vk::MemoryBarrier2 barrier = {
        .srcStageMask = srcAccess == vk::AccessFlagBits2::eTransferWrite ? vk::PipelineStageFlagBits2::eTransfer : vk::PipelineStageFlagBits2::eAccelerationStructureBuildKHR,
        .srcAccessMask = srcAccess,
        .dstStageMask = vk::PipelineStageFlagBits2::eAccelerationStructureBuildKHR,
        .dstAccessMask = dstAccess
    };

    vk::DependencyInfo dependency = {
        .memoryBarrierCount = 1,
        .pMemoryBarriers = &barrier
    };

    cmd.pipelineBarrier2(dependency);
}

// convert a Mat4x4 to the matrix required by acceleration structures
inline vk::TransformMatrixKHR toTransformMatrixKHR(const glm::mat4 &matrix) {
    // VkTransformMatrixKHR uses a row major memory layout, while glm uses a column major memory layout
    glm::mat4 transpose = glm::transpose(matrix); // 16 floats
    vk::TransformMatrixKHR mat; // 12 floats
    memcpy(&mat, &transpose, sizeof(vk::TransformMatrixKHR));
    return mat;
}

struct AccelerationStructure {
    Buffer                             buffer{};
    vk::raii::AccelerationStructureKHR accel{VK_NULL_HANDLE};
    vk::DeviceAddress                  address{};
};

// contains structs used for blas/tlas creation
struct AccelerationStructureGeometryInfo {
    vk::AccelerationStructureGeometryKHR geometry{};
    vk::AccelerationStructureBuildRangeInfoKHR rangeInfo{};
};

// uses AccelerationStructureGeometryInfo to create actual geometry data -> used to compose vulk commands to build tlas/blas
struct AccelerationStructureBuildData {
    vk::AccelerationStructureTypeKHR                        type;
    std::vector<vk::AccelerationStructureGeometryKHR>       geometries;
    std::vector<vk::AccelerationStructureBuildRangeInfoKHR> buildRangeInfos;
    vk::AccelerationStructureBuildGeometryInfoKHR           buildInfo;
    vk::AccelerationStructureBuildSizesInfoKHR              buildSizeInfo;

    void addGeometry(const vk::AccelerationStructureGeometryKHR &geometry, const vk::AccelerationStructureBuildRangeInfoKHR &rangeInfo);
    void addGeometry(const AccelerationStructureGeometryInfo &geometry);

    vk::AccelerationStructureBuildSizesInfoKHR finalizeGeometry(const vk::raii::Device &device,
                                                                vk::BuildAccelerationStructureFlagsKHR flags);

    vk::AccelerationStructureCreateInfoKHR makeCreateInfo();

    // tlas instance helper function
    AccelerationStructureGeometryInfo makeInstanceGeometry(size_t instances, vk::DeviceAddress instanceBufferAddress);

    void cmdBuildAccelerationStructure(const vk::raii::CommandBuffer &cmd, vk::AccelerationStructureKHR as, vk::DeviceAddress scratchBufferAddr);
    void cmdUpdateAccelerationStructure(const vk::raii::CommandBuffer &cmd, vk::AccelerationStructureKHR as, vk::DeviceAddress scratchBufferAddr);
};

// TODO: Create helper class for budgeting and parallel blas construction
}

#endif //VK_RAYTRACING_ACCELERATIONSTRUCTURE_H
