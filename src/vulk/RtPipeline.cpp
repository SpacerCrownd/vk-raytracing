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
        eMissShadow,
        eClosestHit,
        eClosestHitShadow,
        eAnyHit,
        eAnyHitShadow,
        eShaderStageCount
    };

    std::array<vk::PipelineShaderStageCreateInfo, eShaderStageCount> stageInfos{
        shader.createShaderStage(vk::ShaderStageFlagBits::eRaygenKHR, "raygenMain"),
        shader.createShaderStage(vk::ShaderStageFlagBits::eMissKHR, "missMain"),
        shader.createShaderStage(vk::ShaderStageFlagBits::eMissKHR, "missShadow"),
        shader.createShaderStage(vk::ShaderStageFlagBits::eClosestHitKHR, "closesthitMain"),
        shader.createShaderStage(vk::ShaderStageFlagBits::eClosestHitKHR, "closesthitShadow"),
        shader.createShaderStage(vk::ShaderStageFlagBits::eAnyHitKHR, "anyhitMain"),
        shader.createShaderStage(vk::ShaderStageFlagBits::eAnyHitKHR, "anyhitShadow"),
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

    groupInfo.type = vk::RayTracingShaderGroupTypeKHR::eGeneral;
    groupInfo.generalShader = eMissShadow;
    shaderGroups.push_back(groupInfo);

    groupInfo.type = vk::RayTracingShaderGroupTypeKHR::eTrianglesHitGroup;
    groupInfo.generalShader = vk::ShaderUnusedKHR;
    groupInfo.closestHitShader = eClosestHit;
    groupInfo.anyHitShader = eAnyHit;
    shaderGroups.push_back(groupInfo);

    groupInfo.type = vk::RayTracingShaderGroupTypeKHR::eTrianglesHitGroup;
    groupInfo.generalShader = vk::ShaderUnusedKHR;
    groupInfo.closestHitShader = eClosestHitShadow;
    groupInfo.anyHitShader = eAnyHitShadow;
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
        .maxPipelineRayRecursionDepth = 2,
        .layout = m_pipelineLayout,
    };

    m_pipeline = device.getVkDevice().createRayTracingPipelineKHR(nullptr, nullptr, pipelineInfo, nullptr);
    std::cout << "[INFO] Raytracing Pipeline Created" << std::endl;

    createShaderBindingTable(pipelineInfo);
}

void RtPipeline::bind(const vk::raii::CommandBuffer &cmdBuffer) {
    cmdBuffer.bindPipeline(vk::PipelineBindPoint::eRayTracingKHR, m_pipeline);
}

void RtPipeline::createShaderBindingTable(const vk::RayTracingPipelineCreateInfoKHR &rtPipelineInfo) {
    auto alignUp = [](auto value, size_t alignment) noexcept { return ((value + alignment - 1) & ~(alignment - 1)); };

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

    uint32_t raygenSize = handleSizeAligned;
    uint32_t missSize = 2 * handleSizeAligned;
    uint32_t hitSize = 2 * handleSizeAligned;

    uint32_t raygenStride = handleSizeAligned;
    uint32_t missStride = handleSizeAligned;
    uint32_t hitStride = handleSizeAligned;

    uint32_t raygenOffset = 0;
    uint32_t missOffset = alignUp(raygenOffset + raygenSize, baseAlignment);
    uint32_t hitOffset = alignUp(missOffset + missSize, baseAlignment);
    uint32_t callableOffset = alignUp(hitOffset + hitSize, baseAlignment);

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
    m_sbtBuffer = m_allocator.createBufferWithAlignment(buffInfo, allocInfo, baseAlignment);

    memcpy(m_sbtBuffer.pMapping, shaderHandles.data(), handleSize);
    raygenRegion.deviceAddress = m_sbtBuffer.address;
    raygenRegion.stride = raygenStride;
    raygenRegion.size = raygenSize;

    memcpy(m_sbtBuffer.pMapping + missOffset + missStride * 0, shaderHandles.data() + 1 * handleSize, handleSize); // main miss region
    memcpy(m_sbtBuffer.pMapping + missOffset + missStride * 1, shaderHandles.data() + 2 * handleSize, handleSize); // shadow miss region
    missRegion.deviceAddress = m_sbtBuffer.address + missOffset;
    missRegion.stride = missStride;
    missRegion.size = missSize;

    memcpy(m_sbtBuffer.pMapping + hitOffset + hitStride * 0, shaderHandles.data() + 3 * handleSize, handleSize); // hit region
    memcpy(m_sbtBuffer.pMapping + hitOffset + hitStride * 1, shaderHandles.data() + 4 * handleSize, handleSize); // shadow hit region
    hitRegion.deviceAddress = m_sbtBuffer.address + hitOffset;
    hitRegion.stride = hitStride;
    hitRegion.size = hitSize;

    callableRegion.deviceAddress = 0;
    callableRegion.stride = 0;
    callableRegion.size = 0;

    std::cout << "[INFO] SBT Created" << std::endl;
}
}
