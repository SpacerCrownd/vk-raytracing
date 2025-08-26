#ifndef VULKAN_CORE_H
#define VULKAN_CORE_H

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#include <glm/vec4.hpp>
#include <glm/mat4x4.hpp>

#include <memory>
#include <iostream>
#include <algorithm>

import vulkan_hpp;

#include "vk_rt_utils.h"

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
	void Init(const char* pAppName);

private:
	vk::raii::Context m_context;
	vk::raii::Instance m_instance = VK_NULL_HANDLE;
	vk::raii::PhysicalDevice m_physicalDevice = VK_NULL_HANDLE;
	vk::raii::DebugUtilsMessengerEXT m_debugMessenger = VK_NULL_HANDLE;

	void CreateInstance(const char* pAppName);
	void CreateDebugCallback();
	void CreatePhysicalDevice();
};

}

#endif