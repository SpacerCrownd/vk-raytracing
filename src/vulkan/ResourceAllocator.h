#ifndef VK_RAYTRACING_RESOURCE_ALLOCATOR_H
#define VK_RAYTRACING_RESOURCE_ALLOCATOR_H

#include "Resources.h"
#include "Device.h"

namespace ptvk {
class ResourceAllocator {
public:
    ResourceAllocator(const VmaAllocatorCreateInfo &allocatorInfo, Device* device);
    ~ResourceAllocator();

    Buffer createBuffer(const vk::BufferCreateInfo &buffInfo, const VmaAllocationCreateInfo &allocCreateInfo, vk::DeviceSize minAlignment = 0) const;
    Image  createImage(const vk::ImageCreateInfo& imageInfo, const vk::ImageViewCreateInfo& imageViewInfo, const VmaAllocationCreateInfo& allocCreateInfo) const;
    Image  createImage(const vk::ImageCreateInfo& imageInfo, const VmaAllocationCreateInfo& allocCreateInfo) const;

    void getAllocationInfo(VmaAllocation allocation, VkMemoryPropertyFlags* flags) const { return vmaGetAllocationMemoryProperties(m_allocator, allocation, flags);};

    void destroyBuffer(Buffer &buffer) const;
    void destroyImage(Image &image) const;

private:
    VmaAllocator m_allocator{};
    Device*      m_pDevice{};
};
}


#endif //VK_RAYTRACING_RESOURCE_ALLOCATOR_H
