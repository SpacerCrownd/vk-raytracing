#include "VulkanCore.h"
#include <algorithm>
#include <iostream>

#include "Utils.h"
#include "../Pathtracer/Scene.h"

namespace PathTracingVK {

static VKAPI_ATTR vk::Bool32 VKAPI_CALL DebugCallback(vk::DebugUtilsMessageSeverityFlagBitsEXT severity, vk::DebugUtilsMessageTypeFlagsEXT type,const vk::DebugUtilsMessengerCallbackDataEXT* pCallbackData, void*) {
	printf("[Debug Callback]\n %s\n", pCallbackData->pMessage);
	printf("\tSeverity: %s\n", GetDebugSeverityStr(severity));
	printf("\tType: %s", GetDebugType(type));
	printf(" Objects ");

	for (uint32_t i = 0; i < pCallbackData->objectCount; i++) {
		printf("%llx ", pCallbackData->pObjects[i].objectHandle);
	}
	printf("\n");

	return vk::False;
}

static uint32_t ChooseNumImages(const vk::SurfaceCapabilitiesKHR& surfaceCaps) {
	const uint32_t requestedNumImages = surfaceCaps.minImageCount + 1;
	uint32_t finalNumImages = 0;

	if (surfaceCaps.maxImageCount > 0 && requestedNumImages > surfaceCaps.maxImageCount) {
		finalNumImages = surfaceCaps.maxImageCount;
	}
	else {
		finalNumImages = requestedNumImages;
	}

	return finalNumImages;
}

static vk::PresentModeKHR ChoosePresentMode(const std::vector<vk::PresentModeKHR>& presentModes) {
	for (const auto presentMode : presentModes) {
		if (presentMode == vk::PresentModeKHR::eMailbox) {
			return presentMode;
		}
	}

	return vk::PresentModeKHR::eFifo;
}

static vk::SurfaceFormatKHR ChooseSurfaceFormatAndColorSpace(const std::vector<vk::SurfaceFormatKHR>& surfaceFormats) {
	for (const auto surfaceFormat : surfaceFormats) {
		if (surfaceFormat.format == vk::Format::eB8G8R8A8Srgb
			&& surfaceFormat.colorSpace == vk::ColorSpaceKHR::eSrgbNonlinear) {
			return surfaceFormat;
		}
	}

	return surfaceFormats[0];
}

VulkanCore::VulkanCore() = default;

VulkanCore::~VulkanCore() {
	vmaDestroyAllocator(m_allocator);
}

void VulkanCore::Init(const char* pAppName, GLFWwindow* pWindow) {
	CreateInstance(pAppName);
	if (enableValidationLayers) { CreateDebugCallback(); }
	CreateSurface(pWindow);
	SelectPhysicalDevice();
	CreateLogicalDevice();
	InitVmaAllocator();
	CreateSwapchain();
	CreateCommandPool();
	m_queue = std::make_unique<VulkanQueue>(m_device, m_swapChain);
	m_queue->Init(m_queueFamily, 0);
}

void VulkanCore::UpdateInstanceVersion() {
	uint32_t instanceVersion = m_context.enumerateInstanceVersion();

	m_instanceVersion.Major = static_cast<int>(vk::apiVersionMajor(instanceVersion));
	m_instanceVersion.Minor = static_cast<int>(vk::apiVersionMinor(instanceVersion));
	m_instanceVersion.Patch = static_cast<int>(vk::apiVersionPatch(instanceVersion));

	printf("[INFO] Vulkan loader supports version %d.%d.%d\n", m_instanceVersion.Major, m_instanceVersion.Minor, m_instanceVersion.Patch);
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

	std::vector<const char*> layers = {
		"VK_LAYER_KHRONOS_validation",
		"VK_LAYER_LUNARG_monitor"
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
		requiredLayers.assign(layers.begin(), layers.end());
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
	printf("\n[INFO] Instance Created\n");
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
	printf("[INFO] Debug Messenger Created\n");
}

void VulkanCore::CreateSurface(GLFWwindow* pWindow) {
	VkSurfaceKHR surface;
	if (glfwCreateWindowSurface(*m_instance, pWindow, nullptr, &surface) != 0) {
		throw std::runtime_error("Failed to create window surface!");
	}
	m_surface = vk::raii::SurfaceKHR(m_instance, surface);
	printf("[INFO] Surface created\n");
}

void VulkanCore::SelectPhysicalDevice() {
	m_physDevices.Init(m_instance, m_surface);
	m_queueFamily = m_physDevices.SelectDevice(vk::QueueFlagBits::eGraphics, true);
	printf("[INFO] Physical Device selected: %s\n", m_physDevices.Selected().m_devProperties2.properties.deviceName.data());
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

	/*
	bool deviceSupportsDynamicRendering = m_physDevices.Selected().IsExtensionSupported(vk::KHRDynamicRenderingExtensionName);
	bool instance_is_1_3_or_more = (m_instanceVersion.Major >= 1) || (m_instanceVersion.Minor >= 3);
	if (instance_is_1_3_or_more && deviceSupportsDynamicRendering) {
		printf("[INFO] The Vulkan instance and device support dynamic rendering as a core feature\n");
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
	*/

	vk::StructureChain<
		vk::PhysicalDeviceFeatures2,
		vk::PhysicalDeviceVulkan13Features,
		vk::PhysicalDeviceExtendedDynamicStateFeaturesEXT,
		vk::PhysicalDeviceRayTracingPipelineFeaturesKHR,
		vk::PhysicalDeviceAccelerationStructureFeaturesKHR
	> featureChain = {
		{},
		{.dynamicRendering = true},
		{.extendedDynamicState = true},
		{.rayTracingPipeline = true},
		{.accelerationStructure = true},
	};

	featureChain.get<vk::PhysicalDeviceFeatures2>().features.geometryShader = vk::True;
	featureChain.get<vk::PhysicalDeviceFeatures2>().features.tessellationShader = vk::True;

	vk::DeviceCreateInfo deviceCreateInfo = {
		.pNext = &featureChain.get<vk::PhysicalDeviceFeatures2>(),
		.queueCreateInfoCount = 1,
		.pQueueCreateInfos = &qInfo,
		.enabledExtensionCount = static_cast<uint32_t>(devExtensions.size()),
		.ppEnabledExtensionNames = devExtensions.data(),
	};

	m_device = vk::raii::Device(m_physDevices.Selected().m_physDevice, deviceCreateInfo);

	printf("\n[INFO] Device created\n");
}

void VulkanCore::InitVmaAllocator() {
	VmaVulkanFunctions vulkanFunctions = {};
	vulkanFunctions.vkGetInstanceProcAddr = &vkGetInstanceProcAddr;
	vulkanFunctions.vkGetDeviceProcAddr = &vkGetDeviceProcAddr;

	VmaAllocatorCreateInfo allocatorCreateInfo = {};
	allocatorCreateInfo.flags = VMA_ALLOCATOR_CREATE_EXT_MEMORY_BUDGET_BIT;
	allocatorCreateInfo.vulkanApiVersion = VK_API_VERSION_1_2;
	allocatorCreateInfo.physicalDevice = static_cast<VkPhysicalDevice>(*m_physDevices.Selected().m_physDevice);
	allocatorCreateInfo.device = static_cast<VkDevice>(*m_device);
	allocatorCreateInfo.instance = static_cast<VkInstance>(*m_instance);
	allocatorCreateInfo.pVulkanFunctions = &vulkanFunctions;

	VmaAllocator allocator;
	vmaCreateAllocator(&allocatorCreateInfo, &allocator);
}

void VulkanCore::CreateSwapchain() {
	const vk::SurfaceCapabilitiesKHR& surfaceCaps = m_physDevices.Selected().m_surfaceCapabilities;
	uint32_t numImages = ChooseNumImages(surfaceCaps);

	const std::vector<vk::PresentModeKHR>& presentModes = m_physDevices.Selected().m_presentModes;
	vk::PresentModeKHR presentMode = ChoosePresentMode(presentModes);

	m_swapChainSurfaceFormat = ChooseSurfaceFormatAndColorSpace(m_physDevices.Selected().m_surfaceFormats);

	vk::SwapchainCreateInfoKHR swapChainCreateInfo = {
		.surface = m_surface,
		.minImageCount = numImages,
		.imageFormat = m_swapChainSurfaceFormat.format,
		.imageColorSpace = m_swapChainSurfaceFormat.colorSpace,
		.imageExtent = surfaceCaps.currentExtent,
		.imageArrayLayers = 1,
		.imageUsage =
			vk::ImageUsageFlagBits::eColorAttachment |
			vk::ImageUsageFlagBits::eTransferDst
		,
		.imageSharingMode = vk::SharingMode::eExclusive,
		.queueFamilyIndexCount = 1,
		.pQueueFamilyIndices = &m_queueFamily,
		.preTransform = surfaceCaps.currentTransform,
		.compositeAlpha = vk::CompositeAlphaFlagBitsKHR::eOpaque,
		.presentMode = presentMode,
		.clipped = vk::True,
		.oldSwapchain = VK_NULL_HANDLE,
	};

	m_swapChain = vk::raii::SwapchainKHR(m_device, swapChainCreateInfo);
	m_swapChainImages = m_swapChain.getImages();
	printf("[INFO] Swapchain Created\n");

	// image views creation
	m_swapChainImageViews.clear();
	uint32_t layerCount = 1;
	uint32_t mipLevels = 1;

	vk::ImageViewCreateInfo viewInfo = {
		.viewType = vk::ImageViewType::e2D,
		.format = m_swapChainSurfaceFormat.format,
		.components = {
			.r = vk::ComponentSwizzle::eIdentity,
			.g = vk::ComponentSwizzle::eIdentity,
			.b = vk::ComponentSwizzle::eIdentity,
			.a = vk::ComponentSwizzle::eIdentity
		},
		.subresourceRange = {
			.aspectMask = vk::ImageAspectFlagBits::eColor,
			.baseMipLevel = 0,
			.levelCount = mipLevels,
			.baseArrayLayer = 0,
			.layerCount = layerCount
		}
	};

	for (auto image : m_swapChainImages) {
		viewInfo.image = image;
		m_swapChainImageViews.emplace_back(m_device, viewInfo);
	}
}

void VulkanCore::CreateCommandPool() {
	vk::CommandPoolCreateInfo cmdPoolCreateInfo = {
		.queueFamilyIndex = m_queueFamily,
	};

	m_cmdPool = vk::raii::CommandPool(m_device, cmdPoolCreateInfo);
	printf("[INFO] Command pool created\n");
}

void VulkanCore::CreateCommandBuffers(uint32_t count, std::vector<vk::raii::CommandBuffer>& cmdBuffs) {
	vk::CommandBufferAllocateInfo cmdBuffAllocateInfo = {
		.sType = vk::StructureType::eCommandBufferAllocateInfo,
		.commandPool = m_cmdPool,
		.level = vk::CommandBufferLevel::ePrimary,
		.commandBufferCount = count,
	};

	auto buffers = vk::raii::CommandBuffers(m_device, cmdBuffAllocateInfo);

	for (auto& buffer : buffers) {
		cmdBuffs.push_back(std::move(buffer));
	}
}

void VulkanCore::FreeCommandBuffers(std::vector<vk::raii::CommandBuffer>& cmdBuffs) {
	cmdBuffs.clear();
}

void VulkanCore::CreateBLAS(vk::raii::CommandBuffer& cmdBuff) {
	// for each model in the scene

	// test code

	auto triangleData = vk::AccelerationStructureGeometryTrianglesDataKHR{
		.sType = vk::StructureType::eAccelerationStructureGeometryTrianglesDataKHR,
		.vertexFormat = vk::Format::eR32G32B32Sfloat,
		.vertexData = &m_vertices,
		.vertexStride = sizeof(float) * 3,
		.maxVertex = 2,
		.indexType = vk::IndexType::eUint32,
		.indexData = &m_indices[0]
	};
}

void VulkanCore::CreateTLAS(vk::raii::CommandBuffer& cmdBuff) {

}

void VulkanCore::CreateAccelerationStructure(Scene scene) {
	const vk::CommandBufferAllocateInfo cmdBuffAllocateInfo = {
		.sType = vk::StructureType::eCommandBufferAllocateInfo,
		.commandPool = m_cmdPool,
		.level = vk::CommandBufferLevel::ePrimary,
		.commandBufferCount = 1,
	};
	auto cmdBuffers = vk::raii::CommandBuffers(m_device, cmdBuffAllocateInfo);
	auto cmdBuff = std::move(cmdBuffers.front());

	CreateBLAS(cmdBuff);

	constexpr auto flags = vk::AccessFlagBits::eAccelerationStructureReadKHR | vk::AccessFlagBits::eAccelerationStructureWriteKHR;
	vk::MemoryBarrier memoryBarrier = {
		.sType = vk::StructureType::eMemoryBarrier,
		.srcAccessMask = flags,
		.dstAccessMask = flags,
	};
	cmdBuff.pipelineBarrier(vk::PipelineStageFlagBits::eAccelerationStructureBuildKHR,
		vk::PipelineStageFlagBits::eAccelerationStructureBuildKHR,
		{}, memoryBarrier, {}, {});

	CreateTLAS(cmdBuff);
}

void VulkanCore::CreateSBT() {

}

void VulkanCore::CreateRaytracingPipeline() {

}

void VulkanCore::Destroy() {
	m_device.waitIdle();
}
}
