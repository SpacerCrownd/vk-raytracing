#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#include <glm/vec4.hpp>
#include <glm/mat4x4.hpp>

#include <iostream>
#include <vector>
#include <algorithm>

class VulkanApp {
public:
    int width;
    int height;
    char* appName;

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
    GLFWwindow* mainWindow;
    VkInstance instance;

    void Init() {
        InitGLFW();

        // --- Instance creation (minimal, no validation layers here) ---
        VkApplicationInfo appInfo{};
        appInfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
        appInfo.pApplicationName = "Extension Check Example";
        appInfo.applicationVersion = VK_MAKE_VERSION(1, 0, 0);
        appInfo.pEngineName = "No Engine";
        appInfo.engineVersion = VK_MAKE_VERSION(1, 0, 0);
        appInfo.apiVersion = VK_API_VERSION_1_4;

        VkInstanceCreateInfo createInfo{};
        createInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
        createInfo.pApplicationInfo = &appInfo;

        if (vkCreateInstance(&createInfo, nullptr, &instance) != VK_SUCCESS) {
            throw std::runtime_error("Failed to create Vulkan instance!");
        }

        // --- Enumerate physical devices ---
        uint32_t deviceCount = 0;
        vkEnumeratePhysicalDevices(instance, &deviceCount, nullptr);
        if (deviceCount == 0) {
            throw std::runtime_error("No Vulkan physical devices found!");
        }

        std::vector<VkPhysicalDevice> physicalDevices(deviceCount);
        vkEnumeratePhysicalDevices(instance, &deviceCount, physicalDevices.data());

        VkPhysicalDevice physicalDevice = physicalDevices[0];
    }

    void InitGLFW() {
        glfwInit();
        if (!glfwVulkanSupported) {
            std::cerr << "Vulkan is not supported on this system!" << std::endl;
            exit(EXIT_FAILURE);
        }

        glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
        glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);
        mainWindow = glfwCreateWindow(width, height, appName, nullptr, nullptr);

        //glfwSetErrorCallback(GLFWErrorCallback);
    }

    void MainLoop() {
        while (!glfwWindowShouldClose(mainWindow)) {
            glfwPollEvents();
        }
    }

    void CleanUp() {
        // --- Cleanup ---
        // VK
        vkDestroyInstance(instance, nullptr);

        // GLFW
        glfwDestroyWindow(mainWindow);
        glfwTerminate();
    }

    GLFWerrorfun GLFWErrorCallback() {

    }
};

int main() {
    VulkanApp app;
    app.width = 1240;
    app.height = 720;
    app.appName = "vk-rt";
    app.Run();

    return EXIT_SUCCESS;
}
