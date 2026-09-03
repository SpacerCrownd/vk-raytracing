#include "StagingUploader.h"
#include "Utils.h"

namespace ptvk {
StagingUploader::StagingUploader(const ResourceAllocator &allocator) : m_allocator(allocator){}

void StagingUploader::appendImage(const Image &image, const void *data, size_t size, vk::ImageLayout oldLayout, vk::ImageLayout newLayout) {
    // add layout transition to transfer dst before copy
    utils::ImageMemoryBarrierParams params = {
        .image = image.image,
        .oldLayout = oldLayout,
        .newLayout = vk::ImageLayout::eTransferDstOptimal
    };
    m_preImageMemoryBarriers.push_back(utils::makeImageMemoryBarrier(params));

    // add layout transition to specified new layout after copy
    params.oldLayout = vk::ImageLayout::eTransferDstOptimal;
    params.newLayout = newLayout;
    m_postImageMemoryBarriers.push_back(utils::makeImageMemoryBarrier(params));

    // create staging buffer and append copy operations
    Buffer& buffer = createStagingBuffer(data, size);
    vk::BufferImageCopy2 copyBufferImageRegion = {
        .bufferOffset = 0,
        .bufferRowLength = 0,
        .bufferImageHeight = 0,
        .imageSubresource = {
            .aspectMask = vk::ImageAspectFlagBits::eColor,
            .mipLevel = 0,
            .baseArrayLayer = 0,
            .layerCount = 1,
        },
        .imageOffset = 0,
        .imageExtent = image.extent
    };
    vk::CopyBufferToImageInfo2 copyBufferToImageInfo = {
        .srcBuffer = buffer.buffer,
        .dstImage = image.image,
        .dstImageLayout = vk::ImageLayout::eTransferDstOptimal,
        .regionCount = 1,
        .pRegions = nullptr // set when uploading appended cmds
    };
    m_copyBufferImageRegions.push_back(copyBufferImageRegion);
    m_copyBufferImageInfos.push_back(copyBufferToImageInfo);
}

void StagingUploader::uploadAppendedCmd(const vk::raii::CommandBuffer &cmd) {
    // pre-op image transitions
    vk::DependencyInfo dependencyInfo = {
        .imageMemoryBarrierCount = static_cast<uint32_t>(m_preImageMemoryBarriers.size()),
        .pImageMemoryBarriers = m_preImageMemoryBarriers.data(),
    };
    cmd.pipelineBarrier2(dependencyInfo);

    // for each copy info, set regions by region count
    size_t regionOffset = 0;
    for(size_t i = 0; i < m_copyBufferImageInfos.size(); i++)
    {
        m_copyBufferImageInfos[i].pRegions = &m_copyBufferImageRegions[regionOffset];
        regionOffset += m_copyBufferImageInfos[i].regionCount;
        cmd.copyBufferToImage2(m_copyBufferImageInfos[i]);
    }

    // post-op image transitions
    dependencyInfo.imageMemoryBarrierCount = static_cast<uint32_t>(m_postImageMemoryBarriers.size());
    dependencyInfo.pImageMemoryBarriers = m_postImageMemoryBarriers.data();
    cmd.pipelineBarrier2(dependencyInfo);
}

void StagingUploader::releaseStaging() {
    m_stagingBuffers.clear();
    m_preImageMemoryBarriers.clear();
    m_postImageMemoryBarriers.clear();
    m_copyBufferImageInfos.clear();
    m_copyBufferImageRegions.clear();
}

Buffer &StagingUploader::createStagingBuffer(const void *data, size_t size)
{
    VmaAllocationCreateInfo allocInfo = {
        .flags = VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT |
                 VMA_ALLOCATION_CREATE_MAPPED_BIT,
        .usage = VMA_MEMORY_USAGE_AUTO,
        .requiredFlags = VK_MEMORY_PROPERTY_HOST_COHERENT_BIT
    };

    vk::BufferCreateInfo bufferInfo = {
        .size = size,
        .usage = vk::BufferUsageFlagBits::eTransferSrc |
                 vk::BufferUsageFlagBits::eTransferDst
    };

    m_stagingBuffers.emplace_back(
        m_allocator.createBuffer(bufferInfo, allocInfo)
    );

    Buffer& stagingBuffer = m_stagingBuffers.back();

    if (!stagingBuffer.pMapping) {
        throw std::runtime_error("[ERROR] Staging buffer could not be mapped");
    }

    if (data) {
        memcpy(stagingBuffer.pMapping, data, size);
    }

    return stagingBuffer;
}
}
