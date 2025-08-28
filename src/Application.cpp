#include "Application.h"

constexpr uint32_t WINDOW_WIDTH = 1240;
constexpr uint32_t WINDOW_HEIGHT = 720;
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
	GLFWwindow* m_pMainWindow = nullptr;
	PathTracingVK::VulkanCore m_vkCore;

	void Init() {
		InitGLFW();
		InitVulkan();
	}

	void InitVulkan() {
		m_vkCore.Init(pAppName, m_pMainWindow);
	}

	void InitGLFW() {
		glfwInit();
		if (!glfwVulkanSupported()) {
			throw std::runtime_error("Vulkan is not supported on this system!");
		}

		glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
		glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);
		m_pMainWindow = glfwCreateWindow(width, height, pAppName, nullptr, nullptr);

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
