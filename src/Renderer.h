#ifndef VK_RAYTRACING_APPLICATION_H
#define VK_RAYTRACING_APPLICATION_H

#include "vulkan/Core.h"
#include "vulkan/Shader.h"
#include "vulkan/GraphicsPipeline.h"
#include "vulkan/GltfSceneVulkan.h"

#include "Camera.h"
#include "GltfScene.h"

namespace app {
class Renderer {
public:
	int width = 1240;
	int height = 720;
	const char* appName{};

	Renderer(int width, int height, const char* pAppName);
	~Renderer();

	void Run();

private:
	ptvk::Window m_window;
	ptvk::Core m_vkCore;

	std::optional<ptvk::GraphicsPipeline> m_graphicsPipeline;

	Scene m_scene;
	ptvk::GltfSceneVulkan m_vkScene;
	Camera m_camera;

	std::optional<ptvk::Shader> m_rasterShader;
	std::optional<ptvk::Shader> m_rtShader;

	// parameters
	bool m_enableDepth = true;

	// life cycle
	void MainLoop();
	void PrepareFrameData();
	void Draw();
	void OnResize();

	void CreateScene();

	void LoadShaders();
	void CreateGraphicsPipeline();
};
}

#endif //VK_RAYTRACING_APPLICATION_H
