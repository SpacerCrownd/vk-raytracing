#ifndef VK_RAYTRACING_COMMANDS_H
#define VK_RAYTRACING_COMMANDS_H

#include "VulkanUtils.h"

namespace PathTracingVk {
class Commands {
    vk::raii::CommandPool CreateTransientCommandPool(vk::raii::Device& device, uint32_t queueFamilyIndex);

    vk::raii::CommandBuffer CreateSingleTimeCommands(vk::raii::Device& device, vk::raii::CommandPool& cmdPool)
    {
        auto cmd = BeginSingleTimeCommands(device, cmdPool);
        return std::move(cmd);
    }

    vk::raii::CommandBuffer BeginSingleTimeCommands(vk::raii::Device& device, vk::raii::CommandPool& cmdPool);

    vk::Result SubmitSingleTimeCommands(vk::raii::CommandBuffer &cmd, vk::raii::Device& device, vk::raii::CommandPool& cmdPool, vk::raii::Queue& queue);
};
}


#endif //VK_RAYTRACING_COMMANDS_H