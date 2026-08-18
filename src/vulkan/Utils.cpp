#include "Utils.h"
#include <iostream>

namespace ptvk {

const char* getDebugSeverityStr(vk::DebugUtilsMessageSeverityFlagBitsEXT severity)
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

const char* getDebugType(vk::DebugUtilsMessageTypeFlagsEXT type)
{
	// Fixed: Return static string literals instead of returning std::string::c_str() of a local variable
	if (type & vk::DebugUtilsMessageTypeFlagBitsEXT::eGeneral)     return "General";
	if (type & vk::DebugUtilsMessageTypeFlagBitsEXT::eValidation)  return "Validation";
	if (type & vk::DebugUtilsMessageTypeFlagBitsEXT::ePerformance) return "Performance";
#ifdef _WIN64
	if (type & vk::DebugUtilsMessageTypeFlagBitsEXT::eDeviceAddressBinding) return "DeviceAddressBinding";
#endif

	throw std::runtime_error("Invalid type code");
}

void printImageUsageFlags(const vk::ImageUsageFlags &flags) {
	if (flags & vk::ImageUsageFlagBits::eTransferSrc) {
		std::cout << "	Image usage transfer src is supported" << std::endl;
	}

	if (flags & vk::ImageUsageFlagBits::eTransferDst) {
		std::cout << "	Image usage transfer dest is supported" << std::endl;
	}

	if (flags & vk::ImageUsageFlagBits::eSampled) {
		std::cout << "	Image usage sampled is supported" << std::endl;
	}

	if (flags & vk::ImageUsageFlagBits::eColorAttachment) {
		std::cout << "	Image usage color attachment is supported" << std::endl;
	}

	if (flags & vk::ImageUsageFlagBits::eDepthStencilAttachment) {
		std::cout << "	Image usage depth stencil attachment is supported" << std::endl;
	}

	if (flags & vk::ImageUsageFlagBits::eTransientAttachment) {
		std::cout << "	Image usage transient attachment is supported" << std::endl;
	}

	if (flags & vk::ImageUsageFlagBits::eInputAttachment) {
		std::cout << "	Image usage input attachment is supported" << std::endl;
	}
}

void printMemoryPropertyFlags(const vk::Flags<vk::MemoryPropertyFlagBits> &flags) {
	if (flags & vk::MemoryPropertyFlagBits::eDeviceLocal) {
		std::cout << "DEVICE LOCAL ";
	}

	if (flags & vk::MemoryPropertyFlagBits::eHostVisible) {
		std::cout << "HOST VISIBLE ";
	}

	if (flags & vk::MemoryPropertyFlagBits::eHostCoherent) {
		std::cout << "HOST COHERENT ";
	}

	if (flags & vk::MemoryPropertyFlagBits::eHostCached) {
		std::cout << "HOST CACHED ";
	}

	if (flags & vk::MemoryPropertyFlagBits::eLazilyAllocated) {
		std::cout << "LAZILY ALLOCATED ";
	}

	if (flags & vk::MemoryPropertyFlagBits::eProtected) {
		std::cout << "PROTECTED ";
	}
}

void printMemoryPropertyFlags(const VkMemoryPropertyFlags& flags) {
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

vk::Format findSupportedFormat(const vk::raii::PhysicalDevice &device, const std::vector<vk::Format> &candidates,
									  const vk::ImageTiling tiling, const vk::FormatFeatureFlags features) {
	for (const auto format: candidates) {
		vk::FormatProperties props = device.getFormatProperties(format);

		if ((tiling == vk::ImageTiling::eLinear && (props.linearTilingFeatures & features) == features) ||
			(tiling == vk::ImageTiling::eOptimal && (props.optimalTilingFeatures & features) == features)) {
			return format;
		}
	}

	throw std::runtime_error("failed to find supported format!");
}

vk::Format findDepthFormat(const vk::raii::PhysicalDevice &device) {
	std::vector candidates = {
		vk::Format::eD32Sfloat,
		vk::Format::eD32SfloatS8Uint,
		vk::Format::eD24UnormS8Uint
	};

	vk::Format depthFormat = findSupportedFormat(device, candidates, vk::ImageTiling::eOptimal,
												 vk::FormatFeatureFlagBits::eDepthStencilAttachment);

	return depthFormat;
}

// all-purpose inefficient image transition for initial testing
void transitionImage(vk::raii::CommandBuffer& cmd, vk::Image image, vk::ImageLayout currentLayout, vk::ImageLayout newLayout) {
	vk::ImageMemoryBarrier2 imageBarrier {
		.srcStageMask = vk::PipelineStageFlagBits2::eAllCommands,
		.srcAccessMask =  vk::AccessFlagBits2::eMemoryWrite,
		.dstStageMask = vk::PipelineStageFlagBits2::eAllCommands,
		.dstAccessMask = vk::AccessFlagBits2::eMemoryWrite | vk::AccessFlagBits2::eMemoryRead,
		.oldLayout = currentLayout,
		.newLayout = newLayout,
	};

	vk::ImageAspectFlags aspectMask = (newLayout == vk::ImageLayout::eDepthAttachmentOptimal) ? vk::ImageAspectFlags::BitsType::eDepth : vk::ImageAspectFlags::BitsType::eColor;
	imageBarrier.subresourceRange = {
		.aspectMask = aspectMask,
		.baseMipLevel = 0,
		.levelCount = vk::RemainingMipLevels,
		.baseArrayLayer = 0,
		.layerCount = vk::RemainingArrayLayers,
	};
	imageBarrier.image = image;

	vk::DependencyInfo depInfo {
		.imageMemoryBarrierCount = 1,
		.pImageMemoryBarriers = &imageBarrier,
	};

	cmd.pipelineBarrier2(depInfo);
}

void copyImage(vk::raii::CommandBuffer& cmd, vk::Image source, vk::Image destination, vk::Extent2D srcSize, vk::Extent2D dstSize) {
	vk::ImageBlit2 blitRegion{
		.srcSubresource = {
			.aspectMask = vk::ImageAspectFlagBits::eColor,
			.mipLevel = 0,
			.baseArrayLayer = 0,
			.layerCount = 1,
		},
		.dstSubresource = {
			.aspectMask = vk::ImageAspectFlagBits::eColor,
			.mipLevel = 0,
			.baseArrayLayer = 0,
			.layerCount = 1
		},
	};
	blitRegion.srcOffsets[0] = vk::Offset3D(0, 0, 0);
	blitRegion.srcOffsets[1] = vk::Offset3D(srcSize.width, srcSize.height, 1);
	blitRegion.dstOffsets[0] = vk::Offset3D(0, 0, 0);
	blitRegion.dstOffsets[1] = vk::Offset3D(dstSize.width, dstSize.height, 1);

	vk::BlitImageInfo2 blitInfo{
		.srcImage = source,
		.srcImageLayout = vk::ImageLayout::eTransferSrcOptimal,
		.dstImage = destination,
		.dstImageLayout = vk::ImageLayout::eTransferDstOptimal,
		.regionCount = 1,
		.pRegions = &blitRegion,
		.filter = vk::Filter::eLinear,
	};
	cmd.blitImage2(blitInfo);
}
}