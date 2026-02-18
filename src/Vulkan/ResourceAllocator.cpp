#include "ResourceAllocator.h"
#include <iostream>

namespace PathTracingVk {
ResourceAllocator::ResourceAllocator(const VmaAllocatorCreateInfo &allocatorInfo, VulkanDevice* device) : m_device(device){
     vmaCreateAllocator(&allocatorInfo, &m_allocator);
}

ResourceAllocator::~ResourceAllocator() {
     vmaDestroyAllocator(m_allocator);
}

Buffer ResourceAllocator::CreateBuffer(const vk::BufferCreateInfo& buffInfo, const VmaAllocationCreateInfo& allocInfo, vk::DeviceSize minAlignment) const {
     Buffer buffer{};

     VmaAllocationInfo vmaAllocInfo{};
     VkBuffer bufferRawHandle{};

     vmaCreateBufferWithAlignment(m_allocator, &*buffInfo, &allocInfo, minAlignment, &bufferRawHandle, &buffer.allocation, &vmaAllocInfo);

     buffer.buffer = vk::raii::Buffer(m_device->GetDevice(), bufferRawHandle);
     buffer.bufferSize = buffInfo.size;
     buffer.mapping = vmaAllocInfo.pMappedData;

     vk::BufferDeviceAddressInfo buffDeviceAddrInfo{
          .buffer = buffer.buffer,
     };
     buffer.address = m_device->GetDevice().getBufferAddress(buffDeviceAddrInfo);

     return buffer;
}

Image ResourceAllocator::CreateImage(const vk::ImageCreateInfo &imageInfo, const vk::ImageViewCreateInfo &imageViewInfo,
     const VmaAllocationCreateInfo &vmaInfo) const {
     Image image{};


     return image;
}
}
