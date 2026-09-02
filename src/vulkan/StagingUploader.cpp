#include "StagingUploader.h"

namespace ptvk {
StagingUploader::StagingUploader(const ResourceAllocator &allocator) : m_allocator(allocator){}

void StagingUploader::appendImage(Image &image, const void *data, size_t size, vk::ImageLayout newLayout) {
    vk::ImageSubresourceRange imgSubRange = {
        .aspectMask = vk::ImageAspectFlagBits::eColor,
        .baseMipLevel = 0,
        .levelCount = vk::RemainingMipLevels,
        .baseArrayLayer = 0,
        .layerCount = vk::RemainingArrayLayers
    };

    vk::ImageMemoryBarrier2 imgMemoryBarrier = {
        .srcStageMask = vk::PipelineStageFlagBits2::eNone,
        .srcAccessMask = vk::AccessFlagBits2::eNone,
        .dstStageMask = vk::PipelineStageFlagBits2::eTransfer,
        .dstAccessMask = vk::AccessFlagBits2::eTransferWrite,
        .image = image.image,
        .subresourceRange = imgSubRange
    };

    

    Buffer buffer = createStagingBuffer(data, size);
}

void StagingUploader::uploadAppendedCmd(vk::raii::CommandBuffer &cmd) {
}

void StagingUploader::releaseStaging() {
}

Buffer StagingUploader::createStagingBuffer(const void *data, size_t size) {
    return ;
}
}
