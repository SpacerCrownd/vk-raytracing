#ifndef VK_RAYTRACING_RESOURCES_H
#define VK_RAYTRACING_RESOURCES_H

#include <vma/vk_mem_alloc.h>
#include "Vulkan.h"

namespace ptvk {
struct Buffer {
    vk::Buffer buffer{VK_NULL_HANDLE};
    vk::DeviceSize bufferSize{};
    vk::DeviceAddress address{}; // buffer address in shader (Buffer Device Address extension)
    VmaAllocation allocation{};
    VmaAllocator allocator{};
    uint8_t* mapping{};

    Buffer() = default;

    Buffer(const Buffer&) = delete;
    Buffer& operator=(const Buffer&) = delete;

    Buffer(Buffer&& other) noexcept
        : buffer(std::move(other.buffer)),
          bufferSize(other.bufferSize),
          address(other.address),
          allocation(other.allocation),
          allocator(other.allocator),
          mapping(other.mapping)
    {
        other.allocation = nullptr;
        other.mapping = nullptr;
    }

    Buffer& operator=(Buffer&& other) noexcept {
        if (this != &other) {
            cleanup();

            buffer = std::move(other.buffer);
            bufferSize = other.bufferSize;
            address = other.address;
            allocation = other.allocation;
            allocator = other.allocator;
            mapping = other.mapping;

            other.allocation = nullptr;
            other.mapping = nullptr;
        }
        return *this;
    }

    ~Buffer() {
        cleanup();
    }

private:
    void cleanup() {
        if (allocation) {
            vmaDestroyBuffer(allocator, buffer, allocation);

            allocation = nullptr;
        }
    }
};

struct Image {
    vk::raii::Image image{VK_NULL_HANDLE};
    vk::raii::ImageView view{VK_NULL_HANDLE};
    vk::Extent3D extent{};
    vk::Format format{};
    uint32_t mipLevels{};
    uint32_t arrayLayers{};
    VmaAllocation allocation{};
};

struct AccelerationStructure {
    vk::AccelerationStructureKHR accel{};
    vk::DeviceAddress address{};
    Buffer buffer{};
};
}

#endif //VK_RAYTRACING_RESOURCES_H