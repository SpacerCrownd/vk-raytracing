#ifndef VK_RAYTRACING_APPLICATION_H
#define VK_RAYTRACING_APPLICATION_H

#include "../Vulkan/VulkanCore.h"
#include "../Vulkan/Scene.h"
#include "Camera.h"
#include "../Vulkan/Model.h"

namespace PathTracingVk {

struct CameraData {
	glm::mat4 projection;
	glm::mat4 view;
	glm::mat3 position;
};

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
	std::unique_ptr<VulkanCore> m_vkCore;
	// TODO: std::unique_ptr<Scene> m_scene;
	std::vector<Model> m_models{};
	std::unique_ptr<Camera> m_camera;

	void MainLoop();

	void LoadScene();

	void Draw();
	void PrepareFrameData();
};
}

#endif //VK_RAYTRACING_APPLICATION_H
