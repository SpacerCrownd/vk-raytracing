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

    ~Buffer() {
        cleanup();
    }

    Buffer(const Buffer&) = delete;
    Buffer& operator=(const Buffer&) = delete;

    Buffer(Buffer&& other) noexcept {
        buffer = other.buffer;
        bufferSize = other.bufferSize;
        address = other.address;
        allocation = other.allocation;
        allocator = other.allocator;
        mapping = other.mapping;

        other.buffer = VK_NULL_HANDLE;
        other.allocation = VK_NULL_HANDLE;
        other.mapping = nullptr;
    }

    Buffer& operator=(Buffer&& other) noexcept {
        if (this != &other) {
            cleanup();

            buffer = other.buffer;
            bufferSize = other.bufferSize;
            address = other.address;
            allocation = other.allocation;
            allocator = other.allocator;
            mapping = other.mapping;

            other.buffer = VK_NULL_HANDLE;
            other.allocation = VK_NULL_HANDLE;
            other.mapping = nullptr;
        }
        return *this;
    }

private:
    void cleanup() {
        if (allocation) {
            vmaDestroyBuffer(allocator, buffer, allocation);
        }
    }
};

struct Image {
    vk::Image image{VK_NULL_HANDLE};
    vk::ImageView view{VK_NULL_HANDLE};
    vk::Extent3D extent{};
    vk::Format format{};
    uint32_t mipLevels{};
    uint32_t arrayLayers{};
    VmaAllocation allocation{};
    VmaAllocator allocator{};

    Image() = default;

    ~Image() {
        cleanup();
    }

    Image(const Image&) = delete;
    Image& operator=(const Image&) = delete;

    Image(Image&& other) noexcept {
        image = other.image;
        view = other.view;
        extent = other.extent;
        format = other.format;
        mipLevels = other.mipLevels;
        arrayLayers = other.arrayLayers;
        allocation = other.allocation;
        allocator = other.allocator;

        // invalidate source
        other.image = VK_NULL_HANDLE;
        other.view = VK_NULL_HANDLE;
        other.allocation = VK_NULL_HANDLE;
    }

    Image& operator=(Image&& other) noexcept {
        if (this != &other) {
            cleanup();

            image = other.image;
            view = other.view;
            extent = other.extent;
            format = other.format;
            mipLevels = other.mipLevels;
            arrayLayers = other.arrayLayers;
            allocation = other.allocation;
            allocator = other.allocator;

            // invalidate source
            other.image = VK_NULL_HANDLE;
            other.view = VK_NULL_HANDLE;
            other.allocation = VK_NULL_HANDLE;
        }
        return *this;
    }

private:
    void cleanup() {
        if (allocation) {
            vmaDestroyImage(allocator, image, allocation);
        }
    }
};

struct AccelerationStructure {
    vk::AccelerationStructureKHR accel{};
    vk::DeviceAddress address{};
    Buffer buffer{};
};
}

#endif //VK_RAYTRACING_RESOURCES_H