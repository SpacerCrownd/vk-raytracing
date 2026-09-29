#ifndef VK_RAYTRACING_RESOURCE_ALLOCATOR_H
#define VK_RAYTRACING_RESOURCE_ALLOCATOR_H

#include "AccelerationStructure.h"
#include "Resources.h"
#include "Device.h"

namespace ptvk {
class ResourceAllocator {
public:
    ResourceAllocator(const VmaAllocatorCreateInfo &allocatorInfo, const Device& device);
    ~ResourceAllocator();

    Buffer createBuffer(const vk::BufferCreateInfo &buffInfo, const VmaAllocationCreateInfo &allocCreateInfo) const;

    Buffer createBufferWithAlignment(const vk::BufferCreateInfo &buffInfo,
                                     const VmaAllocationCreateInfo &allocCreateInfo,
                                     vk::DeviceSize minAlignment) const;

    Image createImage(const vk::ImageCreateInfo& imageInfo, const vk::ImageViewCreateInfo& imageViewInfo, const VmaAllocationCreateInfo& allocCreateInfo) const;
    Image createImage(const vk::ImageCreateInfo& imageInfo, const VmaAllocationCreateInfo& allocCreateInfo) const;

    AccelerationStructure createAccelerationStructure(vk::AccelerationStructureCreateInfoKHR &createInfo) const;

    void getAllocationInfo(VmaAllocation allocation, VkMemoryPropertyFlags* flags) const { return vmaGetAllocationMemoryProperties(m_allocator, allocation, flags);}

    void destroyBuffer(const Buffer &buffer) const;
    void destroyImage(const Image &image) const;


private:
    const Device& m_device;
    VmaAllocator  m_allocator{};
};
}


#endif //VK_RAYTRACING_RESOURCE_ALLOCATOR_H
