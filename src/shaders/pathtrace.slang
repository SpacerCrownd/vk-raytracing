#include "shaderio.h.slang"
#include "gltfio.h.slang"

[[vk::push_constant]] ConstantBuffer<shaderio::RtPushConstant> pushConstant;
[[vk::binding(shaderio::DescriptorBindingPoints::eTlas, 0)]] RaytracingAccelerationStructure tlas;
[[vk::binding(shaderio::DescriptorBindingPoints::eTextures, 1)]] Texture2D textures[];
[[vk::binding(shaderio::DescriptorBindingPoints::eEnvCube, 1)]] SamplerState samplers[];