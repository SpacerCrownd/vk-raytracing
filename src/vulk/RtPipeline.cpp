#include "RtPipeline.h"
#include "../shaders/shaderio.h.slang"

#include <iostream>

namespace ptvk {
RtPipeline::RtPipeline(const Device &device,
                       const ResourceAllocator &allocator,
                       const Shader &shader,
                       std::vector<vk::DescriptorSetLayout> descLayouts
) : m_device(device), m_allocator(allocator) {
    enum StageIndices {
        eRaygen,
        eMiss,
        eClosestHit,
        eShaderGroupCount
    };

    std::array<vk::PipelineShaderStageCreateInfo, eShaderGroupCount> stageInfos{
        shader.createShaderStage(vk::ShaderStageFlagBits::eRaygenKHR, "raygenMain"),
        shader.createShaderStage(vk::ShaderStageFlagBits::eMissKHR, "missMain"),
        shader.createShaderStage(vk::ShaderStageFlagBits::eClosestHitKHR, "closesthitMain"),
    };

    vk::RayTracingShaderGroupCreateInfoKHR groupInfo{
        .generalShader = vk::ShaderUnusedKHR,
        .closestHitShader = vk::ShaderUnusedKHR,
        .anyHitShader = vk::ShaderUnusedKHR,
        .intersectionShader = vk::ShaderUnusedKHR,
    };

    std::vector<vk::RayTracingShaderGroupCreateInfoKHR> shaderGroups{};
    groupInfo.type = vk::RayTracingShaderGroupTypeKHR::eGeneral;
    groupInfo.generalShader = eRaygen;
    shaderGroups.push_back(groupInfo);

    groupInfo.type = vk::RayTracingShaderGroupTypeKHR::eGeneral;
    groupInfo.generalShader = eMiss;
    shaderGroups.push_back(groupInfo);

    groupInfo.type = vk::RayTracingShaderGroupTypeKHR::eTrianglesHitGroup;
    groupInfo.generalShader = vk::ShaderUnusedKHR;
    groupInfo.closestHitShader = eClosestHit;
    shaderGroups.push_back(groupInfo);

    vk::PushConstantRange pushConstantRange{
        .stageFlags = vk::ShaderStageFlagBits::eAll,
        .offset = 0,
        .size = sizeof(shaderio::RtPushConstant)
    };

    vk::PipelineLayoutCreateInfo layoutInfo{
        .setLayoutCount = static_cast<uint32_t>(descLayouts.size()),
        .pSetLayouts = descLayouts.data(),
        .pushConstantRangeCount = 1,
        .pPushConstantRanges = &pushConstantRange
    };

    m_pipelineLayout = device.getVkDevice().createPipelineLayout(layoutInfo);

    vk::RayTracingPipelineCreateInfoKHR pipelineInfo{
        .stageCount = static_cast<uint32_t>(stageInfos.size()),
        .pStages = stageInfos.data(),
        .groupCount = static_cast<uint32_t>(shaderGroups.size()),
        .pGroups = shaderGroups.data(),
        .maxPipelineRayRecursionDepth = 1,
        .layout = m_pipelineLayout,
    };

    m_pipeline = device.getVkDevice().createRayTracingPipelineKHR(nullptr, nullptr, pipelineInfo, nullptr);
    std::cout << "[INFO] Raytracing Pipeline Created" << std::endl;

    createShaderBindingTable(pipelineInfo);
}

void RtPipeline::bind(const vk::raii::CommandBuffer &cmdBuffer) {
    cmdBuffer.bindPipeline(vk::PipelineBindPoint::eRayTracingKHR, m_pipeline);
}

void RtPipeline::createShaderBindingTable(vk::RayTracingPipelineCreateInfoKHR &rtPipelineInfo) {
    auto alignUp = [](auto value, size_t alignment) noexcept { return ((value + alignment - 1) & ~(alignment - 1)); };

    auto& device = m_device.getVkDevice();

    const auto& physDevice = m_device.getPhysicalDevice();
    uint32_t handleSize = physDevice.m_rtProperties.shaderGroupHandleSize;
    uint32_t handleAlignment = physDevice.m_rtProperties.shaderGroupHandleAlignment;
    uint32_t handleSizeAligned = alignUp(handleSize, handleAlignment);
    uint32_t baseAlignment = physDevice.m_rtProperties.shaderGroupBaseAlignment;
    uint32_t groupCount = rtPipelineInfo.groupCount;

    std::vector<uint8_t> shaderHandles;
    size_t dataSize = handleSize * groupCount;
    shaderHandles.resize(dataSize);
    shaderHandles = m_pipeline.getRayTracingShaderGroupHandlesKHR<uint8_t>(0, groupCount, dataSize);
    uint32_t callableSize = 0;

    uint32_t raygenOffset = 0;
    uint32_t missOffset = alignUp(handleSizeAligned, baseAlignment);
    uint32_t hitOffset = alignUp(missOffset + handleSizeAligned, baseAlignment);
    uint32_t callableOffset = alignUp(hitOffset + handleSizeAligned, baseAlignment);

    size_t bufferSize = callableOffset + callableSize;

    vk::BufferCreateInfo buffInfo = {
        .size = bufferSize,
        .usage = vk::BufferUsageFlagBits::eShaderBindingTableKHR | vk::BufferUsageFlagBits::eShaderDeviceAddress
    };
    VmaAllocationCreateInfo allocInfo = {
        .flags = VMA_ALLOCATION_CREATE_MAPPED_BIT | VMA_ALLOCATION_CREATE_HOST_ACCESS_RANDOM_BIT,
        .usage = VMA_MEMORY_USAGE_AUTO_PREFER_DEVICE,
        .requiredFlags = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT
    };
    m_sbtBuffer = m_allocator.createBuffer(buffInfo, allocInfo);

    memcpy(m_sbtBuffer.pMapping + raygenOffset, shaderHandles.data() + 0 * handleSize, handleSize);
    raygenRegion.deviceAddress = m_sbtBuffer.address + raygenOffset;
    raygenRegion.stride = handleSizeAligned;
    raygenRegion.size = handleSizeAligned;

    memcpy(m_sbtBuffer.pMapping + missOffset, shaderHandles.data() + 1 * handleSize, handleSize);
    missRegion.deviceAddress = m_sbtBuffer.address + missOffset;
    missRegion.stride = handleSizeAligned;
    missRegion.size = handleSizeAligned;

    memcpy(m_sbtBuffer.pMapping + hitOffset, shaderHandles.data() + 2 * handleSize, handleSize);
    hitRegion.deviceAddress = m_sbtBuffer.address + hitOffset;
    hitRegion.stride = handleSizeAligned;
    hitRegion.size = handleSizeAligned;

    callableRegion.deviceAddress = 0;
    callableRegion.stride = 0;
    callableRegion.size = 0;

    std::cout << "[INFO] SBT Created" << std::endl;
}
}
