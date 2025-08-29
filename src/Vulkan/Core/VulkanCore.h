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
	vk::raii::Instance m_instance = VK_NULL_HANDLE;
	vk::raii::DebugUtilsMessengerEXT m_debugMessenger = VK_NULL_HANDLE;
	vk::raii::SurfaceKHR m_surface = VK_NULL_HANDLE;
	vk::raii::Device m_device = VK_NULL_HANDLE;

	VulkanPhysicalDevices m_physDevices;
	uint32_t m_queueFamily = 0;
	struct {
		int Major = 0;
		int Minor = 0;
		int Patch = 0;
	} m_instanceVersion;

	void CreateInstance(const char* pAppName);
	void CreateDebugCallback();
	void CreateSurface(GLFWwindow* pWindow);
	void SelectPhysicalDevice();
	void CreateLogicalDevice();
	void CreateSwapchain();
	void CreateCommandBuffers();
	void CreateRaytracingPipeline();
	void CreateGraphicsPipeline();
	void UpdateInstanceVersion();
};

}

#endif