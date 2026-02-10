#include "Commands.h"

namespace PathTracingVk {
vk::raii::CommandPool CreateTransientCommandPool(vk::raii::Device& device, uint32_t queueFamilyIndex) {
    return vk::raii::CommandPool(device, {
        .flags            = vk::CommandPoolCreateFlagBits::eTransient,
        .queueFamilyIndex = queueFamilyIndex,
    });
}

vk::raii::CommandBuffer BeginSingleTimeCommands(vk::raii::Device &device, vk::raii::CommandPool &cmdPool) {
    vk::CommandBufferAllocateInfo allocInfo{ .commandPool = *cmdPool, .level = vk::CommandBufferLevel::ePrimary, .commandBufferCount = 1 };

    vk::raii::CommandBuffer cmd = std::move(vk::raii::CommandBuffers(device, allocInfo).front());

    cmd.begin({ vk::StructureType::eCommandBufferBeginInfo, nullptr, vk::CommandBufferUsageFlagBits::eOneTimeSubmit });

    return std::move(cmd);
}

vk::Result SubmitSingleTimeCommands(vk::raii::CommandBuffer &cmd, vk::raii::Device &device, vk::raii::CommandPool &cmdPool, vk::raii::Queue &queue) {
    cmd.end();

    vk::FenceCreateInfo fenceCreateInfo{};
    vk::raii::Fence fence = vk::raii::Fence(device, fenceCreateInfo);

    vk::SubmitInfo submitInfo{
        .commandBufferCount = 1,
        .pCommandBuffers = &*cmd,
    };
    queue.submit(submitInfo, fence);
    VK_FAIL_RETURN(device.waitForFences(*fence, vk::True, UINT64_MAX), "Failed waiting for single submit fence");
    cmdPool.reset();
    return vk::Result::eSuccess;
}
}
