#ifndef VMA_IMPLEMENTATION
#define VMA_IMPLEMENTATION
#include <vma/vk_mem_alloc.h>
#endif

#include "ResourceAllocator.h"

namespace ptvk {
ResourceAllocator::ResourceAllocator(const VmaAllocatorCreateInfo &allocatorInfo, Device* device) : m_pDevice(device){
     vmaCreateAllocator(&allocatorInfo, &m_allocator);
}

ResourceAllocator::~ResourceAllocator() {
     vmaDestroyAllocator(m_allocator);
}

Buffer ResourceAllocator::createBuffer(const vk::BufferCreateInfo& buffInfo, const VmaAllocationCreateInfo& allocCreateInfo, vk::DeviceSize minAlignment) const {
     Buffer buffer{};

     VmaAllocationInfo vmaAllocInfo{};
     VkBuffer bufferRawHandle{};

     auto result = vmaCreateBufferWithAlignment(m_allocator, &*buffInfo, &allocCreateInfo, minAlignment, &bufferRawHandle, &buffer.allocation, &vmaAllocInfo);

     if (result != VK_SUCCESS) {
          throw std::runtime_error("Failed to create buffer");
     }

     buffer.buffer = bufferRawHandle;
     buffer.bufferSize = vmaAllocInfo.size;
     buffer.pMapping = static_cast<uint8_t *>(vmaAllocInfo.pMappedData);

     vk::BufferDeviceAddressInfo buffDeviceAddrInfo{
          .buffer = buffer.buffer,
     };
     buffer.address = m_pDevice->getVkDevice().getBufferAddress(buffDeviceAddrInfo);

     buffer.allocator = m_allocator;

     return std::move(buffer);
}

void ResourceAllocator::destroyBuffer(Buffer &buffer) const {
     vmaDestroyBuffer(m_allocator, buffer.buffer, buffer.allocation);
     buffer = {};
}

void ResourceAllocator::destroyImage(Image &image) const {
     vmaDestroyImage(m_allocator, image.image, image.allocation);
}

Image ResourceAllocator::createImage(const vk::ImageCreateInfo& imageInfo, const vk::ImageViewCreateInfo& viewInfo, const VmaAllocationCreateInfo &allocCreateInfo) const {
     Image image = createImage(imageInfo, allocCreateInfo);

     // Create image view
     vk::ImageViewCreateInfo viewInfoTmp = viewInfo;
     viewInfoTmp.image = image.image;
     viewInfoTmp.format = image.format;
     image.view = vk::raii::ImageView(m_pDevice->getVkDevice(), viewInfoTmp);

     // Print memory properties of new allocation
     //VkMemoryPropertyFlags memPropFlags;
     //m_resourceAllocator->GetAllocationMemoryProperties(m_depthImages[i].allocation, memPropFlags);
     //std::cout << "Depth image memory usage flags:\n");
     //PrintMemoryPropertyFlags(memPropFlags);

     return image;
}

Image ResourceAllocator::createImage(const vk::ImageCreateInfo& imageInfo, const VmaAllocationCreateInfo& allocCreateInfo) const {
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
}
