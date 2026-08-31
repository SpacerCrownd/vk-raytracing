#include "Core.h"
#include "Utils.h"
#include "Commands.h"

#include <algorithm>
#include <iostream>

namespace ptvk {
static VKAPI_ATTR vk::Bool32 VKAPI_CALL DebugCallback(
	vk::DebugUtilsMessageSeverityFlagBitsEXT severity,
	vk::DebugUtilsMessageTypeFlagsEXT type,
	const vk::DebugUtilsMessengerCallbackDataEXT* pCallbackData,
	void* pUserData)
{
	std::cout << "[Debug Callback] " << pCallbackData->pMessage << std::endl;

	std::cout << "\tSeverity: "
			  << getDebugSeverityStr(severity) << std::endl;

	std::cout << "\tType: "
			  << getDebugType(type) << std::endl;

	std::cout << " Objects " << std::endl;

	for (uint32_t i = 0; i < pCallbackData->objectCount; i++) {
		std::cout << '\t' << pCallbackData->pObjects[i].objectHandle;

		if (i + 1 < pCallbackData->objectCount)
			std::cout << ", ";
	}

	std::cout << std::endl;

	return vk::False;
}

Core::Core(const char* appName, const Window& window) : m_window(window)
{
	createInstance(appName);
	if (enableDebugging) { createDebugCallback(); }
	createSurface(window.getWindow());
	selectPhysicalDevice();
	createLogicalDevice();
	initResourceAllocator();
	createCommandObjects();
	createSwapchain();
	createSyncObjects();
	createSamplers();
};

void Core::updateInstanceVersion() {
	uint32_t instanceVersion = m_context.enumerateInstanceVersion();

	m_instanceVersion.Major = static_cast<int>(vk::apiVersionMajor(instanceVersion));
	m_instanceVersion.Minor = static_cast<int>(vk::apiVersionMinor(instanceVersion));
	m_instanceVersion.Patch = static_cast<int>(vk::apiVersionPatch(instanceVersion));

	std::cout << "[INFO] Vulkan loader supports version "
		  << m_instanceVersion.Major << "."
		  << m_instanceVersion.Minor << "."
		  << m_instanceVersion.Patch
		  << std::endl;
}

void Core::createInstance(const char* appName) {
	updateInstanceVersion();

	vk::ApplicationInfo appInfo{
		.pApplicationName = appName,
		.applicationVersion = vk::makeVersion(0, 1, 0),
		.pEngineName = appName,
		.engineVersion = vk::makeVersion(0, 1, 0),
		.apiVersion = VK_API_VERSION_1_3,
	};

	std::vector<const char*> layers = {
		"VK_LAYER_LUNARG_monitor"
	};

	if (enableDebugging) {
		layers.push_back("VK_LAYER_KHRONOS_validation");
	}

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
	for (const auto&[extensionName, specVersion] : extensionProperties) {
		std::cout << '\t' << extensionName << std::endl;
	}

	// Get the required instance layers
	std::vector<char const*> requiredLayers;
	requiredLayers.assign(layers.begin(), layers.end());

	// Check if the required layers are supported by the Vulkan implementation
	if (std::ranges::any_of(requiredLayers, [&layerProperties](auto const& requiredLayer) {
		return std::ranges::none_of(layerProperties, [requiredLayer](auto const& layerProperty) { return strcmp(layerProperty.layerName, requiredLayer) == 0; });
	})) {
		throw std::runtime_error("One or more required layers are not supported!");
	}

	vk::InstanceCreateInfo createInfo{
		.pNext = nullptr,
		.pApplicationInfo = &appInfo,
		.enabledLayerCount = static_cast<uint32_t>(requiredLayers.size()),
		.ppEnabledLayerNames = requiredLayers.data(),
		.enabledExtensionCount = static_cast<uint32_t>(extensions.size()),
		.ppEnabledExtensionNames = extensions.data(),
	};

	m_instance = vk::raii::Instance(m_context, createInfo);
	std::cout << std::endl << "[INFO] Instance Created" << std::endl;
}

void Core::createDebugCallback() {
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
	std::cout << "[INFO] Debug Messenger Created" << std::endl;
}

void Core::createSurface(GLFWwindow* window) {
	VkSurfaceKHR surface;
	if (glfwCreateWindowSurface(*m_instance, window, nullptr, &surface) != 0) {
		throw std::runtime_error("Failed to create window surface!");
	}
	m_surface = vk::raii::SurfaceKHR(m_instance, surface);
	std::cout << "[INFO] Surface created" << std::endl;
}

void Core::initResourceAllocator() {
	VmaVulkanFunctions vulkanFunctions = {
		.vkGetInstanceProcAddr = m_instance.getDispatcher()->vkGetInstanceProcAddr,
		.vkGetDeviceProcAddr = m_pDevice->getVkDevice().getDispatcher()->vkGetDeviceProcAddr,
	};

	VmaAllocatorCreateInfo allocatorCreateInfo = {
		.flags = VMA_ALLOCATOR_CREATE_BUFFER_DEVICE_ADDRESS_BIT,
		.physicalDevice = *m_pPhysDevice->m_physDevice,
		.device = *m_pDevice->getVkDevice(),
		.pVulkanFunctions = &vulkanFunctions,
		.instance = *m_instance,
		.vulkanApiVersion = VK_API_VERSION_1_3,
	};

	m_pResourceAllocator = std::make_unique<ResourceAllocator>(allocatorCreateInfo, &*m_pDevice);
	std::cout << "[INFO] VMA Allocator Created" << std::endl;
}

void Core::selectPhysicalDevice() {
	auto vkPhysicalDevices = m_instance.enumeratePhysicalDevices();
	std::vector<PhysicalDevice> physicalDevices;
	physicalDevices.resize(vkPhysicalDevices.size());

	// populate physical devices
	for (uint32_t i = 0; i < vkPhysicalDevices.size(); i++) {
		auto device = vkPhysicalDevices[i];

		// Properties
		physicalDevices[i].m_physDevice = std::move(device);
		physicalDevices[i].m_devProperties2 = physicalDevices[i].m_physDevice.getProperties2();

		std::cout << "\nDevice name: " << physicalDevices[i].m_devProperties2.properties.deviceName.data() << std::endl;

		physicalDevices[i].m_features2 = physicalDevices[i].m_physDevice.getFeatures2();

		// API version
		uint32_t apiVersion = physicalDevices[i].m_devProperties2.properties.apiVersion;
		std::cout << "\tAPI version: "
		  << vk::apiVersionVariant(apiVersion) << "."
		  << vk::apiVersionMajor(apiVersion) << "."
		  << vk::apiVersionMinor(apiVersion) << "."
		  << vk::apiVersionPatch(apiVersion)
		  << '\n';

		if (apiVersion < vk::ApiVersion13) {
			throw std::runtime_error("API version lower than 1.3");
		}

		// Queue Families
		physicalDevices[i].m_queueFamilyProperties = physicalDevices[i].m_physDevice.getQueueFamilyProperties();
		size_t numQFamilies = physicalDevices[i].m_queueFamilyProperties.size();
		physicalDevices[i].m_queueSupportsPresent.resize(numQFamilies);
		std::cout << "\tNumber of Queue families: " << static_cast<int>(numQFamilies) << std::endl;

		for (uint32_t j = 0; j < numQFamilies; j++) {
			auto queueFamProperty = physicalDevices[i].m_queueFamilyProperties[j];
			std::cout << "\tFamily " << j << " Num queues " << queueFamProperty.queueCount << std::endl;

			vk::QueueFlags flags = queueFamProperty.queueFlags;
			std::cout << "\tGraphics " << ((flags & vk::QueueFlagBits::eGraphics) ? "Yes" : "No")
				<< ", Compute " << ((flags & vk::QueueFlagBits::eCompute) ? "Yes" : "No")
				<< ", Transfer " << ((flags & vk::QueueFlagBits::eTransfer) ? "Yes" : "No")
				<< ", Sparse binding " << ((flags & vk::QueueFlagBits::eSparseBinding) ? "Yes" : "No")
				<< std::endl;

			physicalDevices[i].m_queueSupportsPresent[j] = physicalDevices[i].m_physDevice.getSurfaceSupportKHR(j, m_surface);
		}
		std::cout << std::endl << "	Surface Stuff" << std::endl;
		// Formats
		physicalDevices[i].m_surfaceFormats = physicalDevices[i].m_physDevice.getSurfaceFormatsKHR(m_surface);

		for (const auto [format, colorSpace]: physicalDevices[i].m_surfaceFormats) {
			std::cout << " Format " << to_string(format) << " color space " << to_string(colorSpace) << std::endl;
		}

		// Capabilities
		physicalDevices[i].m_surfaceCapabilities = physicalDevices[i].m_physDevice.getSurfaceCapabilitiesKHR(m_surface);
		printImageUsageFlags(physicalDevices[i].m_surfaceCapabilities.supportedUsageFlags);
		std::cout << "	minImageCount = " << physicalDevices[i].m_surfaceCapabilities.minImageCount << " maxImageCount = " <<
			physicalDevices[i].m_surfaceCapabilities.maxImageCount << std::endl;
		std::cout << "	currentExtent = " << physicalDevices[i].m_surfaceCapabilities.currentExtent.width << " x " <<
			physicalDevices[i].m_surfaceCapabilities.currentExtent.height << std::endl;
		std::cout << "	maxImageExtent = " << physicalDevices[i].m_surfaceCapabilities.maxImageExtent.width << " x " <<
			physicalDevices[i].m_surfaceCapabilities.maxImageExtent.height << std::endl;
		std::cout << "	minImageExtent = " << physicalDevices[i].m_surfaceCapabilities.minImageExtent.width << " x " <<
			physicalDevices[i].m_surfaceCapabilities.minImageExtent.height << std::endl;

		// Present modes
		physicalDevices[i].m_presentModes = physicalDevices[i].m_physDevice.getSurfacePresentModesKHR(m_surface);

		std::cout << " Present modes: " << static_cast<int>(physicalDevices[i].m_presentModes.size()) << std::endl;

		for (const vk::PresentModeKHR presentMode: physicalDevices[i].m_presentModes) {
			auto name = "";

			switch (presentMode) {
				case vk::PresentModeKHR::eImmediate:
					name = "IMMEDIATE";
					break;
				case vk::PresentModeKHR::eMailbox:
					name = "MAILBOX";
					break;
				case vk::PresentModeKHR::eFifo:
					name = "FIFO";
					break;
				case vk::PresentModeKHR::eFifoRelaxed:
					name = "FIFO_RELAXED";
					break;
				default: name = "UNKNOWN";
					break;
			}

			std::cout << "	Present mode " << name << " supported" << std::endl;
		}

		// Memory properties
		physicalDevices[i].m_memProperties = physicalDevices[i].m_physDevice.getMemoryProperties();
		std::cout << "Memory types: " << physicalDevices[i].m_memProperties.memoryTypeCount << std::endl;

		for (uint32_t j = 0; j < physicalDevices[i].m_memProperties.memoryTypeCount; j++) {
			std::cout
				<< j
				<< ": flags "
				<< static_cast<uint32_t>(physicalDevices[i].m_memProperties.memoryTypes[j].propertyFlags)
				<< ", heap "
				<< physicalDevices[i].m_memProperties.memoryTypes[j].heapIndex;
			printMemoryPropertyFlags(physicalDevices[i].m_memProperties.memoryTypes[j].propertyFlags);
			std::cout << std::endl;
		}
		std::cout << "Heap Types " << physicalDevices[i].m_memProperties.memoryHeapCount << std::endl;

		//extensions
		physicalDevices[i].m_extensions = physicalDevices[i].m_physDevice.enumerateDeviceExtensionProperties();

		physicalDevices[i].m_depthFormat = findDepthFormat(physicalDevices[i].m_physDevice);

		/*std::cout << "Available extensions:\n");
		std::cout << "Extension count: " << m_devices[i].m_extensions.size() << "\n";
		for (const auto& ext : m_devices[i].m_extensions) {
			std::cout << std::string(ext.extensionName.data()) << "\n";
		}*/
	}

	for (auto & physicalDevice : physicalDevices) {
		// Check device extensions
		bool missingRequiredExtensions = false;

		std::vector<const char *> requiredExtensions = {
			vk::KHRRayTracingPipelineExtensionName,
			vk::KHRAccelerationStructureExtensionName,
			vk::KHRDeferredHostOperationsExtensionName
		};

		for (auto reqExtension: requiredExtensions) {
			if (!physicalDevice.isExtensionSupported(reqExtension)) {
				missingRequiredExtensions = true;
				break;
			}
		}

		if (missingRequiredExtensions)
			continue;

		// Check device features
		if (physicalDevice.m_features2.features.geometryShader == vk::False) {
			continue;
		}

		if (physicalDevice.m_features2.features.tessellationShader == vk::False) {
			continue;
		}

		physicalDevice.m_asProperties =
			physicalDevice.m_physDevice.getProperties2<vk::PhysicalDeviceProperties2, vk::PhysicalDeviceAccelerationStructurePropertiesKHR>()
				.get<vk::PhysicalDeviceAccelerationStructurePropertiesKHR>();

		physicalDevice.m_rtProperties =
			physicalDevice.m_physDevice.getProperties2<vk::PhysicalDeviceProperties2, vk::PhysicalDeviceRayTracingPipelinePropertiesKHR>()
				.get<vk::PhysicalDeviceRayTracingPipelinePropertiesKHR>();

		m_pPhysDevice = std::make_unique<PhysicalDevice>(physicalDevice);
		std::cout << "[INFO] Physical Device selected: " << m_pPhysDevice->m_devProperties2.properties.deviceName.data() << std::endl;
		return;
	}

	throw std::runtime_error("No physical device with required queue type and ray tracing capabilities found");
}

void Core::createLogicalDevice() {
	std::vector<const char *> devExtensions = {
		vk::KHRShaderDrawParametersExtensionName,
		vk::KHRSwapchainExtensionName,
		vk::KHRSpirv14ExtensionName,
		vk::KHRSynchronization2ExtensionName,
		vk::KHRCreateRenderpass2ExtensionName,
		vk::KHRRayTracingPipelineExtensionName,
		vk::KHRAccelerationStructureExtensionName,
		vk::KHRDeferredHostOperationsExtensionName
	};

	vk::StructureChain<
		vk::PhysicalDeviceFeatures2,
		vk::PhysicalDeviceVulkan12Features,
		vk::PhysicalDeviceVulkan13Features,
		vk::PhysicalDeviceExtendedDynamicStateFeaturesEXT,
		vk::PhysicalDeviceRayTracingPipelineFeaturesKHR,
		vk::PhysicalDeviceAccelerationStructureFeaturesKHR
	> featureChain = {
		{},
		{
			.descriptorIndexing = true,
			.descriptorBindingVariableDescriptorCount = true,
			.runtimeDescriptorArray = true,
			.bufferDeviceAddress = true
		},
		{
			.synchronization2 = true,
			.dynamicRendering = true,
		},
		{.extendedDynamicState = true},
		{.rayTracingPipeline = true},
		{.accelerationStructure = true},
	};

	featureChain.get<vk::PhysicalDeviceFeatures2>().features.geometryShader = vk::True;
	featureChain.get<vk::PhysicalDeviceFeatures2>().features.tessellationShader = vk::True;
	featureChain.get<vk::PhysicalDeviceFeatures2>().features.samplerAnisotropy = vk::True;
	auto& features = featureChain.get<vk::PhysicalDeviceFeatures2>();
	std::cout << "Queue family properties size " << m_pPhysDevice->m_queueFamilyProperties.size() << std::endl;
	m_pDevice = std::make_unique<Device>(*m_pPhysDevice, devExtensions, vk::QueueFlagBits::eGraphics, features, m_instanceVersion);
	m_queue = vk::raii::Queue(m_pDevice->getVkDevice(), m_pDevice->queueFamilyIndices.graphics, 0);
}

void Core::createSwapchain() {
	vk::Extent2D extent = chooseSwapExtent(m_pPhysDevice->m_surfaceCapabilities);
	m_pSwapchain = std::make_unique<Swapchain>(*m_pDevice, extent, m_surface);

	// create separate draw image
	vk::Extent3D drawImageExtent = {extent.width, extent.height, 1};

	vk::ImageUsageFlags imageUsageFlags = vk::ImageUsageFlagBits::eColorAttachment
	| vk::ImageUsageFlagBits::eSampled
	| vk::ImageUsageFlagBits::eTransferSrc
	| vk::ImageUsageFlagBits::eTransferDst;

	vk::ImageCreateInfo imageInfo = {
		.imageType = vk::ImageType::e2D,
		.format = vk::Format::eR16G16B16A16Sfloat,
		.extent = drawImageExtent,
		.mipLevels = 1,
		.arrayLayers = 1,
		.tiling = vk::ImageTiling::eOptimal,
		.usage = imageUsageFlags,
	};

	VmaAllocationCreateInfo allocationInfo = {
		.usage = VMA_MEMORY_USAGE_AUTO,
		.requiredFlags = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
	};

	vk::ImageViewCreateInfo imageViewInfo = {
		.viewType = vk::ImageViewType::e2D,
		.subresourceRange = {
			.aspectMask = vk::ImageAspectFlagBits::eColor,
			.baseMipLevel = 0,
			.levelCount = 1,
			.baseArrayLayer = 0,
			.layerCount = 1,
		}
	};

	m_drawImage = m_pResourceAllocator->createImage(imageInfo, imageViewInfo, allocationInfo);

	vk::ImageSubresourceRange subresourceRange = {
		.aspectMask = vk::ImageAspectFlagBits::eColor,
		.baseMipLevel = 0,
		.levelCount = 1,
		.baseArrayLayer = 0,
		.layerCount = 1,
	};

	auto cmdBuf = beginSingleTimeCommands(m_pDevice->getVkDevice(), m_transientCmdPool);
	imageLayoutTransition(cmdBuf,
						  m_drawImage.image,
						  vk::PipelineStageFlagBits2::eAllCommands,
						  vk::PipelineStageFlagBits2::eAllCommands,
						  {},
						  {},
						  vk::ImageLayout::eUndefined,
						  vk::ImageLayout::eGeneral,
						  subresourceRange);
	submitSingleTimeCommandBuffer(cmdBuf);

	createDepthResources();
}

void Core::recreateSwapchain() {
	std::cout << "[INFO] Recreating Swapchain\n" << std::endl;
	int width = 0, height = 0;
	glfwGetFramebufferSize(m_window.getWindow(), &width, &height);
	while (width == 0 || height == 0) {
		glfwGetFramebufferSize(m_window.getWindow(), &width, &height);
		glfwWaitEvents();
	}

	// update surface capabilities
	m_pPhysDevice->m_surfaceCapabilities = m_pPhysDevice->m_physDevice.getSurfaceCapabilitiesKHR(m_surface);

	m_pDevice->getVkDevice().waitIdle();
	m_pSwapchain = nullptr;

	createSwapchain();
}

void Core::createSyncObjects() {
	m_inFlightFences.clear();
	m_renderSemaphores.clear();
	m_presentSemaphores.clear();

	// create one acquisition semaphore for each swapchain image
	for (int i = 0; i < m_pSwapchain->GetSwapchainImageCount(); i++) {
		m_presentSemaphores.emplace_back(m_pDevice->getVkDevice(), vk::SemaphoreCreateInfo());
	}

	// for each in-flight frame create submit semaphores and acquisition fences
	for (int i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
		m_renderSemaphores.emplace_back(m_pDevice->getVkDevice(), vk::SemaphoreCreateInfo());
		m_inFlightFences.emplace_back(m_pDevice->getVkDevice(), vk::FenceCreateInfo{.flags = vk::FenceCreateFlagBits::eSignaled});
	}

	std::cout << "[INFO] Sync Objects Created\n" << std::endl;
}

void Core::createCommandObjects() {
	m_cmdPools.clear();
	m_cmdBuffs.clear();
	m_transientCmdPool.clear();

	m_transientCmdPool = createTransientCommandPool(m_pDevice->getVkDevice(), m_pDevice->queueFamilyIndices.graphics);

	for (int i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
		m_cmdPools.emplace_back(m_pDevice->getVkDevice(), vk::CommandPoolCreateInfo{.queueFamilyIndex = m_pDevice->queueFamilyIndices.graphics});
		m_cmdBuffs.push_back(std::move(vk::raii::CommandBuffers(m_pDevice->getVkDevice(), vk::CommandBufferAllocateInfo{.commandPool = m_cmdPools[i],
															   .level = vk::CommandBufferLevel::ePrimary,
															   .commandBufferCount = 1}).front()));
	}

	std::cout << "[INFO] Command pools and buffers created" << std::endl;
}

void Core::createDepthResources() {
	vk::Format depthFormat = m_pPhysDevice->m_depthFormat;

	auto imageUsageFlags = vk::ImageUsageFlagBits::eDepthStencilAttachment;
	vk::Extent3D extent = {
		.width = m_pSwapchain->GetExtent().width,
		.height = m_pSwapchain->GetExtent().height,
		.depth = 1
	};

	vk::ImageCreateInfo imageCreateInfo = {
		.imageType = vk::ImageType::e2D,
		.format = depthFormat,
		.extent = extent,
		.mipLevels = 1,
		.arrayLayers = 1,
		.tiling = vk::ImageTiling::eOptimal,
		.usage = imageUsageFlags,
	};

	VmaAllocationCreateInfo allocationCreateInfo = {
		.usage = VMA_MEMORY_USAGE_AUTO,
		.requiredFlags = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
	};

	vk::ImageViewCreateInfo viewCreateInfo = {
		.viewType = vk::ImageViewType::e2D,
		.subresourceRange = {
			.aspectMask = vk::ImageAspectFlagBits::eDepth,
			.levelCount = 1,
			.layerCount = 1,
		}
	};

	m_depthImage = m_pResourceAllocator->createImage(imageCreateInfo, viewCreateInfo, allocationCreateInfo);

	// transition image to depth optimal
	vk::ImageSubresourceRange subresourceRange = {
		.aspectMask = vk::ImageAspectFlagBits::eDepth,
		.baseMipLevel = 0,
		.levelCount = 1,
		.baseArrayLayer = 0,
		.layerCount = 1,
	};

	auto cmdBuf = beginSingleTimeCommandBuffer();
	imageLayoutTransition(cmdBuf,
						  m_depthImage.image,
						  vk::PipelineStageFlagBits2::eAllCommands,
						  vk::PipelineStageFlagBits2::eAllCommands,
						  {},
						  {},
						  vk::ImageLayout::eUndefined,
						  vk::ImageLayout::eDepthAttachmentOptimal,
						  subresourceRange);
	submitSingleTimeCommandBuffer(cmdBuf);
}

void Core::createSamplers() {
	std::array<vk::SamplerCreateInfo, 2> createInfos{};

	auto properties = m_pDevice->getPhysicalDevice().m_devProperties2;

	createInfos[0] = {
		.magFilter = vk::Filter::eNearest,
		.minFilter = vk::Filter::eNearest,
		.mipmapMode = vk::SamplerMipmapMode::eNearest,
		.addressModeU = vk::SamplerAddressMode::eRepeat,
		.addressModeV = vk::SamplerAddressMode::eRepeat,
		.addressModeW = vk::SamplerAddressMode::eRepeat,
		.mipLodBias = 0.0f,
		.anisotropyEnable = vk::True,
		.maxAnisotropy = properties.properties.limits.maxSamplerAnisotropy,
		.compareEnable = vk::False,
		.compareOp = vk::CompareOp::eAlways,
		.minLod = 0.0f,
		.maxLod = 0.0f,
		.borderColor = vk::BorderColor::eFloatOpaqueBlack,
		.unnormalizedCoordinates = vk::False,
	};

	createInfos[1] = {
		.magFilter = vk::Filter::eLinear,
		.minFilter = vk::Filter::eLinear,
		.mipmapMode = vk::SamplerMipmapMode::eNearest,
		.addressModeU = vk::SamplerAddressMode::eRepeat,
		.addressModeV = vk::SamplerAddressMode::eRepeat,
		.addressModeW = vk::SamplerAddressMode::eRepeat,
		.mipLodBias = 0.0f,
		.anisotropyEnable = vk::True,
		.maxAnisotropy = properties.properties.limits.maxSamplerAnisotropy,
		.compareEnable = vk::False,
		.compareOp = vk::CompareOp::eAlways,
		.minLod = 0.0f,
		.maxLod = 0.0f,
		.borderColor = vk::BorderColor::eFloatOpaqueBlack,
		.unnormalizedCoordinates = vk::False,
	};

	for (size_t i = 0; i < m_samplers.size(); i++) {
		m_samplers.emplace_back(m_pDevice->getVkDevice(), createInfos[i], nullptr);
	}

	std::cout << "[INFO] Initialized samplers" << std::endl;
}

void Core::prepareFrame() {
	auto fenceResult = m_pDevice->getVkDevice().waitForFences(*m_inFlightFences[m_currentFrameIndex], vk::True, UINT64_MAX);
	VK_CHECK_RESULT(fenceResult, "Failed waiting for frame fence");
	m_pDevice->getVkDevice().resetFences(*m_inFlightFences[m_currentFrameIndex]);

	auto res = m_pSwapchain->AcquireNextImage(m_renderSemaphores[m_currentFrameIndex], m_currentImageIndex);
	if (res == vk::Result::eErrorOutOfDateKHR) {
		recreateSwapchain();
		return;
	}
	if (res != vk::Result::eSuccess && res != vk::Result::eSuboptimalKHR) {
		throw std::runtime_error("[ERROR] Failed to acquire next swapchain image");
	}
}

void Core::submitFrame() {
	m_cmdBuffs[m_currentFrameIndex].end();

	vk::SemaphoreSubmitInfo waitSemaphoreSubmitInfo = {
		.semaphore = m_renderSemaphores[m_currentFrameIndex],
		.stageMask = vk::PipelineStageFlagBits2::eColorAttachmentOutput,
	};

	vk::SemaphoreSubmitInfo signalSemaphoreSubmitInfo = {
		.semaphore = m_presentSemaphores[m_currentImageIndex],
		.stageMask = vk::PipelineStageFlagBits2::eAllCommands,
	};

	vk::CommandBufferSubmitInfo cmdBufferSubmitInfo = {
		.commandBuffer = m_cmdBuffs[m_currentFrameIndex],
	};

	vk::SubmitInfo2 submitInfo = {
		.waitSemaphoreInfoCount = 1,
		.pWaitSemaphoreInfos = &waitSemaphoreSubmitInfo,
		.commandBufferInfoCount = 1,
		.pCommandBufferInfos = &cmdBufferSubmitInfo,
		.signalSemaphoreInfoCount = 1,
		.pSignalSemaphoreInfos = &signalSemaphoreSubmitInfo,
	};

	m_queue.submit2(submitInfo, m_inFlightFences[m_currentFrameIndex]);
}

vk::raii::CommandBuffer& Core::beginCommandRecording() {
	m_cmdPools[m_currentFrameIndex].reset();
	m_cmdBuffs[m_currentFrameIndex].begin(vk::CommandBufferBeginInfo{.flags = vk::CommandBufferUsageFlagBits::eOneTimeSubmit});
	return m_cmdBuffs[m_currentFrameIndex];
}

vk::raii::CommandBuffer Core::beginSingleTimeCommandBuffer() {
	return beginSingleTimeCommands(m_pDevice->getVkDevice(), m_transientCmdPool);
}

vk::Result Core::submitSingleTimeCommandBuffer(const vk::raii::CommandBuffer &cmdBuf) {
	return submitSingleTimeCommands(cmdBuf, m_pDevice->getVkDevice(), m_queue);
}

void Core::presentFrame() {
	const vk::PresentInfoKHR presentInfo = {
		.sType = vk::StructureType::ePresentInfoKHR,
		.pNext = nullptr,
		.waitSemaphoreCount = 1,
		.pWaitSemaphores = &*m_presentSemaphores[m_currentImageIndex],
		.swapchainCount = 1,
		.pSwapchains = &*m_pSwapchain->GetSwapchain(),
		.pImageIndices = &m_currentImageIndex,
	};

	auto res = m_queue.presentKHR(presentInfo);

	if (res == vk::Result::eSuboptimalKHR || res == vk::Result::eErrorOutOfDateKHR || framebufferResized) {
		framebufferResized = false;
		recreateSwapchain();
	}else {
		assert(res == vk::Result::eSuccess && "Failed to present!");
	}
	
	m_currentFrameIndex = (m_currentFrameIndex + 1) % MAX_FRAMES_IN_FLIGHT;
}

void Core::createBLAS(vk::raii::CommandBuffer& cmdBuff) {
	// TODO: after model and scene loading -> create blas for each model in the scene

}

void Core::createTLAS(vk::raii::CommandBuffer& cmdBuff) {

}

void Core::createAccelerationStructure() {
	const vk::CommandBufferAllocateInfo cmdBuffAllocateInfo = {
		.sType = vk::StructureType::eCommandBufferAllocateInfo,
		.commandPool = m_cmdPools[0],
		.level = vk::CommandBufferLevel::ePrimary,
		.commandBufferCount = 1,
	};
	auto cmdBuffers = vk::raii::CommandBuffers(m_pDevice->getVkDevice(), cmdBuffAllocateInfo);
	auto cmdBuff = std::move(cmdBuffers.front());

	createBLAS(cmdBuff);

	constexpr auto flags = vk::AccessFlagBits::eAccelerationStructureReadKHR | vk::AccessFlagBits::eAccelerationStructureWriteKHR;
	vk::MemoryBarrier memoryBarrier = {
		.sType = vk::StructureType::eMemoryBarrier,
		.srcAccessMask = flags,
		.dstAccessMask = flags,
	};
	cmdBuff.pipelineBarrier(vk::PipelineStageFlagBits::eAccelerationStructureBuildKHR,
		vk::PipelineStageFlagBits::eAccelerationStructureBuildKHR,
		{}, memoryBarrier, {}, {});

	createTLAS(cmdBuff);
}

void Core::createSBT() {

}

void Core::createRaytracingPipeline() {

}

vk::Extent2D Core::chooseSwapExtent(const vk::SurfaceCapabilitiesKHR& capabilities) {
	if (capabilities.currentExtent.width != 0xFFFFFFFF) {
		std::cout << "[INFO] Current image extent: " << capabilities.currentExtent.width << " x " << capabilities.currentExtent.height << std::endl;
		return capabilities.currentExtent;
	}
	int width, height;
	glfwGetFramebufferSize(m_window.getWindow(), &width, &height);
	std::cout << "Max image extent: " << capabilities.maxImageExtent.width << std::endl;
	return {
		std::clamp<uint32_t>(width, capabilities.minImageExtent.width, capabilities.maxImageExtent.width),
		std::clamp<uint32_t>(height, capabilities.minImageExtent.height, capabilities.maxImageExtent.height)
	};
}

void Core::deviceWaitIdle() {
	m_pDevice->getVkDevice().waitIdle();
}
}
