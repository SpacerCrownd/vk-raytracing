#ifndef PHYSICAL_DEVICE_H
#define PHYSICAL_DEVICE_H

#include "Vulkan.h"
#include <vector>

namespace PathTracingVK {

struct PhysicalDevice {
	vk::raii::PhysicalDevice m_physDevice = nullptr;
	vk::PhysicalDeviceProperties m_devProperties;
	std::vector<vk::QueueFamilyProperties> m_qFamilyProperties;
	std::vector<vk::Bool32> m_qSupportsPresent;
	std::vector<vk::SurfaceFormatKHR> m_surfaceFormats;
	vk::SurfaceCapabilitiesKHR m_surfaceCapabilities;
	vk::PhysicalDeviceMemoryProperties m_memProperties;
	std::vector<vk::PresentModeKHR> m_presentModes;
	vk::PhysicalDeviceFeatures m_features;
	struct {
		int Variant = 0;
		int Major = 0;
		int Minor = 0;
		int Patch = 0;
	} m_apiVersion;
	std::vector<vk::ExtensionProperties> m_extensions;

	bool IsExtensionSupported(const char* pExt) const;
};

class VulkanPhysicalDevices {
public:
	VulkanPhysicalDevices() {}
	~VulkanPhysicalDevices() {}

	void Init(const vk::raii::Instance& instance, const vk::SurfaceKHR& surface);
	uint32_t SelectDevice(vk::QueueFlags requiredQueueType, bool supportsPresent);
	const PhysicalDevice& Selected() const;

private:
	std::vector<PhysicalDevice> m_devices;
	int m_devIndex = -1;
};

}
#endif