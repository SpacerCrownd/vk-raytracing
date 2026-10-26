#include "OidnDenoiser.h"

#include <chrono>
#include <iostream>

#include "Utils.h"

namespace ptvk {
OidnDenoiser::OidnDenoiser(uint32_t width, uint32_t height, const ResourceAllocator& allocator) : m_width(width), m_height(height){
    m_device = oidn::newDevice(oidn::DeviceType::CPU);
    m_device.commit();
    std::cout << "[OIDN] Device created" << std::endl;
    const size_t pixelCount = static_cast<size_t>(width) * height;

    m_output.resize(pixelCount * 4);

    m_filter = m_device.newFilter("RT");
    m_filter.set("hdr", true);
    m_filter.set("cleanAux", true);
    m_filter.set("quality", oidn::Quality::Fast);
    std::cout << "[OIDN] Filters created" << std::endl;

    checkOidnError();

    for (auto& m_imgBuffer : m_imgBuffers) {
        VmaAllocationCreateInfo allocInfo = {
            .flags = VMA_ALLOCATION_CREATE_HOST_ACCESS_RANDOM_BIT |
                     VMA_ALLOCATION_CREATE_MAPPED_BIT,
            .usage = VMA_MEMORY_USAGE_AUTO,
            .requiredFlags = VK_MEMORY_PROPERTY_HOST_COHERENT_BIT
        };

        vk::BufferCreateInfo bufferInfo = {
            .size = m_output.size() * sizeof(float),
            .usage = vk::BufferUsageFlagBits::eTransferDst
        };

        m_imgBuffer = allocator.createBuffer(bufferInfo, allocInfo);
    }
}

void OidnDenoiser::cmdCopyImageToBuffer(const vk::raii::CommandBuffer& cmd, const Image& img, shaderio::OutputImage outputImageType) {
    auto& buffer = m_imgBuffers[outputImageType];

    vk::BufferImageCopy2 region{};
    region.bufferOffset = 0;
    region.bufferRowLength = 0;    // Tightly packed rows
    region.bufferImageHeight = 0;  // Tightly packed image

    region.imageSubresource.aspectMask = vk::ImageAspectFlagBits::eColor;
    region.imageSubresource.mipLevel = 0;
    region.imageSubresource.baseArrayLayer = 0;
    region.imageSubresource.layerCount = 1;

    region.imageOffset = vk::Offset3D{0, 0, 0};
    region.imageExtent = vk::Extent3D{m_width, m_height, 1};

    vk::CopyImageToBufferInfo2 copyInfo{};
    copyInfo.srcImage = img.image;
    copyInfo.srcImageLayout = vk::ImageLayout::eTransferSrcOptimal;
    copyInfo.dstBuffer = buffer.buffer;
    copyInfo.regionCount = 1;
    copyInfo.pRegions = &region;

    cmd.copyImageToBuffer2(copyInfo);
}

void OidnDenoiser::synchronousDenoise() {
    //using Clock = std::chrono::steady_clock;

    //auto t0 = Clock::now();
    m_filter.setImage("color", m_imgBuffers[shaderio::OutputImage::eResult].pMapping, oidn::Format::Float3, m_width, m_height, 0, 4 * sizeof(float), m_width * 4 * sizeof(float));
    m_filter.setImage("albedo", m_imgBuffers[shaderio::OutputImage::eAlbedo].pMapping, oidn::Format::Float3, m_width, m_height, 0, 4 * sizeof(float), m_width * 4 * sizeof(float));
    m_filter.setImage("normal", m_imgBuffers[shaderio::OutputImage::eNormal].pMapping, oidn::Format::Float3, m_width, m_height, 0, 4 * sizeof(float), m_width * 4 * sizeof(float));
    m_filter.setImage("output", m_output.data(), oidn::Format::Float3, m_width, m_height, 0, 4 * sizeof(float), m_width * 4 * sizeof(float));

    m_filter.commit();
    //auto t1 = Clock::now();
    m_filter.execute(); // synchronous
    //auto t2 = Clock::now();
    //std::cout << "commit: " << std::chrono::duration<double, std::milli>(t1 - t0).count() << " ms, execute: "<< std::chrono::duration<double, std::milli>(t2 - t1).count() << " ms\n";
    checkOidnError();
}

void OidnDenoiser::checkOidnError() {
    const char* message = nullptr;
    const oidn::Error error = m_device.getError(message);

    if (error != oidn::Error::None) {
        throw std::runtime_error(message ? message : "OIDN operation failed");
    }
}
}
