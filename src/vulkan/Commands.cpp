#include "Commands.h"

namespace ptvk {
vk::raii::CommandPool createTransientCommandPool(const vk::raii::Device &device, uint32_t queueFamilyIndex) {
    return vk::raii::CommandPool(device, {
        .flags            = vk::CommandPoolCreateFlagBits::eTransient,
        .queueFamilyIndex = queueFamilyIndex,
    });
}

vk::raii::CommandBuffer beginSingleTimeCommands(const vk::raii::Device &device, const vk::raii::CommandPool &cmdPool) {
    vk::CommandBufferAllocateInfo allocInfo{ .commandPool = *cmdPool, .level = vk::CommandBufferLevel::ePrimary, .commandBufferCount = 1 };

    vk::raii::CommandBuffer cmd = std::move(vk::raii::CommandBuffers(device, allocInfo).front());

    cmd.begin({ vk::StructureType::eCommandBufferBeginInfo, nullptr, vk::CommandBufferUsageFlagBits::eOneTimeSubmit });

    return std::move(cmd);
}

vk::raii::CommandBuffer createSingleTimeCommands(vk::raii::Device &device, vk::raii::CommandPool &cmdPool) {
    auto cmd = beginSingleTimeCommands(device, cmdPool);
    return std::move(cmd);
}

vk::Result submitSingleTimeCommands(const vk::raii::CommandBuffer& cmd, const vk::raii::Device& device, const vk::raii::Queue& queue) {
    cmd.end();
    vk::raii::Fence fence(device, vk::FenceCreateInfo{});

    vk::CommandBufferSubmitInfo cmdSubmitInfo {
        .commandBuffer = *cmd
    };

    vk::SubmitInfo2 submitInfo {
        .commandBufferInfoCount = 1,
        .pCommandBufferInfos    = &cmdSubmitInfo
    };

    queue.submit2(submitInfo, *fence);

    vk::Result result = device.waitForFences(*fence, vk::True, UINT64_MAX);
    if (result != vk::Result::eSuccess) {
        return result;
    }

    cmd.reset();

    return vk::Result::eSuccess;
}
}
