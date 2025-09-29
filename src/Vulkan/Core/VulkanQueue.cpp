#include "VulkanQueue.h"
#include "vk_rt_utils.h"

namespace PathTracingVK {

void VulkanQueue::Init(uint32_t queueFamily, uint32_t queueIndex) {
	m_queue = m_device.getQueue(queueFamily, queueIndex);
	printf("Queue acquired\n");
	CreateSemaphores();
}

void VulkanQueue::CreateSemaphores() {
	m_presentFinishedSemaphore = std::move(CreateSemaphore(m_device));
	m_renderFinishedSemaphore = std::move(CreateSemaphore(m_device));
}

void VulkanQueue::WaitIdle() {
	m_queue.waitIdle();
}

uint32_t VulkanQueue::AcquireNextImage() {
	auto [result, imageIndx] = m_swapChain.acquireNextImage(UINT64_MAX, *m_presentFinishedSemaphore, nullptr);
	return imageIndx;
}

void VulkanQueue::SubmitSync(const vk::CommandBuffer &cmdBuff) {

	vk::SubmitInfo submitInfo = {
		.sType = vk::StructureType::eSubmitInfo,
		.waitSemaphoreCount = 0,
		.pWaitSemaphores = VK_NULL_HANDLE,
		.pWaitDstStageMask = VK_NULL_HANDLE,
		.commandBufferCount = 1,
		.pCommandBuffers = &cmdBuff,
		.signalSemaphoreCount = 0,
		.pSignalSemaphores = VK_NULL_HANDLE,
	};

	m_queue.submit(submitInfo);
}

void VulkanQueue::SubmitAsync(const vk::CommandBuffer &cmdBuff) {
	vk::PipelineStageFlags waitFlags = vk::PipelineStageFlagBits::eColorAttachmentOutput;

	const vk::SubmitInfo submitInfo = {
		.sType = vk::StructureType::eSubmitInfo,
		.waitSemaphoreCount = 1,
		.pWaitSemaphores = &*m_presentFinishedSemaphore,
		.pWaitDstStageMask = &waitFlags,
		.commandBufferCount = 1,
		.pCommandBuffers = &cmdBuff,
		.signalSemaphoreCount = 1,
		.pSignalSemaphores = &*m_renderFinishedSemaphore,
	};

	m_queue.submit(submitInfo);
}

void VulkanQueue::Present(uint32_t imgIndex) {
	const vk::PresentInfoKHR presentInfo = {
		.sType = vk::StructureType::ePresentInfoKHR,
		.waitSemaphoreCount = 1,
		.pWaitSemaphores = &*m_renderFinishedSemaphore,
		.swapchainCount = 1,
		.pSwapchains = &*m_swapChain,
		.pImageIndices = &imgIndex,
	};

	auto res = m_queue.presentKHR(presentInfo);
	vk::detail::resultCheck(res, "vkQueuePresentKHR");
	WaitIdle();
}
}
