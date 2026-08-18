#ifndef VK_RAYTRACING_SWAPCHAIN_H
#define VK_RAYTRACING_SWAPCHAIN_H

#include "Vulkan.h"
#include "Device.h"

#include <vector>

namespace ptvk {

class Swapchain {
public:
    Swapchain(const Device& device, vk::Extent2D extent, vk::raii::SurfaceKHR& surface);
    ~Swapchain();

    vk::raii::SwapchainKHR&       GetSwapchain() { return m_swapchain; }
    const vk::raii::SwapchainKHR& GetSwapchain() const { return m_swapchain; }
    int                           GetSwapchainImageCount() const { return static_cast<int>(m_swapchainImages.size()); }
    vk::Image                     GetSwapchainImage(int n) const { return m_swapchainImages[n]; };
    vk::Format                    GetSwapchainFormat() const { return m_swapchainSurfaceFormat.format; }
    vk::Extent2D                  GetExtent() const { return m_swapchainExtent; }

    vk::Result AcquireNextImage(const vk::raii::Semaphore& renderSemaphore, uint32_t& imageIndex) const;

private:
    vk::raii::SwapchainKHR m_swapchain{VK_NULL_HANDLE};

    const Device&                m_device;
    const vk::raii::SurfaceKHR&  m_surface;

    vk::Extent2D         m_swapchainExtent;
    vk::SurfaceFormatKHR m_swapchainSurfaceFormat;

    std::vector<vk::Image>           m_swapchainImages;
    std::vector<vk::raii::ImageView> m_swapchainImageViews;
};

}

#endif //VK_RAYTRACING_SWAPCHAIN_H