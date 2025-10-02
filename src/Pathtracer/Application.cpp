#include "../Vulkan/VulkanCore.h"
#include <iostream>

#include "Scene.h"

constexpr uint32_t WINDOW_WIDTH = 1240;
constexpr uint32_t WINDOW_HEIGHT = 720;
constexpr const char* APP_NAME = "vk-raytracing";

void GLFW_KeyCallback(GLFWwindow* window, int key, int scancode, int action, int mods) {
	if ((key == GLFW_KEY_ESCAPE) && (action == GLFW_PRESS)) {
		glfwSetWindowShouldClose(window, GLFW_TRUE);
	}
}

class VulkanApp {
public:
	int width = 1240;
	int height = 720;
	const char* pAppName{};

	VulkanApp(int width, int height, const char* pAppName) : m_pQueue(nullptr) {
		this->width = width;
		this->height = height;
		this->pAppName = pAppName;
	};

	~VulkanApp() {
		CleanUp();
	}

	void Run() {
		Init();
		MainLoop();
		CleanUp();
	}

private:
	GLFWwindow* m_pMainWindow{};
	Scene m_scene{};

	PathTracingVK::VulkanCore m_vkCore{};
	PathTracingVK::VulkanQueue* m_pQueue{};
	int m_numImages = 0;
	std::vector<vk::raii::CommandBuffer> m_cmdBuffs;

	void Init() {
		InitGLFW();
		InitVulkan();
	}

	void InitVulkan() {
		m_vkCore.Init(pAppName, m_pMainWindow);
		m_numImages = m_vkCore.GetNumImages();
		m_pQueue = m_vkCore.GetQueue();
		CreateCommandBuffers();
		RecordCommandBuffers();
	}

	void CreateCommandBuffers() {
		m_cmdBuffs.reserve(m_numImages);
		m_vkCore.CreateCommandBuffers(m_numImages, m_cmdBuffs);
	}

	void RecordCommandBuffers() {
		Clear();
	}

	// only useful for rasterization
	void Clear() {
		constexpr vk::ClearColorValue clearColor = {1.0f, .0f, .0f, .0f};

		constexpr vk::ImageSubresourceRange imageRange = {
			.aspectMask = vk::ImageAspectFlagBits::eColor,
			.baseMipLevel = 0,
			.levelCount = 1,
			.baseArrayLayer = 0,
			.layerCount = 1,
		};

		vk::ImageMemoryBarrier presentToClearBarrier = {
			.sType = vk::StructureType::eImageMemoryBarrier,
			.pNext = nullptr,
			.srcAccessMask = vk::AccessFlagBits::eMemoryRead,
			.dstAccessMask = vk::AccessFlagBits::eTransferWrite,
			.oldLayout = vk::ImageLayout::eUndefined,
			.newLayout = vk::ImageLayout::eTransferDstOptimal,
			.srcQueueFamilyIndex = vk::QueueFamilyIgnored,
			.dstQueueFamilyIndex = vk::QueueFamilyIgnored,
			.subresourceRange = imageRange,
		};

		vk::ImageMemoryBarrier clearToPresentBarrier = {
			.sType = vk::StructureType::eImageMemoryBarrier,
			.pNext = nullptr,
			.srcAccessMask = vk::AccessFlagBits::eTransferWrite,
			.dstAccessMask = vk::AccessFlagBits::eMemoryRead,
			.oldLayout = vk::ImageLayout::eTransferDstOptimal,
			.newLayout = vk::ImageLayout::ePresentSrcKHR,
			.srcQueueFamilyIndex = vk::QueueFamilyIgnored,
			.dstQueueFamilyIndex = vk::QueueFamilyIgnored,
			.subresourceRange = imageRange,
		};

		for (uint32_t i = 0; i < m_cmdBuffs.size(); i++) {
			presentToClearBarrier.image = m_vkCore.GetImage(static_cast<int>(i));
			clearToPresentBarrier.image = m_vkCore.GetImage(static_cast<int>(i));

			m_cmdBuffs[i].begin({vk::StructureType::eCommandBufferBeginInfo, nullptr});

			m_cmdBuffs[i].pipelineBarrier(vk::PipelineStageFlagBits::eTransfer,
				vk::PipelineStageFlagBits::eTransfer,
				vk::DependencyFlags{},
				{},
				{},
				{presentToClearBarrier});

			m_cmdBuffs[i].clearColorImage(m_vkCore.GetImage(static_cast<int>(i)), vk::ImageLayout::eTransferDstOptimal, clearColor, imageRange);

			m_cmdBuffs[i].pipelineBarrier(vk::PipelineStageFlagBits::eTransfer,
							vk::PipelineStageFlagBits::eBottomOfPipe,
							vk::DependencyFlags{},
							{},
							{},
							{clearToPresentBarrier});

			m_cmdBuffs[i].end();
		}
	}

	void InitGLFW() {
		glfwInit();
		if (!glfwVulkanSupported()) {
			throw std::runtime_error("Vulkan is not supported on this system!");
		}

		glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
		glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);
		m_pMainWindow = glfwCreateWindow(width, height, pAppName, nullptr, nullptr);
		glfwSetKeyCallback(m_pMainWindow, GLFW_KeyCallback);
		//glfwSetErrorCallback(GLFWErrorCallback);
	}

	void MainLoop() {
		/*
		auto curTime = static_cast<float>(glfwGetTime());
		int frames = 0;
		float fpsTime = 0.0f;
		*/
		while (!glfwWindowShouldClose(m_pMainWindow)) {
			/*
			auto time = static_cast<float>(glfwGetTime());
			float dt = time - curTime;
			*/
			RenderFrame();
			//curTime = time;
			glfwPollEvents();
			/*
			// calculate framerate
			frames++;
			fpsTime += dt;
			if (fpsTime >= 1.0f) {
				//printf("%d\n", frames);
				char title[256];
				snprintf(title, sizeof(title), "%s : FPS %d\n", APP_NAME, frames);
				glfwSetWindowTitle(m_pMainWindow, title);
				fpsTime = 0.0f;
				frames = 0;
			}
			*/
		}
	}

	void CleanUp() {
		// GLFW
		m_vkCore.Destroy();
		glfwDestroyWindow(m_pMainWindow);
		glfwTerminate();
	}

	void RenderFrame() {
		const uint32_t imgIndex = m_pQueue->AcquireNextImage();
		m_pQueue->SubmitAsync(*m_cmdBuffs[imgIndex]);
		m_pQueue->Present(imgIndex);
	}
};

int main() {
	auto app = VulkanApp(WINDOW_WIDTH, WINDOW_HEIGHT, APP_NAME);

	try {
		app.Run();
	}
	catch (const std::exception& e) {
		std::cerr << e.what() << std::endl;
		return EXIT_FAILURE;
	}

	return EXIT_SUCCESS;
}
