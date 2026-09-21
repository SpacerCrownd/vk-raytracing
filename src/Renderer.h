#ifndef VK_RAYTRACING_APPLICATION_H
#define VK_RAYTRACING_APPLICATION_H

#include "vulkan/Core.h"
#include "vulkan/Shader.h"
#include "vulkan/GraphicsPipeline.h"
#include "vulkan/GltfSceneVulkan.h"
#include "vulkan/SamplerPool.h"
#include "vulkan/RtPipeline.h"
#include "vulkan/GltfSceneRt.h"

#include "Camera.h"
#include "GltfScene.h"

#include "shaders/shaderio.h.slang"

namespace app {
enum PipelineType {
	eRaster = 0,
	eRaytracing
};

class Renderer {
public:
	const char* appName{};

	Renderer(int width, int height, const char* pAppName);
	~Renderer();

	void run();

private:
	ptvk::Window m_window;
	ptvk::Core m_vkCore;

	std::unique_ptr<ptvk::GraphicsPipeline> m_pGraphicsPipeline{};
	std::unique_ptr<ptvk::RtPipeline>	    m_pRtPipeline{};

	std::unique_ptr<ptvk::StagingUploader>	m_pStaging{};
	std::unique_ptr<ptvk::SamplerPool>		m_pSamplerPool{};

	std::unique_ptr<ptvk::Shader> m_pRasterShader{};
	std::unique_ptr<ptvk::Shader> m_pRtShader{};

	std::array<ptvk::Buffer, MAX_FRAMES_IN_FLIGHT> m_bFrameData;

	Camera									m_camera{glm::vec3(0.0)};
	std::unique_ptr<GltfScene>				m_pScene{};
	std::unique_ptr<ptvk::GltfSceneVulkan>	m_pVkScene{};
	std::unique_ptr<ptvk::GltfSceneRt>		m_pRtScene{};

	uint32_t m_maxTextures{10000};
	uint32_t m_maxSamplers{0};

	// 2 descriptor sets
	// 0 - Textures and samplers
	// 1 - acceleration structure and output storage image
	std::array<vk::DescriptorSetLayoutBinding, 2>	m_textureDescriptorBindings;
	std::array<vk::DescriptorSetLayoutBinding, 2>	m_rtDescriptorBindings;
	vk::raii::DescriptorSetLayout					m_textureDescriptorLayout{VK_NULL_HANDLE};
	vk::raii::DescriptorSetLayout					m_rtDescriptorLayout{VK_NULL_HANDLE};
	vk::raii::DescriptorPool						m_descriptorPool{VK_NULL_HANDLE};
	vk::raii::DescriptorSet							m_textureDescriptorSet{VK_NULL_HANDLE};

	shaderio::RasterPushConstant m_rasterPushConstant{};
	shaderio::RtPushConstant     m_rtPushConstant{};

	// config parameters
	bool			m_enableDepth = true;
	PipelineType	m_currentPipeline = eRaster;

	// life cycle
	void mainLoop();
	void prepareFrameData(const vk::raii::CommandBuffer &cmd);
	void draw();
	void onResize(int width, int height);

	void handleResize();

	bool createScene(const std::filesystem::path &filename);

	void cleanupScene();

	void finalizeScene();
	void loadShaders();
	void createDescriptors();

	void pushRtDescriptors(const vk::raii::CommandBuffer &cmd);

	void createFrameDataBuffers();
	void createGraphicsPipeline();
	void createAccelerationStructures();
	void createRtPipeline();

	void initializeImGui();
};
}

#endif //VK_RAYTRACING_APPLICATION_H
