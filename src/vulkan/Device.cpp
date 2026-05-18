#include "Device.h"

namespace ptvk {
Device::Device(PhysicalDevice &device, std::vector<const char*> devExtensions, vk::QueueFlags requestedQueueTypes, vk::PhysicalDeviceFeatures2 &features, InstanceVersion instanceVersion) : m_physicalDevice(device) {
    std::vector<vk::DeviceQueueCreateInfo> queueCreateInfos;

    constexpr float defaultQueuePriority(0.0f);

    // graphics queue creation
    if (requestedQueueTypes & vk::QueueFlagBits::eGraphics) {
        queueFamilyIndices.graphics = GetQueueFamilyIndex(vk::QueueFlagBits::eGraphics);
        auto queueCreateInfo = vk::DeviceQueueCreateInfo{
            .sType = vk::StructureType::eDeviceQueueCreateInfo,
            .queueFamilyIndex = queueFamilyIndices.graphics,
            .queueCount = 1,
            .pQueuePriorities = &defaultQueuePriority
        };
        queueCreateInfos.push_back(queueCreateInfo);
    }else {
        queueFamilyIndices.graphics = 0;
    }
    // dedicated compute queue creation
    if (requestedQueueTypes & vk::QueueFlagBits::eCompute) {
        queueFamilyIndices.compute = GetQueueFamilyIndex(vk::QueueFlagBits::eCompute);
        auto queueCreateInfo = vk::DeviceQueueCreateInfo{
            .sType = vk::StructureType::eDeviceQueueCreateInfo,
            .queueFamilyIndex = queueFamilyIndices.compute,
            .queueCount = 1,
            .pQueuePriorities = &defaultQueuePriority
        };
        queueCreateInfos.push_back(queueCreateInfo);
    } else {
        // else use same queue
        queueFamilyIndices.compute = queueFamilyIndices.graphics;
    }
    // dedicated transfer queue creation
    if (requestedQueueTypes & vk::QueueFlagBits::eTransfer) {
        queueFamilyIndices.transfer = GetQueueFamilyIndex(vk::QueueFlagBits::eTransfer);
        auto queueCreateInfo = vk::DeviceQueueCreateInfo{
            .sType = vk::StructureType::eDeviceQueueCreateInfo,
            .queueFamilyIndex = queueFamilyIndices.transfer,
            .queueCount = 1,
            .pQueuePriorities = &defaultQueuePriority
        };
        queueCreateInfos.push_back(queueCreateInfo);
    }else {
        queueFamilyIndices.transfer = queueFamilyIndices.graphics;
    }

    bool deviceSupportsDynamicRendering = m_physicalDevice.IsExtensionSupported(vk::KHRDynamicRenderingExtensionName);
    bool instance_is_1_3_or_more = (instanceVersion.Major >= 1) || (instanceVersion.Minor >= 3);
    if (instance_is_1_3_or_more && deviceSupportsDynamicRendering) {
        printf("[INFO] The Vulkan instance and device support dynamic rendering as a core feature\n");
    }
    else if (instanceVersion.Minor == 2) {
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

    vk::DeviceCreateInfo deviceCreateInfo = {
        .pNext = &features,
        .queueCreateInfoCount = static_cast<uint32_t>(queueCreateInfos.size()),
        .pQueueCreateInfos = queueCreateInfos.data(),
        .enabledExtensionCount = static_cast<uint32_t>(devExtensions.size()),
        .ppEnabledExtensionNames = devExtensions.data(),
    };

    m_device = std::make_unique<vk::raii::Device>(m_physicalDevice.m_physDevice, deviceCreateInfo);

    printf("\n[INFO] Device created\n");
}

uint32_t Device::GetMemoryType(uint32_t typeBits, vk::MemoryPropertyFlags properties, vk::Bool32& memTypeFound) const {
    for (uint32_t i = 0; i < m_physicalDevice.m_memProperties.memoryTypeCount; i++)
    {
        if ((typeBits & 1) == 1)
        {
            if ((m_physicalDevice.m_memProperties.memoryTypes[i].propertyFlags & properties) == properties)
            {
                if (memTypeFound)
                {
                    memTypeFound = true;
                }
                return i;
            }
        }
        typeBits >>= 1;
    }

    if (memTypeFound)
    {
        memTypeFound = false;
        return 0;
    }
    throw std::runtime_error("Could not find a matching memory type");
}

uint32_t Device::GetQueueFamilyIndex(vk::QueueFlags flags) const {
    // Dedicated queue for compute
    // Try to find a queue family index that supports compute but not graphics
    if (flags & vk::QueueFlagBits::eCompute)
    {
        for (uint32_t i = 0; i < static_cast<uint32_t>(m_physicalDevice.m_qFamilyProperties.size()); i++)
        {
            if (m_physicalDevice.m_qFamilyProperties[i].queueFlags & vk::QueueFlagBits::eCompute && !(m_physicalDevice.m_qFamilyProperties[i].queueFlags & vk::QueueFlagBits::eGraphics))
            {
                return i;
            }
        }
    }

    // Dedicated queue for transfer
    // Try to find a queue family index that supports transfer but not graphics and compute
    if (flags & vk::QueueFlagBits::eTransfer)
    {
        for (uint32_t i = 0; i < static_cast<uint32_t>(m_physicalDevice.m_qFamilyProperties.size()); i++)
        {
            if ((m_physicalDevice.m_qFamilyProperties[i].queueFlags & vk::QueueFlagBits::eTransfer) && !(m_physicalDevice.m_qFamilyProperties[i].queueFlags & vk::QueueFlagBits::eGraphics) && !(m_physicalDevice.m_qFamilyProperties[i].queueFlags & vk::QueueFlagBits::eCompute))
            {
                return i;
            }
        }
    }
    // For other queue types or if no separate compute queue is present, return the first one to support the requested flags

    printf("Check %d\n", static_cast<int>(m_physicalDevice.m_qFamilyProperties.size()));

    for (uint32_t i = 0; i < static_cast<uint32_t>(m_physicalDevice.m_qFamilyProperties.size()); i++)
    {
        if (m_physicalDevice.m_qFamilyProperties[i].queueFlags & flags)
        {
            return i;
        }
    }

    throw std::runtime_error("Could not find a matching queue family index");
}
}
