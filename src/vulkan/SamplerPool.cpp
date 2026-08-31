#include "SamplerPool.h"

ptvk::SamplerPool::SamplerPool(const vk::raii::Device &device) : m_device(device) {}

vk::raii::Sampler& ptvk::SamplerPool::acquireSampler(const vk::SamplerCreateInfo &createInfo) {
    SamplerState samplerState = {createInfo};

    if(auto it = m_samplerMap.find(samplerState); it != m_samplerMap.end())
    {
        return it->second;
    }

    auto [it, inserted] = m_samplerMap.try_emplace(samplerState, m_device.createSampler(createInfo));
    return it->second;
}

