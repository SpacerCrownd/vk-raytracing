#ifndef OPENIMAGE_DENOISE
#define OPENIMAGE_DENOISE

#include <OpenImageDenoise/oidn.hpp>
#include "Resources.h"
#include "../shaders/shaderio.h.slang"
#include <vector>

#include "ResourceAllocator.h"

namespace ptvk {
class OidnDenoiser {
public:
    OidnDenoiser(uint32_t width, uint32_t height, const ResourceAllocator& allocator);
    ~OidnDenoiser() = default;

    void cmdCopyImageToBuffer(const vk::raii::CommandBuffer& cmd, const Image& img, shaderio::OutputImage outputImageType);
    void synchronousDenoise();

    const float* output() const { return m_output.data(); }
    size_t outputSize() const { return std::span(m_output).size_bytes(); }

private:
    void checkOidnError();

    uint32_t m_width = 0;
    uint32_t m_height = 0;

    oidn::DeviceRef m_device{};
    oidn::FilterRef m_filter{};
    std::vector<float> m_output{};
    std::array<Buffer, 3> m_imgBuffers{};
};
}

#endif