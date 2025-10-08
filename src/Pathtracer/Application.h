#ifndef VK_RAYTRACING_APPLICATION_H
#define VK_RAYTRACING_APPLICATION_H

#include "../Vulkan/Renderer.h"
#include "Scene.h"

namespace PathTracingVk {

class Application {
public:
	int width = 1240;
	int height = 720;
	const char* appName{};

	Application(int width, int height, const char* pAppName);
	~Application();

	void Run();

private:
	std::unique_ptr<VulkanWindow> m_mainWindow;
	std::unique_ptr<Renderer> m_vkCore;
	std::unique_ptr<Scene> m_scene;

	const vk::raii::Queue* m_queue;
	const VulkanSwapchain* m_swapchain;
	const VulkanDevice* m_device;

	int m_swapchainImageCount = 0;
	std::vector<vk::raii::CommandBuffer> m_cmdBuffs;

	std::vector<vk::raii::Semaphore> m_renderFinishedSemaphores;
	std::vector<vk::raii::Semaphore> m_presentFinishedSemaphores;
	std::vector<vk::raii::Fence> m_inFlightFences;
	uint32_t m_inFlightFrameIndex = 0;
	uint32_t m_currentImageIndex = 0;

	void CreateCommandBuffers();
	void RecordCommandBuffer(uint32_t imgIndex);
	// only useful for rasterization
	void Clear(uint32_t imgIndex);
	void ResetCommandBuffer();
	void MainLoop();
	void RenderFrame();
	void CreateSyncObjs();
};
}

#endif //VK_RAYTRACING_APPLICATION_H
