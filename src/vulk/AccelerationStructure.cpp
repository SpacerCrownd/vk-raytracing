#include "AccelerationStructure.h"

namespace ptvk {
void AccelerationStructureBuildData::addGeometry(const AccelerationStructureGeometryInfo &geometry) {
    geometries.push_back(geometry.geometry);
    buildRangeInfos.push_back(geometry.rangeInfo);
}

void AccelerationStructureBuildData::addGeometry(const vk::AccelerationStructureGeometryKHR &geometry, const vk::AccelerationStructureBuildRangeInfoKHR &rangeInfo) {
    geometries.push_back(geometry);
    buildRangeInfos.push_back(rangeInfo);
}

vk::AccelerationStructureBuildSizesInfoKHR AccelerationStructureBuildData::finalizeGeometry(const vk::raii::Device &device, vk::BuildAccelerationStructureFlagsKHR flags) {
    buildInfo = {
        .type = type,
        .flags = flags,
        .geometryCount = static_cast<uint32_t>(geometries.size()),
        .pGeometries = geometries.data(),
    };

    std::vector<uint32_t> maxPrimCounts(buildRangeInfos.size());
    for (size_t i = 0; i < buildRangeInfos.size(); i++) {
        maxPrimCounts[i] = buildRangeInfos[i].primitiveCount;
    }

    buildSizeInfo = device.getAccelerationStructureBuildSizesKHR(vk::AccelerationStructureBuildTypeKHR::eDevice, buildInfo, maxPrimCounts);
    return buildSizeInfo;
}

vk::AccelerationStructureCreateInfoKHR AccelerationStructureBuildData::makeCreateInfo() {
    vk::AccelerationStructureCreateInfoKHR createInfo = {
        .size = buildSizeInfo.accelerationStructureSize,
        .type = type,
    };

    return createInfo;
}

AccelerationStructureGeometryInfo AccelerationStructureBuildData::makeInstanceGeometry(size_t instances, vk::DeviceAddress instanceBufferAddress) {
    vk::AccelerationStructureGeometryInstancesDataKHR geometryInstances;
    geometryInstances.data.deviceAddress = instanceBufferAddress;

    vk::AccelerationStructureGeometryKHR geometry{};
    geometry.geometryType = vk::GeometryTypeKHR::eInstances;
    geometry.geometry.instances = geometryInstances;

    vk::AccelerationStructureBuildRangeInfoKHR buildRangeInfo = {
        .primitiveCount = static_cast<uint32_t>(instances),
    };

    AccelerationStructureGeometryInfo geometryInfo{
        .geometry = geometry,
        .rangeInfo = buildRangeInfo
    };

    return geometryInfo;
}

// callers must synchronize shader reads with the appropriate shader stage
void AccelerationStructureBuildData::cmdBuildAccelerationStructure(const vk::raii::CommandBuffer &cmd, vk::AccelerationStructureKHR as, vk::DeviceAddress scratchBufferAddr) {
    buildInfo.mode = vk::BuildAccelerationStructureModeKHR::eBuild;
    buildInfo.dstAccelerationStructure = as;
    buildInfo.scratchData.deviceAddress = scratchBufferAddr;
    buildInfo.geometryCount = static_cast<uint32_t>(geometries.size()),
    buildInfo.pGeometries = geometries.data();

    cmd.buildAccelerationStructuresKHR(buildInfo, buildRangeInfos.data());
    // synchronize future as builds/updates
    cmdAccelerationStructureBarrier(cmd, vk::AccessFlagBits2::eAccelerationStructureWriteKHR, vk::AccessFlagBits2::eAccelerationStructureReadKHR | vk::AccessFlagBits2::eAccelerationStructureWriteKHR);
}

// callers must synchronize shader reads with the appropriate shader stage
void AccelerationStructureBuildData::cmdUpdateAccelerationStructure(const vk::raii::CommandBuffer &cmd, vk::AccelerationStructureKHR as, vk::DeviceAddress scratchBufferAddr) {
    buildInfo.mode = vk::BuildAccelerationStructureModeKHR::eUpdate;
    buildInfo.srcAccelerationStructure = as;
    buildInfo.dstAccelerationStructure = as;
    buildInfo.scratchData.deviceAddress = scratchBufferAddr;
    buildInfo.geometryCount = static_cast<uint32_t>(geometries.size()),
    buildInfo.pGeometries = geometries.data();

    cmd.buildAccelerationStructuresKHR(buildInfo, buildRangeInfos.data());
    // synchronize future as builds/updates
    cmdAccelerationStructureBarrier(cmd, vk::AccessFlagBits2::eAccelerationStructureWriteKHR, vk::AccessFlagBits2::eAccelerationStructureReadKHR | vk::AccessFlagBits2::eAccelerationStructureWriteKHR);
}

}
