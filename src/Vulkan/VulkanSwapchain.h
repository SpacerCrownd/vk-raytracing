#ifndef VK_RAYTRACING_SWAPCHAIN_H
#define VK_RAYTRACING_SWAPCHAIN_H

#include "Vulkan.h"
#include "VulkanDevice.h"

#include <vector>

namespace PathTracingVk {

class VulkanSwapchain {
public:
    VulkanSwapchain(VulkanDevice& device, vk::Extent2D extent, vk::raii::SurfaceKHR& surface);
    ~VulkanSwapchain();

    int GetSwapchainImageCount() const { return static_cast<int>(m_swapchainImages.size()); }
    vk::Image GetSwapchainImage(int n) const { return m_swapchainImages[n]; };
    [[nodiscard]] vk::Format GetSwapChainFormat() const { return m_swapchainSurfaceFormat.format; }
    [[nodiscard]] vk::Result AcquireNextImage(vk::raii::Semaphore& presentCompleteSemaphore, uint32_t& imageIndex) const;

private:
    vk::raii::SwapchainKHR m_swapchain = VK_NULL_HANDLE;
    VulkanDevice& m_device;
    vk::raii::SurfaceKHR& m_surface;

    vk::SurfaceFormatKHR m_swapchainSurfaceFormat{};
    std::vector<vk::Image> m_swapchainImages;
    std::vector<vk::raii::ImageView> m_swapchainImageViews;
};

}

#endif //VK_RAYTRACING_SWAPCHAIN_H