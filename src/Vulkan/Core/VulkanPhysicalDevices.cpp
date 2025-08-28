#include "VulkanPhysicalDevices.h"
#include <algorithm>

namespace PathTracingVK {

static void PrintImageUsageFlags(const vk::ImageUsageFlags& flags)
{
	if (flags & vk::ImageUsageFlagBits::eTransferSrc) {
		printf("	Image usage transfer src is supported\n");
	}

	if (flags & vk::ImageUsageFlagBits::eTransferDst) {
		printf("	Image usage transfer dest is supported\n");
	}

	if (flags & vk::ImageUsageFlagBits::eSampled) {
		printf("	Image usage sampled is supported\n");
	}

	if (flags & vk::ImageUsageFlagBits::eColorAttachment) {
		printf("	Image usage color attachment is supported\n");
	}

	if (flags & vk::ImageUsageFlagBits::eDepthStencilAttachment) {
		printf("	Image usage depth stencil attachment is supported\n");
	}

	if (flags & vk::ImageUsageFlagBits::eTransientAttachment) {
		printf("	Image usage transient attachment is supported\n");
	}

	if (flags & vk::ImageUsageFlagBits::eInputAttachment) {
		printf("	Image usage input attachment is supported\n");
	}
}

static void PrintMemoryProperty(const vk::Flags<vk::MemoryPropertyFlagBits>& flags)
{
	if (flags & vk::MemoryPropertyFlagBits::eDeviceLocal) {
		printf("DEVICE LOCAL ");
	}

	if (flags & vk::MemoryPropertyFlagBits::eHostVisible) {
		printf("HOST VISIBLE ");
	}

	if (flags & vk::MemoryPropertyFlagBits::eHostCoherent) {
		printf("HOST COHERENT ");
	}

	if (flags & vk::MemoryPropertyFlagBits::eHostCached) {
		printf("HOST CACHED ");
	}

	if (flags & vk::MemoryPropertyFlagBits::eLazilyAllocated) {
		printf("LAZILY ALLOCATED ");
	}

	if (flags & vk::MemoryPropertyFlagBits::eProtected) {
		printf("PROTECTED ");
	}
}

void VulkanPhysicalDevices::Init(const vk::raii::Instance& instance, const vk::SurfaceKHR& surface) {
	auto devices = instance.enumeratePhysicalDevices();
	m_devices.resize(devices.size());

	for (uint32_t i = 0; i < devices.size(); i++) {
		auto device = devices[i];

		// Properties
		m_devices[i].m_physDevice = std::move(device);
		m_devices[i].m_devProperties = m_devices[i].m_physDevice.getProperties();
		printf("\nDevice name: %s\n", m_devices[i].m_devProperties.deviceName.data());

		m_devices[i].m_features = m_devices[i].m_physDevice.getFeatures();

		// API version
		uint32_t apiVersion = m_devices[i].m_devProperties.apiVersion;
		printf("	API version: %d.%d.%d.%d\n",
			VK_API_VERSION_VARIANT(apiVersion),
			VK_API_VERSION_MAJOR(apiVersion),
			VK_API_VERSION_VARIANT(apiVersion),
			VK_API_VERSION_VARIANT(apiVersion)
		);

		// Queue Families
		m_devices[i].m_qFamilyProperties = m_devices[i].m_physDevice.getQueueFamilyProperties();
		size_t numQFamilies = m_devices[i].m_qFamilyProperties.size();
		printf("	Number of Queue families: %d\n", (int)numQFamilies);

		m_devices[i].m_qFamilyProperties.resize(numQFamilies);
		m_devices[i].m_qSupportsPresent.resize(numQFamilies);

		
		for (uint32_t j = 0; j < numQFamilies; j++) {
			auto queueFamProperty = m_devices[i].m_qFamilyProperties[j];

			printf("	Family %d Num queues %d", j, queueFamProperty.queueCount);

			vk::QueueFlags flags = queueFamProperty.queueFlags;

			printf("	Graphics %s, Compute %s, Transfer %s, Sparse binding %s\n", 
				(flags & vk::QueueFlagBits::eGraphics) ? "Yes" : "No",
				(flags & vk::QueueFlagBits::eCompute) ? "Yes" : "No",
				(flags & vk::QueueFlagBits::eTransfer) ? "Yes" : "No",
				(flags & vk::QueueFlagBits::eSparseBinding) ? "Yes" : "No"
			);

			m_devices[i].m_qSupportsPresent[j] = m_devices[i].m_physDevice.getSurfaceSupportKHR(j, surface);
		}
		printf("\n	Surface Stuff\n");
		// Formats
		auto surfaceFormats = m_devices[i].m_physDevice.getSurfaceFormatsKHR(surface);
		m_devices[i].m_surfaceFormats.resize(surfaceFormats.size());
		m_devices[i].m_surfaceFormats = std::move(surfaceFormats);

		for (uint32_t j = 0; j < m_devices[i].m_surfaceFormats.size(); j++)
		{
			const vk::SurfaceFormatKHR& surfaceFormat = m_devices[i].m_surfaceFormats[j];
			printf("	Format %d color space %d\n", surfaceFormat.format, surfaceFormat.colorSpace);
		}

		// Capabilities
		m_devices[i].m_surfaceCapabilities = m_devices[i].m_physDevice.getSurfaceCapabilitiesKHR(surface);
		PrintImageUsageFlags(m_devices[i].m_surfaceCapabilities.supportedUsageFlags);
		printf("	minImageCount = %d maxImageCount = %d\n", m_devices[i].m_surfaceCapabilities.minImageCount, m_devices[i].m_surfaceCapabilities.maxImageCount);
		printf("	currentExtent = %d x %d\n", m_devices[i].m_surfaceCapabilities.currentExtent.width, m_devices[i].m_surfaceCapabilities.currentExtent.height);
		printf("	maxImageExtent = %d x %d\n", m_devices[i].m_surfaceCapabilities.maxImageExtent.width, m_devices[i].m_surfaceCapabilities.maxImageExtent.height);
		printf("	minImageExtent = %d x %d\n", m_devices[i].m_surfaceCapabilities.minImageExtent.width, m_devices[i].m_surfaceCapabilities.minImageExtent.height);

		// Present modes
		auto presentModes = m_devices[i].m_physDevice.getSurfacePresentModesKHR(surface);
		m_devices[i].m_presentModes.resize(presentModes.size());
		m_devices[i].m_presentModes = presentModes;

		printf("	Present modes: %d\n", (int)presentModes.size());

		for (vk::PresentModeKHR presentMode : presentModes) {
			const char* name = "";
			switch (presentMode) {
			case vk::PresentModeKHR::eImmediate: name = "IMMEDIATE"; break;
			case vk::PresentModeKHR::eMailbox:   name = "MAILBOX"; break;
			case vk::PresentModeKHR::eFifo:      name = "FIFO"; break;
			case vk::PresentModeKHR::eFifoRelaxed: name = "FIFO_RELAXED"; break;
			default: name = "UNKNOWN"; break;
			}
			printf("	Present mode %s supported\n", name);
		}

		// Memory properties
		m_devices[i].m_memProperties = m_devices[i].m_physDevice.getMemoryProperties();
		printf("Memory types: %d\n", m_devices[i].m_memProperties.memoryTypeCount);
		for (uint32_t j = 0; j < m_devices[i].m_memProperties.memoryTypeCount; j++) {
			printf("%d: flags %x, heap %d ",
				j,
				static_cast<uint32_t>(m_devices[i].m_memProperties.memoryTypes[j].propertyFlags),
				m_devices[i].m_memProperties.memoryTypes[j].heapIndex
			);

			PrintMemoryProperty(m_devices[i].m_memProperties.memoryTypes[j].propertyFlags);
			printf("\n");
		}
		printf("Heap Types %d\n", m_devices[i].m_memProperties.memoryHeapCount);

		//extensions
		auto extensions = m_devices[i].m_physDevice.enumerateDeviceExtensionProperties();
		m_devices[i].m_extensions.resize(extensions.size());
		m_devices[i].m_extensions = extensions;
	}
}

uint32_t VulkanPhysicalDevices::SelectDevice(vk::QueueFlags requiredQueueType, bool supportsPresent) {
	for (uint32_t i = 0; i < m_devices.size(); i++) {
		// Check each queue
		for (uint32_t j = 0; j < m_devices[i].m_qFamilyProperties.size(); j++) {
			const vk::QueueFamilyProperties qFamilyProp = m_devices[i].m_qFamilyProperties[j];

			if (qFamilyProp.queueFlags & requiredQueueType && (bool)m_devices[i].m_qSupportsPresent[j] == supportsPresent) {
				bool isSuitable = true;

				// Hard coded extensions validation
				std::vector<const char*> requiredExtensions = {
					VK_KHR_RAY_TRACING_PIPELINE_EXTENSION_NAME
				};

				for(auto reqExtension : requiredExtensions)
				{
					isSuitable = m_devices[i].IsExtensionSupported(reqExtension);
					if (!isSuitable)
						break;
				}

				if (isSuitable) {
					m_devIndex = i;
					int queueFamily = j;
					printf("Using graphics device %d and queue family %d\n", m_devIndex, queueFamily);
					return queueFamily;
				}
			}
		}
	}

	throw std::runtime_error("No physical device with required queue type and ray tracing capabilities found");
}

const PhysicalDevice& VulkanPhysicalDevices::Selected() const {
	if (m_devIndex < 0) {
		throw std::runtime_error("No physical device selected");
	}
}

bool PhysicalDevice::IsExtensionSupported(const char* pExt) const {
	bool res = false;
	for (const auto extension : m_extensions) {
		if (strcmp(extension.extensionName, pExt)) {
			res = true;
			break;
		}
	}

	printf("Extension %s %s supporterd\n", pExt, res ? "is" : "is not");
	return res;
}

}