#include "VulkanQueue.h"

namespace PathTracingVK {

void VulkanQueue::Init(const uint32_t queueFamily, const uint32_t queueIndex) {
	m_queue = m_device.getQueue(queueFamily, queueIndex);
	m_numSwapchainImgs = m_swapChain.getImages().size();
	CreateSyncObjs();
}

void VulkanQueue::CreateSyncObjs() {
	m_inFlightFences.clear();
	m_presentFinishedSemaphores.clear();
	m_renderFinishedSemaphores.clear();

	// create one acquisition semaphore for each swapchain image
	for (int i = 0; i < m_numSwapchainImgs; i++) {
		m_renderFinishedSemaphores.emplace_back(m_device, vk::SemaphoreCreateInfo());
	}

	// for each in-flight frame create submit semaphores and acquisition fences
	for (int i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
		m_presentFinishedSemaphores.emplace_back(m_device, vk::SemaphoreCreateInfo());
		m_inFlightFences.emplace_back(m_device, vk::FenceCreateInfo{.flags = vk::FenceCreateFlagBits::eSignaled});
	}
}

uint32_t VulkanQueue::AcquireNextImage() {
	while (m_device.waitForFences(*m_inFlightFences[m_inFlightFrameIndex], vk::True, UINT64_MAX) == vk::Result::eTimeout);
	m_device.resetFences(*m_inFlightFences[m_inFlightFrameIndex]);

	auto [result, imageIndex] = m_swapChain.acquireNextImage(UINT64_MAX, m_presentFinishedSemaphores[m_inFlightFrameIndex]);
	/**
	 * render finished semaphore for the current in-flight frame may still be in use by the presentation engine,
	 * but we know that the render finished semaphore for the currently acquired image is safe to use (present operation has finished)
	 * so we can reuse that
	 **/
	m_currentImageIndex = imageIndex;

	return imageIndex;
}

void VulkanQueue::SubmitSync(const vk::CommandBuffer &cmdBuff) {
	vk::SubmitInfo submitInfo = {
		.sType = vk::StructureType::eSubmitInfo,
		.pNext = nullptr,
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
		.pWaitSemaphores = &*m_presentFinishedSemaphores[m_inFlightFrameIndex],
		.pWaitDstStageMask = &waitFlags,
		.commandBufferCount = 1,
		.pCommandBuffers = &cmdBuff,
		.signalSemaphoreCount = 1,
		.pSignalSemaphores = &*m_renderFinishedSemaphores[m_currentImageIndex],
	};

	m_queue.submit(submitInfo, m_inFlightFences[m_inFlightFrameIndex]);
}

void VulkanQueue::Present(uint32_t imgIndex) {
	const vk::PresentInfoKHR presentInfo = {
		.sType = vk::StructureType::ePresentInfoKHR,
		.pNext = nullptr,
		.waitSemaphoreCount = 1,
		.pWaitSemaphores = &*m_renderFinishedSemaphores[m_currentImageIndex],
		.swapchainCount = 1,
		.pSwapchains = &*m_swapChain,
		.pImageIndices = &imgIndex,
	};

	auto res = m_queue.presentKHR(presentInfo);

	if (res == vk::Result::eSuboptimalKHR || res == vk::Result::eErrorOutOfDateKHR) {

	}else {
		vk::detail::resultCheck(res, "vkQueuePresentKHR");
	}

	m_inFlightFrameIndex = (m_inFlightFrameIndex + 1) % MAX_FRAMES_IN_FLIGHT;
}
}
