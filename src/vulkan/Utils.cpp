#include "Utils.h"
#include <iostream>

namespace ptvk {

const char* GetDebugSeverityStr(vk::DebugUtilsMessageSeverityFlagBitsEXT severity)
{
	switch (severity) {
	case vk::DebugUtilsMessageSeverityFlagBitsEXT::eVerbose:
		return "Verbose";

	case vk::DebugUtilsMessageSeverityFlagBitsEXT::eInfo:
		return "Info";

	case vk::DebugUtilsMessageSeverityFlagBitsEXT::eWarning:
		return "Warning";

	case vk::DebugUtilsMessageSeverityFlagBitsEXT::eError:
		return "Error";

	default:
		throw std::runtime_error("[ERROR] Invalid severity code");
	}
}

const char *GetDebugType(vk::DebugUtilsMessageTypeFlagsEXT type)
{
    std::string result;

    if (type & vk::DebugUtilsMessageTypeFlagBitsEXT::eGeneral)
        result += "General ";
    if (type & vk::DebugUtilsMessageTypeFlagBitsEXT::eValidation)
        result += "Validation ";
    if (type & vk::DebugUtilsMessageTypeFlagBitsEXT::ePerformance)
        result += "Performance ";
#ifdef _WIN64
    if (type & vk::DebugUtilsMessageTypeFlagBitsEXT::eDeviceAddressBinding)
        result += "DeviceAddressBinding ";
#endif

    if (result.empty())
        throw std::runtime_error("Invalid type code");

    return result.c_str();
}

void PrintImageUsageFlags(const vk::ImageUsageFlags &flags) {
	if (flags & vk::ImageUsageFlagBits::eTransferSrc) {
		printf("	Image usage transfer src is supported\n");
	}

	if (flags & vk::ImageUsageFlagBits::eTransferDst) {
		printf("	Image usage transfer dest is supported\n");
	}

	if (flags & vk::ImageUsageFlagBits::eSampled) {
		printf("	Image usage sampled is supported\n");
	}

	if (flags & vk::ImageUsageFlagBits::eColorAttachment) {
		printf("	Image usage color attachment is supported\n");
	}

	if (flags & vk::ImageUsageFlagBits::eDepthStencilAttachment) {
		printf("	Image usage depth stencil attachment is supported\n");
	}

	if (flags & vk::ImageUsageFlagBits::eTransientAttachment) {
		printf("	Image usage transient attachment is supported\n");
	}

	if (flags & vk::ImageUsageFlagBits::eInputAttachment) {
		printf("	Image usage input attachment is supported\n");
	}
}

void PrintMemoryProperty(const vk::Flags<vk::MemoryPropertyFlagBits> &flags) {
	if (flags & vk::MemoryPropertyFlagBits::eDeviceLocal) {
		printf("DEVICE LOCAL ");
	}

	if (flags & vk::MemoryPropertyFlagBits::eHostVisible) {
		printf("HOST VISIBLE ");
	}

	if (flags & vk::MemoryPropertyFlagBits::eHostCoherent) {
		printf("HOST COHERENT ");
	}

	if (flags & vk::MemoryPropertyFlagBits::eHostCached) {
		printf("HOST CACHED ");
	}

	if (flags & vk::MemoryPropertyFlagBits::eLazilyAllocated) {
		printf("LAZILY ALLOCATED ");
	}

	if (flags & vk::MemoryPropertyFlagBits::eProtected) {
		printf("PROTECTED ");
	}
}

vk::Format FindSupportedFormat(const vk::raii::PhysicalDevice &device, const std::vector<vk::Format> &candidates,
									  const vk::ImageTiling tiling, const vk::FormatFeatureFlags features) {
	for (const auto format: candidates) {
		vk::FormatProperties props = device.getFormatProperties(format);

		if (tiling == vk::ImageTiling::eLinear && (props.linearTilingFeatures & features) == features) {
			return format;
		}

		if (tiling == vk::ImageTiling::eOptimal && (props.optimalTilingFeatures & features) == features) {
			return format;
		}
	}

	throw std::runtime_error("failed to find supporting format!");
}

vk::Format FindDepthFormat(const vk::raii::PhysicalDevice &device) {
	std::vector Candidates = {
		vk::Format::eD32Sfloat,
		vk::Format::eD32SfloatS8Uint,
		vk::Format::eD24UnormS8Uint
	};

	vk::Format depthFormat = FindSupportedFormat(device, Candidates, vk::ImageTiling::eOptimal,
												 vk::FormatFeatureFlagBits::eDepthStencilAttachment);

	return depthFormat;
}

void PrintMemoryPropertyFlags(VkMemoryPropertyFlags flags) {
	std::cout << "VkMemoryPropertyFlags: ";

	if (flags & VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT)
		std::cout << "DEVICE_LOCAL | ";

	if (flags & VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT)
		std::cout << "HOST_VISIBLE | ";

	if (flags & VK_MEMORY_PROPERTY_HOST_COHERENT_BIT)
		std::cout << "HOST_COHERENT | ";

	if (flags & VK_MEMORY_PROPERTY_HOST_CACHED_BIT)
		std::cout << "HOST_CACHED | ";

	if (flags & VK_MEMORY_PROPERTY_LAZILY_ALLOCATED_BIT)
		std::cout << "LAZILY_ALLOCATED | ";

	if (flags & VK_MEMORY_PROPERTY_PROTECTED_BIT)
		std::cout << "PROTECTED | ";

#ifdef VK_MEMORY_PROPERTY_DEVICE_COHERENT_BIT_AMD
	if (flags & VK_MEMORY_PROPERTY_DEVICE_COHERENT_BIT_AMD)
		std::cout << "DEVICE_COHERENT_AMD | ";
#endif

#ifdef VK_MEMORY_PROPERTY_DEVICE_UNCACHED_BIT_AMD
	if (flags & VK_MEMORY_PROPERTY_DEVICE_UNCACHED_BIT_AMD)
		std::cout << "DEVICE_UNCACHED_AMD | ";
#endif

	std::cout << std::endl;
}
}