#include "shaderio.h.slang"
#include "gltfio.h.slang"

[[vk::push_constant]]
shaderio::RtPushConstant pushConst;

[[vk::binding(shaderio::DescriptorBindingPoints::eTextures, 0)]]
Texture2D textures[];

[[vk::binding(shaderio::DescriptorBindingPoints::eSamplers, 0)]]
SamplerState samplers[];

[[vk::binding(shaderio::DescriptorBindingPoints::eTlas, 1)]]
RaytracingAccelerationStructure tlas;

[[vk::binding(shaderio::DescriptorBindingPoints::eOutImage, 1)]]
RWTexture2D outImage;

struct HitPayload {
    float3 color;
    float weight = 1;
    int depth = 0;
};

[shader("raygeneration")]
void raygenMain() {
    // Get the current pixel coordinates and image dimensions
    float2 launchID = (float2)DispatchRaysIndex().xy;     // Current pixel (x,y)
    float2 launchSize = (float2)DispatchRaysDimensions().xy; // Image size (width,height)

    // Set ray tracing flags (0 = no special flags)
    const uint rayFlags = 0;

    // Convert pixel coordinates to normalized device coordinates (NDC)
    // Range: [-1,1] for both x and y
    const float2 clipCoords = launchID / launchSize * 2.0 - 1.0;

    // Transform from NDC to view space using inverse projection matrix
    const float4 viewCoords = mul(pushConst.frameData->projInvMatrix, float4(clipCoords, 1.0, 1.0));

    // Create the primary ray
    RayDesc ray;
    // Transform camera origin (0,0,0) from view space to world space
    ray.Origin = mul(pushConst.frameData->viewInvMatrix, float4(0.0, 0.0, 0.0, 1.0)).xyz;
    // Transform ray direction from view space to world space
    ray.Direction = mul(pushConst.frameData->viewInvMatrix, float4(normalize(viewCoords.xyz), 0.0)).xyz;
    ray.TMin = 0.001;    // Minimum distance to avoid self-intersection
    ray.TMax = 1e32F; // Maximum distance (infinite for primary rays)

    // Initialize ray payload with default values
    HitPayload payload = {};

    // Cast the ray into the scene using the acceleration structure
    // Parameters: AS, flags, instance mask, sbt offset, sbt stride, miss offset, ray, payload
    TraceRay(tlas, rayFlags, 0xff, 0, 0, 0, ray, payload);

    // Get the final color from the ray tracing result
    float3 color = payload.color;

    // Write the result to the output image
    outImage[int2(launchID)] = float4(color, 1.0);
}

[shader("miss")]
void missMain(inout HitPayload payload) {
    payload.color = float3(0.5, 0.5, 0.5);
}

[shader("closesthit")]
void closesthitMain(inout HitPayload payload) {
    // For a first test, don't even worry about your glTF vertex data.
    // // If we hit geometry, return a constant color.
    payload.color = float3(1.0, 0.0, 0.0);
}