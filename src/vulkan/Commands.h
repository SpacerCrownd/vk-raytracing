#ifndef VK_RAYTRACING_COMMANDS_H
#define VK_RAYTRACING_COMMANDS_H

#include "Utils.h"

namespace ptvk {
vk::raii::CommandPool CreateTransientCommandPool(vk::raii::Device& device, uint32_t queueFamilyIndex);
vk::raii::CommandBuffer BeginSingleTimeCommands(vk::raii::Device& device, vk::raii::CommandPool& cmdPool);

inline vk::raii::CommandBuffer CreateSingleTimeCommands(vk::raii::Device& device, vk::raii::CommandPool& cmdPool)
{
    auto cmd = BeginSingleTimeCommands(device, cmdPool);
    return std::move(cmd);
}

vk::Result SubmitSingleTimeCommands(vk::raii::CommandBuffer &cmd, vk::raii::Device& device, vk::raii::CommandPool& cmdPool, vk::raii::Queue& queue);
}


#endif //VK_RAYTRACING_COMMANDS_H