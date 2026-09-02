#include "SamplerPool.h"
namespace ptvk {

SamplerPool::SamplerPool(const vk::raii::Device &device) : m_device(device) {}

vk::Sampler SamplerPool::acquireSampler(const vk::SamplerCreateInfo &createInfo) {
    SamplerState samplerState = {createInfo};

    if(auto it = m_samplerMap.find(samplerState); it != m_samplerMap.end())
    {
        return it->second;
    }
    auto [it, inserted] = m_samplerMap.try_emplace(samplerState, m_device.createSampler(createInfo));
    return *it->second; // give back raw vk handle
}

void SamplerPool::releaseSampler(vk::Sampler sampler) {
    if (sampler == VK_NULL_HANDLE) {
        return;
    }

    for (auto it = m_samplerMap.begin(); it != m_samplerMap.end(); ++it) {
        if (*it->second == sampler) {
            m_samplerMap.erase(it);
            return;
        }
    }
}
}
