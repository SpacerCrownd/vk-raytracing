#ifndef VK_RAYTRACING_SAMPLERPOOL_H
#define VK_RAYTRACING_SAMPLERPOOL_H

#include <unordered_map>

#include "Vulkan.h"
#include "Utils.h"

namespace ptvk {
struct SamplerState {
  VkSamplerCreateInfo createInfo{};

  bool operator==(const SamplerState& other) const
  {
    return other.createInfo.flags == createInfo.flags && other.createInfo.magFilter == createInfo.magFilter
           && other.createInfo.minFilter == createInfo.minFilter && other.createInfo.mipmapMode == createInfo.mipmapMode
           && other.createInfo.addressModeU == createInfo.addressModeU && other.createInfo.addressModeV == createInfo.addressModeV
           && other.createInfo.addressModeW == createInfo.addressModeW && other.createInfo.mipLodBias == createInfo.mipLodBias
           && other.createInfo.anisotropyEnable == createInfo.anisotropyEnable
           && other.createInfo.maxAnisotropy == createInfo.maxAnisotropy && other.createInfo.compareEnable == createInfo.compareEnable
           && other.createInfo.compareOp == createInfo.compareOp && other.createInfo.minLod == createInfo.minLod
           && other.createInfo.maxLod == createInfo.maxLod && other.createInfo.borderColor == createInfo.borderColor
           && other.createInfo.unnormalizedCoordinates == createInfo.unnormalizedCoordinates;
  }
};

struct SamplerStateHashFn {
    std::size_t operator()(const SamplerState& s) const
    {
        return hashVal(s.createInfo.flags, s.createInfo.magFilter, s.createInfo.minFilter, s.createInfo.mipmapMode,
                                s.createInfo.addressModeU, s.createInfo.addressModeV, s.createInfo.addressModeW,
                                s.createInfo.mipLodBias, s.createInfo.anisotropyEnable, s.createInfo.maxAnisotropy,
                                s.createInfo.compareEnable, s.createInfo.compareOp, s.createInfo.minLod, s.createInfo.maxLod,
                                s.createInfo.borderColor, s.createInfo.unnormalizedCoordinates);
    }
};

class SamplerPool {
public:
    explicit SamplerPool(const vk::raii::Device &device);

    vk::raii::Sampler& acquireSampler(const vk::SamplerCreateInfo &createInfo = {
        .magFilter = vk::Filter::eLinear,
        .minFilter = vk::Filter::eLinear
    });

private:
    vk::raii::Device& m_device;

    std::unordered_map<SamplerState, vk::raii::Sampler, SamplerStateHashFn> m_samplerMap{};
};
}

#endif //VK_RAYTRACING_SAMPLERPOOL_H
