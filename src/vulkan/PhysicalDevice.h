#ifndef VK_RAYTRACING_PHYSICAL_DEVICE_H
#define VK_RAYTRACING_PHYSICAL_DEVICE_H

#include "Vulkan.h"
#include <vector>

struct PhysicalDevice {
    vk::raii::PhysicalDevice m_physDevice = VK_NULL_HANDLE;
    vk::PhysicalDeviceProperties2 m_devProperties2{};
    std::vector<vk::QueueFamilyProperties> m_qFamilyProperties;
    std::vector<vk::Bool32> m_qSupportsPresent;
    std::vector<vk::SurfaceFormatKHR> m_surfaceFormats;
    vk::SurfaceCapabilitiesKHR m_surfaceCapabilities{};
    vk::PhysicalDeviceMemoryProperties m_memProperties{};
    std::vector<vk::PresentModeKHR> m_presentModes;
    vk::PhysicalDeviceFeatures2 m_features2{};
    vk::Format m_depthFormat{};
    vk::PhysicalDeviceRayTracingPipelinePropertiesKHR m_rtProperties{vk::StructureType::ePhysicalDeviceRayTracingPipelinePropertiesKHR};
    vk::PhysicalDeviceAccelerationStructurePropertiesKHR m_asProperties{vk::StructureType::ePhysicalDeviceAccelerationStructurePropertiesKHR};

    struct {
        int Variant = 0;
        int Major = 0;
        int Minor = 0;
        int Patch = 0;
    } m_apiVersion;
    std::vector<vk::ExtensionProperties> m_extensions;

    bool IsExtensionSupported(const char* pExt) const;
    [[nodiscard]] vk::PhysicalDeviceRayTracingPipelinePropertiesKHR GetRayTracingPipelinePropertiesKHR() const { return m_rtProperties; }
    [[nodiscard]] vk::PhysicalDeviceAccelerationStructurePropertiesKHR GetAccelerationStructurePropertiesKHR() const { return m_asProperties; }
};


#endif //VK_RAYTRACING_PHYSICAL_DEVICE_H