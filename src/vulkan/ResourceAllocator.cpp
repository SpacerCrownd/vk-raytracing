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

     vmaCreateBufferWithAlignment(m_allocator, &*buffInfo, &allocCreateInfo, minAlignment, &bufferRawHandle, &buffer.allocation, &vmaAllocInfo);

     buffer.buffer = vk::raii::Buffer(m_device->GetVkDevice(), bufferRawHandle);
     buffer.bufferSize = buffInfo.size;
     buffer.mapping = static_cast<uint8_t *>(vmaAllocInfo.pMappedData);

     vk::BufferDeviceAddressInfo buffDeviceAddrInfo{
          .buffer = buffer.buffer,
     };
     buffer.address = m_device->GetVkDevice().getBufferAddress(buffDeviceAddrInfo);

     return buffer;
}

void ResourceAllocator::DestroyBuffer(Buffer &buffer) const {
     vmaDestroyBuffer(m_allocator, *buffer.buffer, buffer.allocation);
     buffer = {};
}

Image ResourceAllocator::CreateImage(const vk::ImageCreateInfo& imageInfo, const VmaAllocationCreateInfo& allocCreateInfo) const {
     Image image{};
     VmaAllocationInfo allocInfo{};
     VkImage imageRawHandle{};

     vmaCreateImage(m_allocator, &*imageInfo, &allocCreateInfo, &imageRawHandle, &image.allocation, &allocInfo);
     image.image = vk::raii::Image(m_device->GetVkDevice(), imageRawHandle);
     image.extent = imageInfo.extent;
     image.format = imageInfo.format;
     image.mipLevels = imageInfo.mipLevels;
     image.arrayLayers = imageInfo.arrayLayers;

     return image;
}

Image ResourceAllocator::CreateImage(const vk::ImageCreateInfo& imageInfo, const vk::ImageViewCreateInfo& viewInfo, const VmaAllocationCreateInfo &allocCreateInfo) const {
     Image image{};
     image = CreateImage(imageInfo, allocCreateInfo);

     // Create image view
     vk::ImageViewCreateInfo viewInfoTmp = viewInfo;
     viewInfoTmp.image = image.image;
     viewInfoTmp.format = image.format;
     image.view = vk::raii::ImageView(m_device->GetVkDevice(), viewInfoTmp);

     return image;
}

void ResourceAllocator::DestroyImage(Image &image) const {
     vmaDestroyImage(m_allocator, *image.image, image.allocation);
     image = {};
}
}
