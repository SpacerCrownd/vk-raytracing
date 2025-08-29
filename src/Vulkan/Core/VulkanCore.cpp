#include "VulkanCore.h"

#include <algorithm>

namespace PathTracingVK {

VulkanCore::VulkanCore() {

}

VulkanCore::~VulkanCore() {

}

void VulkanCore::Init(const char* pAppName, GLFWwindow* pWindow) {
	CreateInstance(pAppName);
	if (enableValidationLayers) { CreateDebugCallback(); }
	CreateSurface(pWindow);
	SelectPhysicalDevice();
	CreateLogicalDevice();
}

void VulkanCore::CreateInstance(const char* pAppName) {
	UpdateInstanceVersion();

	vk::ApplicationInfo appInfo{
		.pApplicationName = pAppName,
		.applicationVersion = vk::makeVersion(0, 1, 0),
		.pEngineName = "NoEngine",
		.engineVersion = vk::makeVersion(0, 1, 0),
		.apiVersion = vk::makeApiVersion(0, m_instanceVersion.Major, m_instanceVersion.Minor, m_instanceVersion.Patch),
	};

	std::vector<const char*> validationLayers = {
		"VK_LAYER_KHRONOS_validation"
	};

	std::vector<const char*> extensions = {
		vk::KHRSurfaceExtensionName,
		vk::EXTDebugUtilsExtensionName,
#if defined (_WIN32)
			"VK_KHR_win32_surface",
#endif
#if defined(__APPLE__)
			"VK_MVK_macos_surface",
#endif
#if defined(__linux__)
			"VK_KHR_xcb_surface",
#endif
	};

	// List available instance layers
	auto layerProperties = m_context.enumerateInstanceLayerProperties();
	std::cout << "Available instance layer:" << std::endl;
	for (const auto& layer : layerProperties) {
		std::cout << '\t' << layer.layerName << std::endl;
	}

	// List available instance extensions
	auto extensionProperties = m_context.enumerateInstanceExtensionProperties();
	std::cout << "Available instance extensions:" << std::endl;
	for (const auto& extension : extensionProperties) {
		std::cout << '\t' << extension.extensionName << std::endl;
	}

	// Get the required instance layers
	std::vector<char const*> requiredLayers;
	if (enableValidationLayers) {
		requiredLayers.assign(validationLayers.begin(), validationLayers.end());
	}

	// Check if the required layers are supported by the Vulkan implementation
	if (std::ranges::any_of(requiredLayers, [&layerProperties](auto const& requiredLayer) {
		return std::ranges::none_of(layerProperties,
			[requiredLayer](auto const& layerProperty)
		{ return strcmp(layerProperty.layerName, requiredLayer) == 0; });
	}))
	{
		throw std::runtime_error("One or more required layers are not supported!");
	}

	vk::InstanceCreateInfo createInfo{
		.pNext = nullptr,
		.pApplicationInfo = &appInfo,
		.enabledLayerCount = uint32_t(requiredLayers.size()),
		.ppEnabledLayerNames = requiredLayers.data(),
		.enabledExtensionCount = uint32_t(extensions.size()),
		.ppEnabledExtensionNames = extensions.data(),
	};

	m_instance = vk::raii::Instance(m_context, createInfo);
	std::cout << std::endl;
}

void VulkanCore::CreateSurface(GLFWwindow* pWindow) {
	VkSurfaceKHR surface;
	if (glfwCreateWindowSurface(*m_instance, pWindow, nullptr, &surface) != 0) {
		throw std::runtime_error("Failed to create window surface!");
	}
	m_surface = vk::raii::SurfaceKHR(m_instance, surface);
}

void VulkanCore::SelectPhysicalDevice() {
	m_physDevices.Init(m_instance, m_surface);
	m_queueFamily = m_physDevices.SelectDevice(vk::QueueFlagBits::eGraphics, true);
}

static VKAPI_ATTR vk::Bool32 VKAPI_CALL DebugCallback(
	vk::DebugUtilsMessageSeverityFlagBitsEXT severity, 
	vk::DebugUtilsMessageTypeFlagsEXT type, 
	const vk::DebugUtilsMessengerCallbackDataEXT* pCallbackData, 
	void*	) {
	printf("Debug callback: %s\n", pCallbackData->pMessage);
	printf(" Severity %s\n", GetDebugSeverityStr(severity));
	printf(" Type %s", GetDebugType(type));
	printf(" Objects ");

	for (uint32_t i = 0; i < pCallbackData->objectCount; i++) {
		printf("%llx ", pCallbackData->pObjects[i].objectHandle);
	}

	return vk::False;
}

void VulkanCore::CreateDebugCallback() {
	vk::DebugUtilsMessengerCreateInfoEXT msgCreateInfo{
		.messageSeverity = vk::DebugUtilsMessageSeverityFlagBitsEXT::eVerbose |
						   vk::DebugUtilsMessageSeverityFlagBitsEXT::eInfo |
						   vk::DebugUtilsMessageSeverityFlagBitsEXT::eWarning |
						   vk::DebugUtilsMessageSeverityFlagBitsEXT::eError,
		.messageType = vk::DebugUtilsMessageTypeFlagBitsEXT::eGeneral |
					   vk::DebugUtilsMessageTypeFlagBitsEXT::eValidation |
					   vk::DebugUtilsMessageTypeFlagBitsEXT::ePerformance,
		.pfnUserCallback = &DebugCallback,
		.pUserData = nullptr,
	};

	m_debugMessenger = m_instance.createDebugUtilsMessengerEXT(msgCreateInfo);
}

void VulkanCore::CreateLogicalDevice() {
	float qPriorities[] = { 1.0f };

	vk::DeviceQueueCreateInfo qInfo = {
		.queueFamilyIndex = m_queueFamily,
		.queueCount = 1,
		.pQueuePriorities = &qPriorities[0]
	};

	std::vector<const char*> devExtensions = {
		vk::KHRShaderDrawParametersExtensionName,
		vk::KHRSwapchainExtensionName,
		vk::KHRSpirv14ExtensionName,
		vk::KHRSynchronization2ExtensionName,
		vk::KHRCreateRenderpass2ExtensionName,
		vk::KHRRayTracingPipelineExtensionName,
		vk::KHRAccelerationStructureExtensionName,
		vk::KHRDeferredHostOperationsExtensionName
	};

	bool deviceSupportsDynamicRendering = m_physDevices.Selected().IsExtensionSupported(vk::KHRDynamicRenderingExtensionName);
	
	bool instance_is_1_3_or_more = (m_instanceVersion.Major >= 1) || (m_instanceVersion.Minor >= 3);

	if (instance_is_1_3_or_more && deviceSupportsDynamicRendering) {
		printf("The Vulkan instance and device support dynamic rendering as a core feature\n");
	}
	else if (m_instanceVersion.Minor == 2) {
		if (deviceSupportsDynamicRendering) {
			devExtensions.push_back(vk::KHRDynamicRenderingExtensionName);
		}
		else {
			throw std::runtime_error("The system doesn't support dynamic rendering");
		}
	}
	else {
		throw std::runtime_error("The system doesn't support dynamic rendering");
	}

	if (m_physDevices.Selected().m_features.geometryShader == vk::False) {
		throw std::runtime_error("The Geometry Shader is not supported!");
	}

	if (m_physDevices.Selected().m_features.tessellationShader == vk::False) {
		throw std::runtime_error("The Tessellation Shader is not supported!");
	}

	vk::StructureChain<vk::PhysicalDeviceFeatures2, vk::PhysicalDeviceVulkan13Features, vk::PhysicalDeviceExtendedDynamicStateFeaturesEXT> featureChain = {
		{},
		{.dynamicRendering = true},
		{.extendedDynamicState = true}
	};

	featureChain.get<vk::PhysicalDeviceFeatures2>().features.geometryShader = vk::True;
	featureChain.get<vk::PhysicalDeviceFeatures2>().features.tessellationShader = vk::True;

	vk::DeviceCreateInfo DeviceCreateInfo = {
		.pNext = &featureChain.get<vk::PhysicalDeviceFeatures2>(),
		.queueCreateInfoCount = 1,
		.pQueueCreateInfos = &qInfo,
		.enabledExtensionCount = (uint32_t)devExtensions.size(),
		.ppEnabledExtensionNames = devExtensions.data(),
	};

	m_device = vk::raii::Device(m_physDevices.Selected().m_physDevice, DeviceCreateInfo);

	printf("\nDevice created\n");
}

void VulkanCore::CreateSwapchain() {

}

void VulkanCore::CreateCommandBuffers() {

}

void VulkanCore::UpdateInstanceVersion() {
	uint32_t instanceVersion = m_context.enumerateInstanceVersion();

	m_instanceVersion.Major = vk::apiVersionMajor(instanceVersion);
	m_instanceVersion.Minor = vk::apiVersionMinor(instanceVersion);
	m_instanceVersion.Patch = vk::apiVersionPatch(instanceVersion);

	printf("Vulkan loader supports version %d.%d.%d\n", m_instanceVersion.Major, m_instanceVersion.Minor, m_instanceVersion.Patch);
}

}