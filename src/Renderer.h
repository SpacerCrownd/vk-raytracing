#ifndef VK_RAYTRACING_APPLICATION_H
#define VK_RAYTRACING_APPLICATION_H

#include "vulkan/Core.h"
#include "Camera.h"
#include "GltfScene.h"
#include "vulkan/Shader.h"

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
	ptvk::Window m_mainWindow;
	ptvk::Core m_vkCore;
	GltfScene m_scene;
	Camera m_camera;

	std::optional<ptvk::Shader> m_rasterShader;
	std::optional<ptvk::Shader> m_rtShader;

	void MainLoop();

	void CreateScene();
	void LoadShaders();

	void Draw();
	void PrepareFrameData();

};
}

#endif //VK_RAYTRACING_APPLICATION_H
