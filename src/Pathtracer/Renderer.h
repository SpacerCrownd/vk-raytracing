#ifndef VK_RAYTRACING_APPLICATION_H
#define VK_RAYTRACING_APPLICATION_H

#include "../Vulkan/VulkanCore.h"
#include "Scene.h"

namespace PathTracingVk {

class Renderer {
public:
	int width = 1240;
	int height = 720;
	const char* appName{};

	Renderer(int width, int height, const char* pAppName);
	~Renderer();

	void Run();

private:
	std::unique_ptr<VulkanWindow> m_mainWindow;
	std::unique_ptr<VulkanCore> m_renderer;
	std::unique_ptr<Scene> m_scene;

	void MainLoop();
	void Draw();
	void Clear(); // rasterization only
};
}

#endif //VK_RAYTRACING_APPLICATION_H
