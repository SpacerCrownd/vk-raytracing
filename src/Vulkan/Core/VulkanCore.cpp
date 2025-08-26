#include "VulkanCore.h"

namespace PathTracingVK {

VulkanCore::VulkanCore() {

}

VulkanCore::~VulkanCore() {
}

void VulkanCore::Init(const char* pAppName) {
	CreateInstance(pAppName);
	CreateDebugCallback();
	CreatePhysicalDevice();
}

void VulkanCore::CreateInstance(const char* pAppName) {
	vk::ApplicationInfo appInfo{
		.pApplicationName = pAppName,
		.applicationVersion = VK_MAKE_VERSION(0, 1, 0),
		.pEngineName = "NoEngine",
		.engineVersion = VK_MAKE_VERSION(0, 1, 0),
		.apiVersion = vk::ApiVersion14
	};

	std::vector<const char*> validationLayers = {
		"VK_LAYER_KHRONOS_validation"
	};

	std::vector<const char*> extensions = {
		VK_KHR_SURFACE_EXTENSION_NAME,
		VK_EXT_DEBUG_UTILS_EXTENSION_NAME,
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

void VulkanCore::CreatePhysicalDevice() {

	auto devices = m_instance.enumeratePhysicalDevices();
	if (devices.empty()) {
		throw std::runtime_error("Failed to find GPUs with Vulkan support!");
	}
	std::cout << "Available Physical Devices:" << std::endl;
	for (const auto& device : devices) {
		// If it is useful to query for raytracing property keep using VkPhysicalDeviceProperties2
		// else revert to VkPhysicalDeviceProperties
		auto deviceProperties = device.getProperties2();

		// Other queriable stuff if ever needed
		//auto deviceFeatures = device.getFeatures2();
		//auto extensions = device.enumerateDeviceExtensionProperties();
		//auto queueFamilyProperties = device.getQueueFamilyProperties();

		// Device properties
		std::cout << '\t' << "Name: " << deviceProperties.properties.deviceName << std::endl;
		std::cout << '\t' << "API version: " << deviceProperties.properties.apiVersion << std::endl;
		std::cout << '\t' << "Driver version: " << deviceProperties.properties.driverVersion << std::endl;
		std::cout << '\t' << "Vendor ID: " << deviceProperties.properties.vendorID << std::endl;
		std::cout << '\t' << "ID: " << deviceProperties.properties.deviceID << std::endl;

		switch (deviceProperties.properties.deviceType) {
			case vk::PhysicalDeviceType::eOther: {
				std::cout << '\t' << "Type: " << "Other" << std::endl;
				break;
			}
			case vk::PhysicalDeviceType::eIntegratedGpu: {
				std::cout << '\t' << "Type: " << "Integrated GPU" << std::endl;
				break;
			}
			case vk::PhysicalDeviceType::eDiscreteGpu: {
				std::cout << '\t' << "Type: " << "Discrete GPU" << std::endl;
				break;
			}
			case vk::PhysicalDeviceType::eVirtualGpu: {
				std::cout << '\t' << "Type: " << "Virtual GPU" << std::endl;
				break;
			}
			case vk::PhysicalDeviceType::eCpu: {
				std::cout << '\t' << "Type: " << "CPU" << std::endl;
				break;
			}
		}

		/*
			std::cout << '\t' << "Supported extensions:" << std::endl;
			for (const auto& extension : extensions) {
				std::cout << "\t\t - " << extension.extensionName << std::endl;
			}
		*/

		// Check for suitability (TODO: properly check for raytracing extensions support)
		bool isSuitable = device.getProperties().apiVersion >= VK_API_VERSION_1_4;
		if (isSuitable) {
			m_physicalDevice = device;
			break;
		}
	}
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

}