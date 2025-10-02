#include "VulkanQueue.h"

namespace PathTracingVK {

void VulkanQueue::Init(const uint32_t queueFamily, const uint32_t queueIndex) {
	m_queue = m_device.getQueue(queueFamily, queueIndex);
	m_numFrames = m_swapChain.getImages().size();
	//printf("Queue acquired\n");
	CreateSyncObjs();
}

void VulkanQueue::CreateSyncObjs() {
	m_imagesInFlight.resize(m_numFrames, VK_NULL_HANDLE);
	// for each in-flight frame create semaphores and fences
	for (int i = 0; i < m_numFrames; i++) {
		m_imgAvailableSemaphores.emplace_back(m_device, vk::SemaphoreCreateInfo());
		m_renderFinishedSemaphores.emplace_back(m_device, vk::SemaphoreCreateInfo());
		m_fences.emplace_back(m_device, vk::FenceCreateInfo(vk::StructureType::eFenceCreateInfo, nullptr,
		                                                    vk::FenceCreateFlags(vk::FenceCreateFlagBits::eSignaled)));
	}
}

uint32_t VulkanQueue::AcquireNextImage() {
	while (m_device.waitForFences(*m_fences[m_currentFrame], vk::True, UINT64_MAX) == vk::Result::eTimeout);
	m_device.resetFences(*m_fences[m_currentFrame]);

	auto [result, imageIndex] = m_swapChain.acquireNextImage(UINT64_MAX, m_imgAvailableSemaphores[m_currentFrame]);
	if (static_cast<VkFence>(m_imagesInFlight[imageIndex]) != VK_NULL_HANDLE &&
		m_imagesInFlight[imageIndex] != *m_fences[imageIndex]) {
		while (m_device.waitForFences(m_imagesInFlight[imageIndex], vk::True, UINT64_MAX)== vk::Result::eTimeout);
	}

	m_imagesInFlight[imageIndex] = m_fences[imageIndex];

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
		.pWaitSemaphores = &*m_imgAvailableSemaphores[m_currentFrame],
		.pWaitDstStageMask = &waitFlags,
		.commandBufferCount = 1,
		.pCommandBuffers = &cmdBuff,
		.signalSemaphoreCount = 1,
		.pSignalSemaphores = &*m_renderFinishedSemaphores[m_currentFrame],
	};

	m_queue.submit(submitInfo, m_fences[m_currentFrame]);
}

void VulkanQueue::Present(uint32_t imgIndex) {
	const vk::PresentInfoKHR presentInfo = {
		.sType = vk::StructureType::ePresentInfoKHR,
		.pNext = nullptr,
		.waitSemaphoreCount = 1,
		.pWaitSemaphores = &*m_renderFinishedSemaphores[m_currentFrame],
		.swapchainCount = 1,
		.pSwapchains = &*m_swapChain,
		.pImageIndices = &imgIndex,
	};

	auto res = m_queue.presentKHR(presentInfo);
	if (res == vk::Result::eSuboptimalKHR || res == vk::Result::eErrorOutOfDateKHR) {

	}else {
		vk::detail::resultCheck(res, "vkQueuePresentKHR");
	}

	m_currentFrame = (m_currentFrame + 1) % m_numFrames;
}
}
