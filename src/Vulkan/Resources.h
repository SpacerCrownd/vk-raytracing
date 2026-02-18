#ifndef VK_RAYTRACING_RESOURCES_H
#define VK_RAYTRACING_RESOURCES_H

#include "vk_mem_alloc.h"
#include "Vulkan.h"

namespace PathTracingVk {
struct Buffer {
    vk::raii::Buffer buffer{VK_NULL_HANDLE};
    vk::DeviceSize bufferSize{};
    vk::DeviceAddress address{}; // buffer address in shader (Buffer Device Address extension)
    VmaAllocation allocation{};
    void* mapping{};
};

struct Image {
    vk::raii::Image image{VK_NULL_HANDLE};
    vk::raii::ImageView view{VK_NULL_HANDLE};
    vk::Extent3D extent{};
    vk::Format format{};
    VmaAllocation allocation{};
};

struct AccelerationStructure {
    vk::AccelerationStructureKHR accel{};
    vk::DeviceAddress address{};
    Buffer buffer{};
};
}

#endif //VK_RAYTRACING_RESOURCES_H