#ifndef VK_RAYTRACING_STAGINGUPLOADER_H
#define VK_RAYTRACING_STAGINGUPLOADER_H

#include "Vulkan.h"
#include "Resources.h"
#include "ResourceAllocator.h"

namespace ptvk {
class StagingUploader {
public:
    StagingUploader(const ResourceAllocator& allocator);

    void appendImage(const Image &image,
                     const void *data,
                     size_t size,
                     vk::ImageLayout oldLayout,
                     vk::ImageLayout newLayout);
    void uploadAppendedCmd(const vk::raii::CommandBuffer& cmd); // after usage, need wait device idle to ensure operations are completed
    void releaseStaging();
private:
    const ResourceAllocator& m_allocator;

    std::vector<Buffer>                     m_stagingBuffers;
    std::vector<vk::CopyBufferToImageInfo2> m_copyBufferImageInfos;
    std::vector<vk::BufferImageCopy2>       m_copyBufferImageRegions;
    std::vector<vk::ImageMemoryBarrier2>    m_preImageMemoryBarriers;
    std::vector<vk::ImageMemoryBarrier2>    m_postImageMemoryBarriers;

    Buffer &createStagingBuffer(const void *data, size_t size);
};
}


#endif //VK_RAYTRACING_STAGINGUPLOADER_H
