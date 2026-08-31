#ifndef VK_RT_UTILS_H
#define VK_RT_UTILS_H

#include "Vulkan.h"
#include <vector>
#include <hash_combine/hash_combine.h>

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

vk::Format findSupportedFormat(const vk::raii::PhysicalDevice& device,
							   const std::vector<vk::Format>& candidates,
                               vk::ImageTiling tiling,
                               vk::FormatFeatureFlags features);
vk::Format findDepthFormat(const vk::raii::PhysicalDevice& device);

void imageLayoutTransition(vk::raii::CommandBuffer& cmd, vk::Image image, vk::ImageLayout currentLayout, vk::ImageLayout newLayout); // generic inefficient transition memory barriers
void imageLayoutTransition(vk::raii::CommandBuffer &cmd,
						   vk::Image image,
						   vk::PipelineStageFlags2 srcStageMask,
                           vk::PipelineStageFlags2 dstStageMask,
                           vk::AccessFlags2 srcAccessMask,
                           vk::AccessFlags2 dstAccessMask,
                           vk::ImageLayout oldLayout,
                           vk::ImageLayout newLayout,
                           const vk::ImageSubresourceRange &subresourceRange);

void copyImage(const vk::raii::CommandBuffer& cmd, vk::Image source, vk::Image destination, vk::Extent2D srcSize, vk::Extent2D dstSize);

//---- Hash Combination ----
template <typename T>
void hashCombine(std::size_t& seed, const T& val)
{
	boost::hash_combine(seed, val);
}
// Auxiliary generic functions to create a hash value using a seed
template <typename T, typename... Types>
void hashCombine(std::size_t& seed, const T& val, const Types&... args)
{
	hashCombine(seed, val);
	hashCombine(seed, args...);
}

// Generic function to create a hash value out of a heterogeneous list of arguments
template <typename... Types>
std::size_t hashVal(const Types&... args)
{
	std::size_t seed = 0;
	hashCombine(seed, args...);
	return seed;
}
}
#endif
