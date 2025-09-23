#ifndef VULKAN_QUEUE_H
#define VULKAN_QUEUE_H

#include "Vulkan.h"

namespace PathTracingVK {

class VulkanQueue {
public:
	VulkanQueue(vk::raii::Device& device, vk::raii::SwapchainKHR& swapchain) : m_device(device), m_swapChain(swapchain) {}
	~VulkanQueue() {}

	void Init(uint32_t queueFamily, uint32_t queueIndx);
	void Destroy();
	uint32_t AcquireNextImage();
	void SubmitSync(vk::raii::CommandBuffer& cmdBuff);
	void SubmitAsync(vk::raii::CommandBuffer& cmdBuff);
	void Present(uint32_t imgIndx);
	void WaitIdle();

private:
	void CreateSemaphores();

	vk::raii::Device& m_device;
	vk::raii::SwapchainKHR& m_swapChain;
	vk::raii::Queue m_queue = VK_NULL_HANDLE;
	vk::raii::Semaphore m_renderFinishedSemaphore = VK_NULL_HANDLE;
	vk::raii::Semaphore m_presentFinishedSemaphore = VK_NULL_HANDLE;
};

}

#endif
