#ifndef VULKAN_CORE_H
#define VULKAN_CORE_H

#include "Vulkan.h"

#include "vk_rt_utils.h"
#include "vk_mem_alloc.h"
#include "VulkanPhysicalDevices.h"

#include <iostream>

namespace PathTracingVK {

#ifdef NDEBUG
constexpr bool enableValidationLayers = false;
#else
constexpr bool enableValidationLayers = true;
#endif

class VulkanCore {

public:
	VulkanCore();
	~VulkanCore();
	void Init(const char* pAppName, GLFWwindow* pWindow);

private:
	vk::raii::Context m_context;
	vk::raii::Instance m_instance = nullptr;
	vk::raii::DebugUtilsMessengerEXT m_debugMessenger = nullptr;
	vk::raii::SurfaceKHR m_surface = nullptr;

	VulkanPhysicalDevices m_physDevices;
	uint32_t m_queueFamily = 0;

	void CreateInstance(const char* pAppName);
	void CreateDebugCallback();
	void CreateSurface(GLFWwindow* pWindow);
	void SelectPhysicalDevice();
	void CreateLogicalDevice();
	void CreateSwapchain();
	void CreateCommandBuffers();
	void CreateRaytracingPipeline();
	void CreateGraphicsPipeline();
};

}

#endif