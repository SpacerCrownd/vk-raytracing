#include "Application.h"

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
	const char* pAppName;

	VulkanApp() {}

	~VulkanApp() {
		CleanUp();
	}

	void Run() {
		Init();
		MainLoop();
		CleanUp();
	}

private:
	GLFWwindow* m_pMainWindow = nullptr;

	PathTracingVK::VulkanCore m_vkCore;
	int m_numImages = 0;
	std::vector<vk::raii::CommandBuffer> m_cmdBuffs;

	void Init() {
		InitGLFW();
		InitVulkan();
	}

	void InitVulkan() {
		m_vkCore.Init(pAppName, m_pMainWindow);
		m_numImages = m_vkCore.GetNumImages();
		m_vkCore.CreateCommandBuffers(m_numImages, m_cmdBuffs);
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
		while (!glfwWindowShouldClose(m_pMainWindow)) {
			glfwPollEvents();
		}
	}

	void CleanUp() {
		// GLFW
		glfwDestroyWindow(m_pMainWindow);
		glfwTerminate();
	}

	void RenderFrame() {

	}
};

int main() {
	VulkanApp app;
	app.width = WINDOW_WIDTH;
	app.height = WINDOW_HEIGHT;
	app.pAppName = APP_NAME;

	try {
		app.Run();
	}
	catch (const std::exception& e) {
		std::cerr << e.what() << std::endl;
		return EXIT_FAILURE;
	}


	return EXIT_SUCCESS;
}
