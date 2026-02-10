#ifndef VK_RAYTRACING_RESOURCES_H
#define VK_RAYTRACING_RESOURCES_H

#include "vk_mem_alloc.h"
#include "Vulkan.h"

namespace PathTracerVk {
struct Buffer {
    vk::raii::Buffer buffer{VK_NULL_HANDLE};
    vk::DeviceSize bufferSize{};
    vk::DeviceAddress address{};
    VmaAllocation allocation{};
};

struct Image {
    vk::raii::Image image{VK_NULL_HANDLE};
    vk::raii::ImageView view{VK_NULL_HANDLE};
    vk::Extent3D extent{};
    vk::Format format{};
    VmaAllocation allocation{};
};

struct AccelerationStructure {
    vk::AccelerationStructureKHR as{};
    vk::DeviceAddress device{};
};
}

#endif //VK_RAYTRACING_RESOURCES_H