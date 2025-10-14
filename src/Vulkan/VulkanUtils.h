#ifndef VK_RT_UTILS_H
#define VK_RT_UTILS_H

#include "Vulkan.h"
#include <vector>

#define CHECK_VK_RESULT(res, msg) \
if(res != VK_SUCCESS) { \
	fprintf(stderr, "[ERROR] Error in %s:%d - %s, code %x", __FILE__, __LINE__, msg, res); \
	throw std::runtime_error(msg); \
}

namespace PathTracingVk
{
const char* GetDebugSeverityStr(vk::DebugUtilsMessageSeverityFlagBitsEXT severity);
const char* GetDebugType(vk::DebugUtilsMessageTypeFlagsEXT type);
void PrintImageUsageFlags(const vk::ImageUsageFlags& flags);
void PrintMemoryProperty(const vk::Flags<vk::MemoryPropertyFlagBits>& flags);
vk::Format FindSupportedFormat(const vk::raii::PhysicalDevice& device, const std::vector<vk::Format>& candidates,
                               vk::ImageTiling tiling, vk::FormatFeatureFlags features);
vk::Format FindDepthFormat(const vk::raii::PhysicalDevice& device);
}
#endif
