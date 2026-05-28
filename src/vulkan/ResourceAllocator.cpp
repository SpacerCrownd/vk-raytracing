#ifndef VMA_IMPLEMENTATION
#define VMA_IMPLEMENTATION
#include <vma/vk_mem_alloc.h>
#endif

#include "ResourceAllocator.h"

namespace ptvk {
ResourceAllocator::ResourceAllocator(const VmaAllocatorCreateInfo &allocatorInfo, Device* device) : m_device(device){
     vmaCreateAllocator(&allocatorInfo, &m_allocator);
}

ResourceAllocator::~ResourceAllocator() {
     vmaDestroyAllocator(m_allocator);
}

Buffer ResourceAllocator::CreateBuffer(const vk::BufferCreateInfo& buffInfo, const VmaAllocationCreateInfo& allocCreateInfo, vk::DeviceSize minAlignment) const {
     Buffer buffer{};

     VmaAllocationInfo vmaAllocInfo{};
     VkBuffer bufferRawHandle{};

     auto result = vmaCreateBufferWithAlignment(m_allocator, &*buffInfo, &allocCreateInfo, minAlignment, &bufferRawHandle, &buffer.allocation, &vmaAllocInfo);

     if (result != VK_SUCCESS) {
          throw std::runtime_error("Failed to create buffer");
     }

     buffer.buffer = bufferRawHandle;
     buffer.bufferSize = buffInfo.size;
     buffer.mapping = static_cast<uint8_t *>(vmaAllocInfo.pMappedData);

     vk::BufferDeviceAddressInfo buffDeviceAddrInfo{
          .buffer = buffer.buffer,
     };
     buffer.address = m_device->GetVkDevice().getBufferAddress(buffDeviceAddrInfo);

     buffer.allocator = m_allocator;

     return std::move(buffer);
}

void ResourceAllocator::DestroyBuffer(Buffer &buffer) const {
     vmaDestroyBuffer(m_allocator, buffer.buffer, buffer.allocation);
     buffer = {};
}

Image ResourceAllocator::CreateImage(const vk::ImageCreateInfo& imageInfo, const VmaAllocationCreateInfo& allocCreateInfo) const {
     Image image{};
     VmaAllocationInfo allocInfo{};
     VkImage imageRawHandle{};

     auto result = vmaCreateImage(m_allocator, &*imageInfo, &allocCreateInfo, &imageRawHandle, &image.allocation, &allocInfo);

     if (result != VK_SUCCESS) {
          throw std::runtime_error("Failed to create image");
     }

     image.image = vk::Image(imageRawHandle);
     image.extent = imageInfo.extent;
     image.format = imageInfo.format;
     image.mipLevels = imageInfo.mipLevels;
     image.arrayLayers = imageInfo.arrayLayers;
     image.allocator = m_allocator;

     return std::move(image);
}

Image ResourceAllocator::CreateImage(const vk::ImageCreateInfo& imageInfo, const vk::ImageViewCreateInfo& viewInfo, const VmaAllocationCreateInfo &allocCreateInfo) const {
     Image image = CreateImage(imageInfo, allocCreateInfo);

     // Create image view
     vk::ImageViewCreateInfo viewInfoTmp = viewInfo;
     viewInfoTmp.image = image.image;
     viewInfoTmp.format = image.format;
     image.view = m_device->GetVkDevice().createImageView(viewInfoTmp);
     return image;
}
}
