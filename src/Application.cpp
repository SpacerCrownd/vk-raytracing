#include "Application.h"

constexpr uint32_t WINDOW_WIDTH = 800;
constexpr uint32_t WINDOW_HEIGHT = 600;
constexpr const char* APP_NAME = "vk-raytracing";

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
	GLFWwindow* m_mainWindow = nullptr;
	PathTracingVK::VulkanCore m_vkCore;

	void Init() {
		InitGLFW();
		InitVulkan();
	}

	void InitVulkan() {
		m_vkCore.Init(pAppName);
	}

	void InitGLFW() {
		glfwInit();
		if (!glfwVulkanSupported()) {
			std::cerr << "Vulkan is not supported on this system!" << std::endl;
			exit(EXIT_FAILURE);
		}

		glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
		glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);
		m_mainWindow = glfwCreateWindow(width, height, pAppName, nullptr, nullptr);

		//glfwSetErrorCallback(GLFWErrorCallback);
	}

	void MainLoop() {
		while (!glfwWindowShouldClose(m_mainWindow)) {
			glfwPollEvents();
		}
	}

	void CleanUp() {
		// GLFW
		glfwDestroyWindow(m_mainWindow);
		glfwTerminate();
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
