#ifndef VK_RAYTRACING_APPLICATION_H
#define VK_RAYTRACING_APPLICATION_H

#include "vulkan/Core.h"
#include "vulkan/Shader.h"
#include "vulkan/GraphicsPipeline.h"
#include "vulkan/GltfSceneVulkan.h"
#include "vulkan/SamplerPool.h"
#include "vulkan/RtPipeline.h"

#include "Camera.h"
#include "GltfScene.h"
#include "vulkan/GltfSceneRt.h"

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

	void run();

private:
	ptvk::Window m_window;
	ptvk::Core m_vkCore;

	std::unique_ptr<ptvk::GraphicsPipeline> m_pGraphicsPipeline{};
	std::unique_ptr<ptvk::RtPipeline>	    m_pRtPipeline{};

	Camera									m_camera{glm::vec3(0.0)};
	GltfScene								m_scene{m_camera};
	std::unique_ptr<ptvk::GltfSceneVulkan>	m_pVkScene{};
	std::unique_ptr<ptvk::GltfSceneRt>		m_pRtScene{};

	std::unique_ptr<ptvk::StagingUploader>	m_pStaging{};
	std::unique_ptr<ptvk::SamplerPool>		m_pSamplerPool{};

	std::unique_ptr<ptvk::Shader> m_pRasterShader{};
	std::unique_ptr<ptvk::Shader> m_pRtShader{};

	std::array<ptvk::Buffer, MAX_FRAMES_IN_FLIGHT> m_bFrameData;

	uint32_t m_maxTextures{10000};
	uint32_t m_maxSamplers{0};

	std::array<vk::DescriptorSetLayoutBinding, 2>	m_textureDescriptorBindings;
	std::array<vk::DescriptorSetLayoutBinding, 2>	m_rtDescriptorBindings;
	vk::raii::DescriptorSetLayout					m_textureDescriptorLayout{nullptr};
	vk::raii::DescriptorSetLayout					m_rtDescriptorLayout{nullptr};
	vk::raii::DescriptorPool						m_descriptorPool{VK_NULL_HANDLE};
	vk::raii::DescriptorSet							m_textureDescriptorSet{VK_NULL_HANDLE};

	// config parameters
	bool			m_enableDepth = true;
	PipelineType	m_currentPipeline = eRaster;

	// life cycle
	void mainLoop();
	void prepareFrameData();
	void draw();
	void onResize();

	void handleResize();
	void createScene();
	void finalizeScene();
	void loadShaders();
	void createDescriptors();

	void pushRtDescriptorSet(const vk::raii::CommandBuffer &cmd);

	void createFrameDataBuffers();
	void createGraphicsPipeline();
	void createAccelerationStructures();
	void createRtPipeline();

	void initializeImGui();
};
}

#endif //VK_RAYTRACING_APPLICATION_H
