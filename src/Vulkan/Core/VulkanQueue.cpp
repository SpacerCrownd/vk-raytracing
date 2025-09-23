#include "VulkanQueue.h"
#include "vk_rt_utils.h"

namespace PathTracingVK {

void VulkanQueue::Init(uint32_t queueFamily, uint32_t queueIndx) {
	m_queue = m_device.getQueue(queueFamily, queueIndx);
	printf("Queue acquired\n");
}

void VulkanQueue::CreateSemaphores() {
	m_presentFinishedSemaphore = std::move(CreateSemaphore(m_device));
	m_renderFinishedSemaphore = std::move(CreateSemaphore(m_device));
}

void VulkanQueue::WaitIdle() {
	m_queue.waitIdle();
}

uint32_t VulkanQueue::AcquireNextImage() {
	auto [result, imageIndx] = m_swapChain.acquireNextImage(UINT64_MAX, m_presentFinishedSemaphore);
	return imageIndx;
}

}