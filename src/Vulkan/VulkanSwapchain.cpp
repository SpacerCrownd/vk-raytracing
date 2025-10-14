#include "VulkanSwapchain.h"

namespace PathTracingVk {
static uint32_t ChooseNumImages(const vk::SurfaceCapabilitiesKHR& surfaceCaps) {
    const uint32_t requestedNumImages = surfaceCaps.minImageCount + 1;
    uint32_t finalNumImages = 0;

    if (surfaceCaps.maxImageCount > 0 && requestedNumImages > surfaceCaps.maxImageCount) {
        finalNumImages = surfaceCaps.maxImageCount;
    }
    else {
        finalNumImages = requestedNumImages;
    }

    return finalNumImages;
}

static vk::PresentModeKHR ChoosePresentMode(const std::vector<vk::PresentModeKHR>& presentModes) {
    for (const auto presentMode : presentModes) {
        if (presentMode == vk::PresentModeKHR::eMailbox) {
            return presentMode;
        }
    }

    return vk::PresentModeKHR::eFifo;
}

static vk::SurfaceFormatKHR ChooseSurfaceFormatAndColorSpace(const std::vector<vk::SurfaceFormatKHR>& surfaceFormats) {
    for (const auto surfaceFormat : surfaceFormats) {
        if (surfaceFormat.format == vk::Format::eB8G8R8A8Srgb
            && surfaceFormat.colorSpace == vk::ColorSpaceKHR::eSrgbNonlinear) {
            return surfaceFormat;
            }
    }

    return surfaceFormats[0];
}

VulkanSwapchain::VulkanSwapchain(VulkanDevice& device, vk::Extent2D extent, vk::raii::SurfaceKHR& surface) : m_device(device), m_surface(surface){
    const vk::SurfaceCapabilitiesKHR& surfaceCaps = m_device.GetPhysicalDevice().m_surfaceCapabilities;

	uint32_t numImages = ChooseNumImages(surfaceCaps);
	vk::PresentModeKHR presentMode = ChoosePresentMode(m_device.GetPhysicalDevice().m_presentModes);
	m_swapchainSurfaceFormat = ChooseSurfaceFormatAndColorSpace(m_device.GetPhysicalDevice().m_surfaceFormats);

	vk::SwapchainCreateInfoKHR swapChainCreateInfo = {
		.surface = m_surface,
		.minImageCount = numImages,
		.imageFormat = m_swapchainSurfaceFormat.format,
		.imageColorSpace = m_swapchainSurfaceFormat.colorSpace,
		.imageExtent = extent,
		.imageArrayLayers = 1,
		.imageUsage =
			vk::ImageUsageFlagBits::eColorAttachment |
			vk::ImageUsageFlagBits::eTransferDst
		,
		.imageSharingMode = vk::SharingMode::eExclusive,
		.preTransform = surfaceCaps.currentTransform,
		.compositeAlpha = vk::CompositeAlphaFlagBitsKHR::eOpaque,
		.presentMode = presentMode,
		.clipped = vk::True,
		.oldSwapchain = VK_NULL_HANDLE,
	};

	//printf("WINDOW EXTENT : width %d, height %d", m_window.GetExtent().width, m_window.GetExtent().heigth);

	m_swapchain = vk::raii::SwapchainKHR(m_device.GetDevice(), swapChainCreateInfo);
	m_swapchainImages = m_swapchain.getImages();
	printf("[INFO] Swapchain Created\n");

	// image views creation
	m_swapchainImageViews.clear();
	uint32_t layerCount = 1;
	uint32_t mipLevels = 1;

	vk::ImageViewCreateInfo viewInfo = {
		.viewType = vk::ImageViewType::e2D,
		.format = m_swapchainSurfaceFormat.format,
		.components = {
			.r = vk::ComponentSwizzle::eIdentity,
			.g = vk::ComponentSwizzle::eIdentity,
			.b = vk::ComponentSwizzle::eIdentity,
			.a = vk::ComponentSwizzle::eIdentity
		},
		.subresourceRange = {
			.aspectMask = vk::ImageAspectFlagBits::eColor,
			.baseMipLevel = 0,
			.levelCount = mipLevels,
			.baseArrayLayer = 0,
			.layerCount = layerCount
		}
	};

	for (auto image : m_swapchainImages) {
		viewInfo.image = image;
		m_swapchainImageViews.emplace_back(m_device.GetDevice(), viewInfo);
	}
}

VulkanSwapchain::~VulkanSwapchain() {
	m_swapchainImageViews.clear();
}

vk::Result VulkanSwapchain::AcquireNextImage(const vk::raii::Semaphore &renderSemaphore, uint32_t &imageIndex) const {
	auto [result, index] = m_swapchain.acquireNextImage(UINT64_MAX, renderSemaphore);
	imageIndex = index;
	return result;
}
}
