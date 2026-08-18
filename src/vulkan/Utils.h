#ifndef VK_RT_UTILS_H
#define VK_RT_UTILS_H

#include "Vulkan.h"
#include <vector>

#define VK_CHECK_RESULT(res, msg) \
if(res != vk::Result::eSuccess) { \
	fprintf(stderr, "[ERROR] Error in %s:%d - %s, code %x", __FILE__, __LINE__, msg, res); \
	throw std::runtime_error(msg); \
}

namespace ptvk
{
const char* getDebugSeverityStr(vk::DebugUtilsMessageSeverityFlagBitsEXT severity);
const char* getDebugType(vk::DebugUtilsMessageTypeFlagsEXT type);

void printImageUsageFlags(const vk::ImageUsageFlags& flags);
void printMemoryPropertyFlags(const vk::Flags<vk::MemoryPropertyFlagBits>& flags);
void printMemoryPropertyFlags(const VkMemoryPropertyFlags &flags);

vk::Format findSupportedFormat(const vk::raii::PhysicalDevice& device, const std::vector<vk::Format>& candidates,
                               vk::ImageTiling tiling, vk::FormatFeatureFlags features);
vk::Format findDepthFormat(const vk::raii::PhysicalDevice& device);

void transitionImage(vk::raii::CommandBuffer& cmd, vk::Image image, vk::ImageLayout currentLayout, vk::ImageLayout newLayout); // generic inefficient transition memory barriers
void copyImage(vk::raii::CommandBuffer& cmd, vk::Image source, vk::Image destination, vk::Extent2D srcSize, vk::Extent2D dstSize);
}
#endif
