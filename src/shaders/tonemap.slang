#include "shaderio.h.slang"

[[vk::push_constant]]
shaderio::RasterPushConstant pushConst;

[[vk::binding(shaderio::DescriptorBindingPoints::eOutImage, 0)]]
Texture2D hdrImage;

struct VertexOutput {
    float4 pos : SV_Position;
};

// ACES film tonemapper (Narkowicz)
float3 tonemapACES(float3 x) {
    float a = 2.51f;
    float b = 0.03f;
    float c = 2.43f;
    float d = 0.59f;
    float e = 0.14f;

    return saturate((x * (a * x + b)) / (x * (c * x + d) + e));
}

[shader("vertex")]
VertexOutput vertMain(uint vertexID : SV_VertexID)
{
    // fullscreen triangle in clip space that covers all screen
    float2 positions[3] = {
        float2(-1.0, -1.0),
        float2(-1.0,  3.0),
        float2( 3.0, -1.0),
    };

    VertexOutput output;
    output.pos = float4(positions[vertexID], 0.0, 1.0);

    return output;
}

[shader("fragment")]
float4 fragMain(VertexOutput vertIn) : SV_Target {
    int2 pixel = int2(vertIn.pos.xy);
    float4 hdr = hdrImage.Load(int3(pixel, 0));
    float exposure = exp2(pushConst.frameData->exposureEV);
    //return hdr;
    return float4(tonemapACES(hdr.rgb * exposure), 1.0);
}