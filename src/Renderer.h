#ifndef VK_RAYTRACING_APPLICATION_H
#define VK_RAYTRACING_APPLICATION_H

#include "vulk/Core.h"
#include "vulk/Shader.h"
#include "vulk/GraphicsPipeline.h"
#include "vulk/GltfSceneVk.h"
#include "vulk/SamplerPool.h"
#include "vulk/RtPipeline.h"
#include "vulk/GltfSceneRt.h"
#include "shaders/shaderio.h.slang"
#include "GltfScene.h"

#include "Camera.h"
#include "vulk/ImguiVulkan.h"

namespace app {
enum PipelineType {
	eRaster = 0,
	eRaytracing
};

class Renderer {
public:
	const char* appName{};

	// config parameters
	bool			enableDepth = true;
	PipelineType	currentPipeline = PipelineType::eRaytracing;

	bool      enableDirLight = true;
	glm::vec3 lightRotationEuler = glm::vec3(0, 0, 90);
	glm::vec3 lightColor = glm::vec3(1.0f, 1.0f, 1.0f);
	float     lightIntensity = 5.0f;

	glm::vec3 backgroundColor = glm::vec3(1,1,1);

	int samplesPerPixel = 1;
	int bounces = 3;
	int maxSamples = 0;

	Renderer(int width, int height, const char* pAppName);
	~Renderer();

	void run();

private:
	Window m_window;
	ptvk::Core m_vkCore;

	std::unique_ptr<ptvk::GraphicsPipeline> m_pOpaqueRasterPipeline{};
	std::unique_ptr<ptvk::GraphicsPipeline> m_pPostprocessPipeline{};
	std::unique_ptr<ptvk::RtPipeline>	    m_pRtPipeline{};

	std::unique_ptr<ptvk::ImguiVulkan> m_pImgui{};

	std::unique_ptr<ptvk::StagingUploader>	m_pStaging{};
	std::unique_ptr<ptvk::SamplerPool>		m_pSamplerPool{};

	std::unique_ptr<ptvk::Shader> m_pRasterShader{};
	std::unique_ptr<ptvk::Shader> m_pRtShader{};
	std::unique_ptr<ptvk::Shader> m_pTonemapShader{};

	std::array<ptvk::Buffer, MAX_FRAMES_IN_FLIGHT> m_bFrameData;

	Camera									m_camera{glm::vec3(0.0f, 0.0f, 0.0f)};
	std::unique_ptr<GltfScene>				m_pScene{};
	std::unique_ptr<ptvk::GltfSceneVk>		m_pSceneVk{};
	std::unique_ptr<ptvk::GltfSceneRt>		m_pSceneRt{};

	uint32_t m_maxTextures{10000};
	uint32_t m_maxSamplers{0};

	int    m_frameCount{-1};
	int    m_totalSamples{0};
	double m_lastFrameTime{0};
	double m_deltaTime{0};
	bool   m_configChanged = false;
	bool   m_showGUI = true;

	// 2 descriptor sets
	// 0 - Textures and samplers
	// 1 - acceleration structure and output storage image
	std::array<vk::DescriptorSetLayoutBinding, 2>	m_textureDescriptorBindings;
	std::array<vk::DescriptorSetLayoutBinding, 2>	m_rtDescriptorBindings;
	std::array<vk::DescriptorSetLayoutBinding, 1>	m_postprocessDescriptorBindings;
	vk::raii::DescriptorSetLayout					m_textureDescriptorLayout{VK_NULL_HANDLE};
	vk::raii::DescriptorSetLayout					m_rtDescriptorLayout{VK_NULL_HANDLE};
	vk::raii::DescriptorSetLayout					m_postprocessDescriptorLayout{VK_NULL_HANDLE};
	vk::raii::DescriptorPool						m_descriptorPool{VK_NULL_HANDLE};
	vk::raii::DescriptorSet							m_textureDescriptorSet{VK_NULL_HANDLE};
	vk::raii::DescriptorSet							m_ppDescriptorSet{VK_NULL_HANDLE};

	shaderio::RasterPushConstant m_rasterPushConstant{};
	shaderio::RtPushConstant     m_rtPushConstant{};

	// life cycle
	void mainLoop();

	void update();

	void prepareFrameData(const vk::raii::CommandBuffer &cmd);

	void updateGUI();

	void draw();
	void onResize(int width, int height);

	bool createScene(const std::filesystem::path &filename);

	void changeScene(const std::filesystem::path &filename);
	void changePipeline(PipelineType type);

	void cleanupScene();

	void loadShaders();
	void createDescriptors();

	void pushRtDescriptors(const vk::raii::CommandBuffer &cmd);
	void pushPPDescriptors(const vk::raii::CommandBuffer &cmd);

	void createFrameDataBuffers();
	void createGraphicsPipeline();
	void createAccelerationStructures();
	void createRtPipeline();

	void initializeImGui();

	void resetAccumulation();
};
}

#endif //VK_RAYTRACING_APPLICATION_H
