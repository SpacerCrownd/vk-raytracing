#ifndef VULKAN_QUEUE_H
#define VULKAN_QUEUE_H

#include <vector>

#include "Vulkan.h"

namespace PathTracingVK {

class VulkanQueue {
public:
	VulkanQueue(vk::raii::Device& device, vk::raii::SwapchainKHR& swapchain) : m_device(device), m_swapChain(swapchain) {}
	~VulkanQueue() = default;

	void Init(uint32_t queueFamily, uint32_t queueIndex);
	uint32_t AcquireNextImage();
	void SubmitSync(const vk::CommandBuffer& cmdBuff);
	void SubmitAsync(const vk::CommandBuffer& cmdBuff);
	void Present(uint32_t imgIndex);

private:
	void CreateSyncObjs();

	vk::raii::Device& m_device;
	vk::raii::SwapchainKHR& m_swapChain;
	vk::raii::Queue m_queue = VK_NULL_HANDLE;
	std::vector<vk::raii::Semaphore> m_renderFinishedSemaphores;
	std::vector<vk::raii::Semaphore> m_imgAvailableSemaphores;
	std::vector<vk::raii::Fence> m_fences;
	std::vector<vk::Fence> m_imagesInFlight; // for use when swapchain image acquired is out of order

	uint32_t m_numFrames = 0;
	uint32_t m_currentFrame = 0;
};

}

#endif
