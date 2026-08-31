#ifndef VK_RAYTRACING_COMMANDS_H
#define VK_RAYTRACING_COMMANDS_H

#include "Vulkan.h"

namespace ptvk {
vk::raii::CommandPool createTransientCommandPool(const vk::raii::Device& device, uint32_t queueFamilyIndex);
vk::raii::CommandBuffer beginSingleTimeCommands(const vk::raii::Device& device, const vk::raii::CommandPool& cmdPool);

inline vk::raii::CommandBuffer createSingleTimeCommands(vk::raii::Device& device, vk::raii::CommandPool& cmdPool)
{
    auto cmd = beginSingleTimeCommands(device, cmdPool);
    return std::move(cmd);
}

vk::Result submitSingleTimeCommands(const vk::raii::CommandBuffer &cmd, const vk::raii::Device& device, const vk::raii::Queue& queue);
}


#endif //VK_RAYTRACING_COMMANDS_H