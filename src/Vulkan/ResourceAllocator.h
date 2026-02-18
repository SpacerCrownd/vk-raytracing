#ifndef VK_RAYTRACING_RESOURCE_ALLOCATOR_H
#define VK_RAYTRACING_RESOURCE_ALLOCATOR_H

#include "Resources.h"
#include "VulkanDevice.h"

namespace PathTracingVk {
class ResourceAllocator {
public:
    explicit ResourceAllocator(const VmaAllocatorCreateInfo &allocatorInfo, VulkanDevice* device);
    ~ResourceAllocator();

    [[nodiscard]] Buffer CreateBuffer(const vk::BufferCreateInfo &buffInfo, const VmaAllocationCreateInfo &allocCreateInfo, vk::DeviceSize minAlignment = 0) const;
    [[nodiscard]] Image CreateImage(const vk::ImageCreateInfo& imageInfo, const vk::ImageViewCreateInfo& imageViewInfo, const VmaAllocationCreateInfo& vmaInfo) const;
private:
    VmaAllocator m_allocator{};
    VulkanDevice* m_device{};
};
}


#endif //VK_RAYTRACING_RESOURCE_ALLOCATOR_H
