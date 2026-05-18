#ifndef VK_RAYTRACING_SWAPCHAIN_H
#define VK_RAYTRACING_SWAPCHAIN_H

#include "Vulkan.h"
#include "Device.h"

#include <vector>

namespace ptvk {

class Swapchain {
public:
    Swapchain(Device& device, vk::Extent2D extent, vk::raii::SurfaceKHR& surface);
    ~Swapchain();

    [[nodiscard]] vk::raii::SwapchainKHR& GetSwapchain() { return m_swapchain; }
    [[nodiscard]] int GetSwapchainImageCount() const { return static_cast<int>(m_swapchainImages.size()); }
    [[nodiscard]] vk::Image GetSwapchainImage(int n) const { return m_swapchainImages[n]; };
    [[nodiscard]] vk::Format GetSwapchainFormat() const { return m_swapchainSurfaceFormat.format; }
    [[nodiscard]] vk::Result AcquireNextImage(const vk::raii::Semaphore& renderSemaphore, uint32_t& imageIndex) const;

private:
    vk::raii::SwapchainKHR m_swapchain = VK_NULL_HANDLE;
    Device& m_device;
    vk::raii::SurfaceKHR& m_surface;

    vk::SurfaceFormatKHR m_swapchainSurfaceFormat{};
    std::vector<vk::Image> m_swapchainImages;
    std::vector<vk::raii::ImageView> m_swapchainImageViews;
};

}

#endif //VK_RAYTRACING_SWAPCHAIN_H