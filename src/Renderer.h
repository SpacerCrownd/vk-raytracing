#ifndef VK_RAYTRACING_APPLICATION_H
#define VK_RAYTRACING_APPLICATION_H

#include "vulkan/Core.h"
#include "vulkan/Shader.h"
#include "vulkan/GraphicsPipeline.h"
#include "vulkan/GltfSceneVulkan.h"
#include "vulkan/SamplerPool.h"

#include "Camera.h"
#include "GltfScene.h"

namespace app {
enum PipelineType {
	eRaster = 0,
	eRaytracing
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
	ptvk::Window m_window;
	ptvk::Core m_vkCore;

	std::unique_ptr<ptvk::GraphicsPipeline> m_pGraphicsPipeline{};

	GltfScene							   m_scene{};
	Camera								   m_camera{glm::vec3(0.0)};
	std::unique_ptr<ptvk::GltfSceneVulkan> m_pVkScene{};

	std::unique_ptr<ptvk::SamplerPool> m_pSamplerPool{};

	std::unique_ptr<ptvk::Shader> m_pRasterShader{};
	std::unique_ptr<ptvk::Shader> m_pRtShader{};

	// config parameters
	bool m_enableDepth = true;
	PipelineType m_currentPipeline = eRaster;

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
