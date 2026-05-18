#ifndef VK_RAYTRACING_RESOURCE_ALLOCATOR_H
#define VK_RAYTRACING_RESOURCE_ALLOCATOR_H

#include "Resources.h"
#include "Device.h"

namespace ptvk {
class ResourceAllocator {
public:
    explicit ResourceAllocator(const VmaAllocatorCreateInfo &allocatorInfo, Device* device);
    ~ResourceAllocator();

    [[nodiscard]] Buffer CreateBuffer(const vk::BufferCreateInfo &buffInfo, const VmaAllocationCreateInfo &allocCreateInfo, vk::DeviceSize minAlignment = 0) const;
    [[nodiscard]] Image CreateImage(const vk::ImageCreateInfo& imageInfo, const vk::ImageViewCreateInfo& imageViewInfo, const VmaAllocationCreateInfo& allocCreateInfo) const;
    [[nodiscard]] Image CreateImage(const vk::ImageCreateInfo& imageInfo, const VmaAllocationCreateInfo& allocCreateInfo) const;

    void DestroyBuffer(Buffer &buffer) const;
    void DestroyImage(Image &image) const;

    void GetAllocationInfo(VmaAllocation allocation, VkMemoryPropertyFlags* flags) const { return vmaGetAllocationMemoryProperties(m_allocator, allocation, flags);};
private:
    VmaAllocator m_allocator{};
    Device* m_device{};
};
}


#endif //VK_RAYTRACING_RESOURCE_ALLOCATOR_H
