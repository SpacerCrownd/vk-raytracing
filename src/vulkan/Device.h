#ifndef PHYSICAL_DEVICE_H
#define PHYSICAL_DEVICE_H

#include "Vulkan.h"
#include "PhysicalDevice.h"

namespace ptvk {

class Device {

public:
	Device(PhysicalDevice &device, std::vector<const char*> devExtensions, vk::QueueFlags requestedQueueTypes, vk::PhysicalDeviceFeatures2 &features, InstanceVersion instanceVersion);
	~Device() = default;

	[[nodiscard]] vk::raii::Device& GetVkDevice() const { return *m_device; }
	[[nodiscard]] PhysicalDevice& GetPhysicalDevice() const { return m_physicalDevice; }
	[[nodiscard]] uint32_t GetMemoryType(uint32_t typeBits, vk::MemoryPropertyFlags properties, vk::Bool32& memTypeFound) const;

	struct {
		uint32_t graphics;
		uint32_t compute;
		uint32_t transfer;
	} queueFamilyIndices{};

private:
	std::unique_ptr<vk::raii::Device> m_device;
	PhysicalDevice& m_physicalDevice;

	[[nodiscard]] uint32_t GetQueueFamilyIndex(vk::QueueFlags flags) const;
};

}
#endif