#ifndef VULKAN_QUEUE_H
#define VULKAN_QUEUE_H

#include <vector>

#include "Vulkan.h"

namespace PathTracingVk {

class VulkanQueue {
public:
	VulkanQueue(vk::raii::Device& device, vk::raii::SwapchainKHR& swapchain, uint32_t queueFamily, uint32_t queueIndex);
	~VulkanQueue() = default;

	[[nodiscard]] uint32_t AcquireNextImage();

	void SubmitSync(const vk::CommandBuffer& cmdBuff);
	void SubmitAsync(const vk::CommandBuffer& cmdBuff);
	void Present(uint32_t imgIndex);

private:
	void CreateSyncObjs();

	vk::raii::Device& m_device;
	vk::raii::SwapchainKHR& m_swapchain;
	vk::raii::Queue m_queue = VK_NULL_HANDLE;

	std::vector<vk::raii::Semaphore> m_renderFinishedSemaphores;
	std::vector<vk::raii::Semaphore> m_presentFinishedSemaphores;
	std::vector<vk::raii::Fence> m_inFlightFences;

	uint32_t m_numSwapchainImgs = 0;
	uint32_t m_inFlightFrameIndex = 0;
	uint32_t m_currentImageIndex = 0; // current image acquired by swapchain to get presentation semaphore
};

}

#endif
