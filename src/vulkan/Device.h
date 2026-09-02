#ifndef PHYSICAL_DEVICE_H
#define PHYSICAL_DEVICE_H

#include "Vulkan.h"
#include "PhysicalDevice.h"

namespace ptvk {

class Device {

public:
	Device(const PhysicalDevice &device,
		std::vector<const char*> &devExtensions,
		const vk::QueueFlags &requestedQueueTypes,
		vk::PhysicalDeviceFeatures2 &features,
		InstanceVersion instanceVersion);

	const vk::raii::Device& getVkDevice() const { return m_device; }
	vk::raii::Device&		getVkDevice() { return m_device; }
	const PhysicalDevice&	getPhysicalDevice() const { return m_physicalDevice; }

	uint32_t getMemoryType(uint32_t typeBits, vk::MemoryPropertyFlags properties, vk::Bool32& memTypeFound) const;

	struct {
		uint32_t graphics;
		uint32_t compute;
		uint32_t transfer;
	} queueFamilyIndices{};

private:
	vk::raii::Device      m_device{VK_NULL_HANDLE};
	const PhysicalDevice& m_physicalDevice;

	uint32_t getQueueFamilyIndex(vk::QueueFlags flags) const;
};

}
#endif