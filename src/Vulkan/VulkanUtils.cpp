#include "VulkanUtils.h"

namespace PathTracingVk {

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

const char* GetDebugType(vk::DebugUtilsMessageTypeFlagsEXT type)
{
	if (type & vk::DebugUtilsMessageTypeFlagBitsEXT::eGeneral)
		return "General";
	if (type & vk::DebugUtilsMessageTypeFlagBitsEXT::eValidation)
		return "Validation";
	if (type & vk::DebugUtilsMessageTypeFlagBitsEXT::ePerformance)
		return "Performance";
#ifdef _WIN64
	if (type & vk::DebugUtilsMessageTypeFlagBitsEXT::eDeviceAddressBinding)
		return "Device address binding";
#endif
	throw std::runtime_error("[ERROR] Invalid type code");
}

static void PrintImageUsageFlags(const vk::ImageUsageFlags &flags) {
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

static void PrintMemoryProperty(const vk::Flags<vk::MemoryPropertyFlagBits> &flags) {
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

static vk::Format FindSupportedFormat(const vk::raii::PhysicalDevice &device, const std::vector<vk::Format> &candidates,
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

static vk::Format FindDepthFormat(const vk::raii::PhysicalDevice &device) {
	std::vector Candidates = {
		vk::Format::eD32Sfloat,
		vk::Format::eD32SfloatS8Uint,
		vk::Format::eD24UnormS8Uint
	};

	vk::Format depthFormat = FindSupportedFormat(device, Candidates, vk::ImageTiling::eOptimal,
												 vk::FormatFeatureFlagBits::eDepthStencilAttachment);

	return depthFormat;
}

}