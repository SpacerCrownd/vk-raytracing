#ifndef VK_RAYTRACING_PHYSICAL_DEVICE_H
#define VK_RAYTRACING_PHYSICAL_DEVICE_H

#include "Vulkan.h"
#include <vector>

struct PhysicalDevice {
    vk::raii::PhysicalDevice m_physDevice = VK_NULL_HANDLE;

    vk::PhysicalDeviceProperties2      m_devProperties2{};
    vk::SurfaceCapabilitiesKHR         m_surfaceCapabilities{};
    vk::PhysicalDeviceMemoryProperties m_memProperties{};
    vk::PhysicalDeviceFeatures2        m_features2{};
    vk::Format                         m_depthFormat{};

    std::vector<vk::ExtensionProperties> m_extensions;

    std::vector<vk::SurfaceFormatKHR> m_surfaceFormats;
    std::vector<vk::PresentModeKHR>   m_presentModes;

    vk::PhysicalDeviceRayTracingPipelinePropertiesKHR    m_rtProperties{vk::StructureType::ePhysicalDeviceRayTracingPipelinePropertiesKHR};
    vk::PhysicalDeviceAccelerationStructurePropertiesKHR m_asProperties{vk::StructureType::ePhysicalDeviceAccelerationStructurePropertiesKHR};

    std::vector<vk::QueueFamilyProperties> m_queueFamilyProperties;
    std::vector<vk::Bool32>                m_queueSupportsPresent;

    bool isExtensionSupported(const char* pExt) const;

    vk::PhysicalDeviceRayTracingPipelinePropertiesKHR    getRayTracingPipelinePropertiesKHR() const { return m_rtProperties; }
    vk::PhysicalDeviceAccelerationStructurePropertiesKHR getAccelerationStructurePropertiesKHR() const { return m_asProperties; }
};


#endif //VK_RAYTRACING_PHYSICAL_DEVICE_H