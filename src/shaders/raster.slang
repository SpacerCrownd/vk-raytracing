#include "shaderio.h.slang"
#include "gltfio.h.slang"

static float3 positions[6] = float3[](
    float3(0.0, -0.5, 0.0),
    float3(0.5, 0.5, 0.0),
    float3(-0.5, 0.5, 0.0),
    float3(0.0, -0.25, 1.0),
    float3(0.25, 0.25, 1.0),
    float3(-0.25, 0.25, 1.0)
);

static float3 colors[6] = float3[](
    float3(1.0, 0.0, 0.0),
    float3(0.0, 1.0, 0.0),
    float3(0.0, 0.0, 1.0),
    float3(0.0, 0.0, 0.0),
    float3(0.0, 0.0, 0.0),
    float3(0.0, 0.0, 0.0),
);

[[vk::push_constant]]
ConstantBuffer<shaderio::RasterPushConstant> pushConstant;

[[vk::binding(shaderio::DescriptorBindingPoints::eTextures, 0)]]
Texture2D textures[];

[[vk::binding(shaderio::DescriptorBindingPoints::eSamplers, 0)]]
SamplerState samplers[];

struct VertexInput {
    float3 position : POSITION;
}

struct VertexOutput {
    float4 sv_position : SV_Position;
    float3 color;
};

[shader("vertex")]
VertexOutput vertMain(VertexInput input, uint vid : SV_VertexID) {
    VertexOutput output;
    output.sv_position = float4(positions[vid], 1.0);
    output.color = colors[vid];
    return output;
}

[shader("fragment")]
float4 fragMain(VertexOutput inVert) : SV_Target
{
    return float4(inVert.color, 1.0);
}