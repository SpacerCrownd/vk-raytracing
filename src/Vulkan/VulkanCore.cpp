#include "VulkanCore.h"
#include "VulkanUtils.h"

#include <algorithm>
#include <iostream>

namespace PathTracingVk {

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

VulkanCore::VulkanCore(const char* appName, const VulkanWindow& window) : m_window(window) {
	CreateInstance(appName);
	if (enableDebugging) { CreateDebugCallback(); }
	CreateSurface(window.GetWindow());
	SelectPhysicalDevice();
	CreateLogicalDevice();
	InitVmaAllocator();
	CreateSwapchain();
	CreateSyncObjects();
	CreateCommandObjects();
	//m_queue = std::make_unique<VulkanQueue>(m_device, m_swapchain, m_queueFamily, 0);
};

VulkanCore::~VulkanCore() {
	vmaDestroyAllocator(m_allocator);
}

void VulkanCore::UpdateInstanceVersion() {
	uint32_t instanceVersion = m_context.enumerateInstanceVersion();

	m_instanceVersion.Major = static_cast<int>(vk::apiVersionMajor(instanceVersion));
	m_instanceVersion.Minor = static_cast<int>(vk::apiVersionMinor(instanceVersion));
	m_instanceVersion.Patch = static_cast<int>(vk::apiVersionPatch(instanceVersion));

	printf("[INFO] Vulkan loader supports version %d.%d.%d\n", m_instanceVersion.Major, m_instanceVersion.Minor, m_instanceVersion.Patch);
}

void VulkanCore::CreateInstance(const char* appName) {
	UpdateInstanceVersion();

	vk::ApplicationInfo appInfo{
		.pApplicationName = appName,
		.applicationVersion = vk::makeVersion(0, 1, 0),
		.pEngineName = appName,
		.engineVersion = vk::makeVersion(0, 1, 0),
		.apiVersion = vk::makeApiVersion(0, m_instanceVersion.Major, m_instanceVersion.Minor, m_instanceVersion.Patch),
	};

	std::vector<const char*> layers = {
		"VK_LAYER_LUNARG_monitor",
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
	for (const auto& extension : extensionProperties) {
		std::cout << '\t' << extension.extensionName << std::endl;
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

void VulkanCore::CreateSurface(GLFWwindow* window) {
	VkSurfaceKHR surface;
	if (glfwCreateWindowSurface(*m_instance, window, nullptr, &surface) != 0) {
		throw std::runtime_error("Failed to create window surface!");
	}
	m_surface = vk::raii::SurfaceKHR(m_instance, surface);
	printf("[INFO] Surface created\n");
}

void VulkanCore::InitVmaAllocator() {
	VmaVulkanFunctions vulkanFunctions = {
		.vkGetInstanceProcAddr = &vkGetInstanceProcAddr,
		.vkGetDeviceProcAddr = &vkGetDeviceProcAddr,
	};

	VmaAllocatorCreateInfo allocatorCreateInfo = {
		.flags = VMA_ALLOCATOR_CREATE_EXT_MEMORY_BUDGET_BIT,
		.physicalDevice = static_cast<VkPhysicalDevice>(*m_physDevice->m_physDevice),
		.device = static_cast<VkDevice>(*m_device->GetDevice()),
		.pVulkanFunctions = &vulkanFunctions,
		.instance = static_cast<VkInstance>(*m_instance),
		.vulkanApiVersion = VK_API_VERSION_1_3,
	};

	vmaCreateAllocator(&allocatorCreateInfo, &m_allocator);
	printf("[INFO] VMA Allocator Created\n");
}

void VulkanCore::SelectPhysicalDevice() {
	auto vkPhysicalDevices = m_instance.enumeratePhysicalDevices();
	std::vector<VulkanPhysicalDevice> physicalDevices;
	physicalDevices.resize(vkPhysicalDevices.size());

	// populate physical devices
	for (uint32_t i = 0; i < vkPhysicalDevices.size(); i++) {
        auto device = vkPhysicalDevices[i];

        // Properties
        physicalDevices[i].m_physDevice = std::move(device);
        physicalDevices[i].m_devProperties2 = physicalDevices[i].m_physDevice.getProperties2();

        printf("\nDevice name: %s\n", physicalDevices[i].m_devProperties2.properties.deviceName.data());

        physicalDevices[i].m_features2 = physicalDevices[i].m_physDevice.getFeatures2();

        // API version
        uint32_t apiVersion = physicalDevices[i].m_devProperties2.properties.apiVersion;
        printf("	API version: %d.%d.%d.%d\n",
               vk::apiVersionVariant(apiVersion),
               vk::apiVersionMajor(apiVersion),
               vk::apiVersionMinor(apiVersion),
               vk::apiVersionPatch(apiVersion)
        );

        if (apiVersion < vk::ApiVersion13) {
            throw std::runtime_error("API version lower than 1.3");
        }

        // Queue Families
        physicalDevices[i].m_qFamilyProperties = physicalDevices[i].m_physDevice.getQueueFamilyProperties();
        size_t numQFamilies = physicalDevices[i].m_qFamilyProperties.size();
        physicalDevices[i].m_qSupportsPresent.resize(numQFamilies);
        printf("	Number of Queue families: %d\n", static_cast<int>(numQFamilies));

        for (uint32_t j = 0; j < numQFamilies; j++) {
            auto queueFamProperty = physicalDevices[i].m_qFamilyProperties[j];
            printf("	Family %d Num queues %d", j, queueFamProperty.queueCount);

            vk::QueueFlags flags = queueFamProperty.queueFlags;
            printf("	Graphics %s, Compute %s, Transfer %s, Sparse binding %s\n",
                   (flags & vk::QueueFlagBits::eGraphics) ? "Yes" : "No",
                   (flags & vk::QueueFlagBits::eCompute) ? "Yes" : "No",
                   (flags & vk::QueueFlagBits::eTransfer) ? "Yes" : "No",
                   (flags & vk::QueueFlagBits::eSparseBinding) ? "Yes" : "No"
            );

            physicalDevices[i].m_qSupportsPresent[j] = physicalDevices[i].m_physDevice.getSurfaceSupportKHR(j, m_surface);
        }
        printf("\n	Surface Stuff\n");
        // Formats
        physicalDevices[i].m_surfaceFormats = physicalDevices[i].m_physDevice.getSurfaceFormatsKHR(m_surface);

        for (const auto [format, colorSpace]: physicalDevices[i].m_surfaceFormats) {
            printf("	Format %d color space %d\n", format, colorSpace);
        }

        // Capabilities
        physicalDevices[i].m_surfaceCapabilities = physicalDevices[i].m_physDevice.getSurfaceCapabilitiesKHR(m_surface);
        PrintImageUsageFlags(physicalDevices[i].m_surfaceCapabilities.supportedUsageFlags);
        printf("	minImageCount = %d maxImageCount = %d\n", physicalDevices[i].m_surfaceCapabilities.minImageCount,
               physicalDevices[i].m_surfaceCapabilities.maxImageCount);
        printf("	currentExtent = %d x %d\n", physicalDevices[i].m_surfaceCapabilities.currentExtent.width,
               physicalDevices[i].m_surfaceCapabilities.currentExtent.height);
        printf("	maxImageExtent = %d x %d\n", physicalDevices[i].m_surfaceCapabilities.maxImageExtent.width,
               physicalDevices[i].m_surfaceCapabilities.maxImageExtent.height);
        printf("	minImageExtent = %d x %d\n", physicalDevices[i].m_surfaceCapabilities.minImageExtent.width,
               physicalDevices[i].m_surfaceCapabilities.minImageExtent.height);

        // Present modes
        physicalDevices[i].m_presentModes = physicalDevices[i].m_physDevice.getSurfacePresentModesKHR(m_surface);

        printf("	Present modes: %d\n", static_cast<int>(physicalDevices[i].m_presentModes.size()));

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

            printf("	Present mode %s supported\n", name);
        }

        // Memory properties
        physicalDevices[i].m_memProperties = physicalDevices[i].m_physDevice.getMemoryProperties();
        printf("Memory types: %d\n", physicalDevices[i].m_memProperties.memoryTypeCount);

        for (uint32_t j = 0; j < physicalDevices[i].m_memProperties.memoryTypeCount; j++) {
            printf("%d: flags %x, heap %d ",
                   j,
                   static_cast<uint32_t>(physicalDevices[i].m_memProperties.memoryTypes[j].propertyFlags),
                   physicalDevices[i].m_memProperties.memoryTypes[j].heapIndex
            );

            PrintMemoryProperty(physicalDevices[i].m_memProperties.memoryTypes[j].propertyFlags);
            printf("\n");
        }
        printf("Heap Types %d\n", physicalDevices[i].m_memProperties.memoryHeapCount);

        //extensions
        physicalDevices[i].m_extensions = physicalDevices[i].m_physDevice.enumerateDeviceExtensionProperties();

        physicalDevices[i].m_depthFormat = FindDepthFormat(physicalDevices[i].m_physDevice);

        /*printf("Available extensions:\n");
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
            if (!physicalDevice.IsExtensionSupported(reqExtension)) {
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

		m_physDevice = std::make_unique<VulkanPhysicalDevice>(physicalDevice);
		printf("[INFO] Physical Device selected: %s\n", m_physDevice->m_devProperties2.properties.deviceName.data());
		return;
    }

    throw std::runtime_error("No physical device with required queue type and ray tracing capabilities found");
}

void VulkanCore::CreateLogicalDevice() {
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
	auto& features = featureChain.get<vk::PhysicalDeviceFeatures2>();
	printf("%d", m_physDevice->m_qFamilyProperties.size());
	m_device = std::make_unique<VulkanDevice>(*m_physDevice, devExtensions, vk::QueueFlagBits::eGraphics, features);
	m_queue = vk::raii::Queue(m_device->GetDevice(), m_device->queueFamilyIndices.graphics, 0);
}

void VulkanCore::CreateSwapchain() {
	m_swapchain = std::make_unique<VulkanSwapchain>(*m_device, m_window.GetExtent(), m_surface);
}

void VulkanCore::RecreateSwapchain() {
	int width = 0, height = 0;
	glfwGetFramebufferSize(m_window.GetWindow(), &width, &height);
	while (width == 0 || height == 0) {
		glfwGetFramebufferSize(m_window.GetWindow(), &width, &height);
		glfwWaitEvents();
	}

	m_device->GetDevice().waitIdle();
	m_swapchain = nullptr;

	CreateSwapchain();
}

void VulkanCore::CreateSyncObjects() {
	m_inFlightFences.clear();
	m_renderSemaphores.clear();
	m_presentSemaphores.clear();

	// create one acquisition semaphore for each swapchain image
	for (int i = 0; i < m_swapchain->GetSwapchainImageCount(); i++) {
		m_presentSemaphores.emplace_back(m_device->GetDevice(), vk::SemaphoreCreateInfo());
	}

	// for each in-flight frame create submit semaphores and acquisition fences
	for (int i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
		m_renderSemaphores.emplace_back(m_device->GetDevice(), vk::SemaphoreCreateInfo());
		m_inFlightFences.emplace_back(m_device->GetDevice(), vk::FenceCreateInfo{.flags = vk::FenceCreateFlagBits::eSignaled});
	}

	printf("[INFO] Sync Objects Created\n");
}

void VulkanCore::CreateCommandObjects() {
	m_cmdPools.clear();
	m_cmdBuffs.clear();

	for (int i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
		m_cmdPools.emplace_back(m_device->GetDevice(), vk::CommandPoolCreateInfo{.queueFamilyIndex = m_device->queueFamilyIndices.graphics});
		m_cmdBuffs.push_back(std::move(vk::raii::CommandBuffers(m_device->GetDevice(), vk::CommandBufferAllocateInfo{.commandPool = m_cmdPools[i],
			                                                   .level = vk::CommandBufferLevel::ePrimary,
			                                                   .commandBufferCount = 1}).front()));
	}

	printf("[INFO] Command pools and buffers created\n");
}

vk::raii::CommandBuffer& VulkanCore::PrepareFrame() {
	while (m_device->GetDevice().waitForFences(*m_inFlightFences[m_currentFrameIndex], vk::True, UINT64_MAX) == vk::Result::eTimeout);
	m_device->GetDevice().resetFences(*m_inFlightFences[m_currentFrameIndex]);

	auto res = m_swapchain->AcquireNextImage(m_renderSemaphores[m_currentFrameIndex], m_currentImageIndex);
	//vk::detail::resultCheck(res, "[ERROR] Swapchain Acquire Next image");
	return BeginCommandBuffer();
}

void VulkanCore::SubmitFrame() {
	m_cmdBuffs[m_currentFrameIndex].end();
	SubmitAsync(m_cmdBuffs[m_currentFrameIndex]);
	Present();
}

// could be renamed in the future, right now it's very specific to drawing to swapchain
vk::raii::CommandBuffer& VulkanCore::BeginCommandBuffer() {
	m_cmdPools[m_currentFrameIndex].reset();
	m_cmdBuffs[m_currentFrameIndex].begin(vk::CommandBufferBeginInfo{.flags = vk::CommandBufferUsageFlagBits::eOneTimeSubmit});
	return m_cmdBuffs[m_currentFrameIndex];
}

void VulkanCore::SubmitAsync(const vk::CommandBuffer &cmdBuff) {
	vk::PipelineStageFlags waitFlags = vk::PipelineStageFlagBits::eColorAttachmentOutput;

	const vk::SubmitInfo submitInfo = {
		.sType = vk::StructureType::eSubmitInfo,
		.waitSemaphoreCount = 1,
		.pWaitSemaphores = &*m_renderSemaphores[m_currentFrameIndex],
		.pWaitDstStageMask = &waitFlags,
		.commandBufferCount = 1,
		.pCommandBuffers = &cmdBuff,
		.signalSemaphoreCount = 1,
		.pSignalSemaphores = &*m_presentSemaphores[m_currentImageIndex],
	};

	m_queue.submit(submitInfo, m_inFlightFences[m_currentFrameIndex]);
}

void VulkanCore::Present() {
	const vk::PresentInfoKHR presentInfo = {
		.sType = vk::StructureType::ePresentInfoKHR,
		.pNext = nullptr,
		.waitSemaphoreCount = 1,
		.pWaitSemaphores = &*m_presentSemaphores[m_currentImageIndex],
		.swapchainCount = 1,
		.pSwapchains = &*m_swapchain->GetSwapchain(),
		.pImageIndices = &m_currentImageIndex,
	};

	auto res = m_queue.presentKHR(presentInfo);

	if (res == vk::Result::eSuboptimalKHR || res == vk::Result::eErrorOutOfDateKHR) {

	}else {
		vk::detail::resultCheck(res, "vkQueuePresentKHR");
	}

	m_currentFrameIndex = (m_currentFrameIndex + 1) % MAX_FRAMES_IN_FLIGHT;
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
		.commandPool = m_cmdPools[0],
		.level = vk::CommandBufferLevel::ePrimary,
		.commandBufferCount = 1,
	};
	auto cmdBuffers = vk::raii::CommandBuffers(m_device->GetDevice(), cmdBuffAllocateInfo);
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

void VulkanCore::DeviceWaitIdle() {
	m_device->GetDevice().waitIdle();
}
}
