#ifndef VK_RT_UTILS_H
#define VK_RT_UTILS_H

#include "Vulkan.h"
#include <vector>

#define VK_FAIL_RETURN(res, msg) \
if(res != vk::Result::eSuccess) { \
	fprintf(stderr, "[ERROR] Error in %s:%d - %s, code %x", __FILE__, __LINE__, msg, res); \
	return res; \
}

#define VK_CHECK_RESULT(res, msg) \
if(res != vk::Result::eSuccess) { \
	fprintf(stderr, "[ERROR] Error in %s:%d - %s, code %x", __FILE__, __LINE__, msg, res); \
	throw std::runtime_error(msg); \
}

namespace ptvk
{
const char* GetDebugSeverityStr(vk::DebugUtilsMessageSeverityFlagBitsEXT severity);
const char* GetDebugType(vk::DebugUtilsMessageTypeFlagsEXT type);
void PrintImageUsageFlags(const vk::ImageUsageFlags& flags);
void PrintMemoryProperty(const vk::Flags<vk::MemoryPropertyFlagBits>& flags);
vk::Format FindSupportedFormat(const vk::raii::PhysicalDevice& device, const std::vector<vk::Format>& candidates,
                               vk::ImageTiling tiling, vk::FormatFeatureFlags features);
vk::Format FindDepthFormat(const vk::raii::PhysicalDevice& device);

void PrintMemoryPropertyFlags(const VkMemoryPropertyFlags &flags);

void TransitionImage(vk::raii::CommandBuffer& cmd, vk::Image image, vk::ImageLayout currentLayout, vk::ImageLayout newLayout); // generic inefficient transition memory barriers

void CopyImage(vk::raii::CommandBuffer& cmd, vk::Image source, vk::Image destination, vk::Extent2D srcSize, vk::Extent2D dstSize);
}
#endif
