#ifndef VK_RAYTRACING_APPLICATION_H
#define VK_RAYTRACING_APPLICATION_H

#include "../Vulkan/VulkanCore.h"
#include "Scene.h"
namespace PathTracingVk {

class VulkanApp {
public:
	int width = 1240;
	int height = 720;
	const char* pAppName{};

	VulkanApp(int width, int height, const char* pAppName);

	~VulkanApp();

	void Run();

private:
	std::unique_ptr<Window> m_mainWindow;
	std::unique_ptr<VulkanCore> m_vkCore;
	std::unique_ptr<Scene> m_scene;

	VulkanQueue* m_pQueue{};
	int m_numImages = 0;
	std::vector<vk::raii::CommandBuffer> m_cmdBuffs;

	void CreateCommandBuffers();

	void RecordCommandBuffers();

	// only useful for rasterization
	void Clear();

	void MainLoop();

	void RenderFrame();
};
}

#endif //VK_RAYTRACING_APPLICATION_H
