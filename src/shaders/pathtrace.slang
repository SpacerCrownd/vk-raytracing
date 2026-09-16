#include "shaderio.h.slang"
#include "gltfio.h.slang"

[[vk::push_constant]]
ConstantBuffer<shaderio::RtPushConstant> pushConstant;

[[vk::binding(shaderio::DescriptorBindingPoints::eTextures, 0)]]
Texture2D textures[];

[[vk::binding(shaderio::DescriptorBindingPoints::eSamplers, 0)]]
SamplerState samplers[];

[[vk::binding(shaderio::DescriptorBindingPoints::eTlas, 1)]]
RaytracingAccelerationStructure tlas;

[[vk::binding(shaderio::DescriptorBindingPoints::eOutImage, 1)]]
RWTexture2D<float4> outputImage;

struct RayPayload { float3 color; };

[shader("raygeneration")]
void raygenMain() {
    uint2 pixel = DispatchRaysIndex().xy;
    uint2 size = DispatchRaysDimensions().xy;
    float2 uv = (float2(pixel) + 0.5) / float2(size);
    float2 ndc = uv * 2.0 - 1.0; // Simple camera ray.
    // // Replace these with your actual camera matrices later.
    float3 origin = pushConstant.frameData.cameraPosition.xyz;
    float3 direction = normalize( float3(ndc.x, -ndc.y, 1.0) );
    RayDesc ray;
    ray.Origin = origin;
    ray.Direction = direction;
    ray.TMin = 0.001;
    ray.TMax = 10000.0;
    RayPayload payload;
    payload.color = float3(0.0);
    TraceRay(tlas,
        RAY_FLAG_NONE,
        0xFF,
        0, // hit group
        0, // miss shader
        0, // miss shader index
        ray,
        payload );

    outputImage[pixel] = float4(payload.color, 1.0);
}

[shader("miss")]
void missMain(inout RayPayload payload) {
    payload.color = float3(0.05, 0.05, 0.08);
}

[shader("closesthit")]
void closestHitMain(inout RayPayload payload) {
    // For a first test, don't even worry about your glTF vertex data.
    // // If we hit geometry, return a constant color.
    payload.color = float3(1.0, 0.2, 0.1);
}