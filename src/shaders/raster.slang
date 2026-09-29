#include "shaderio.h.slang"
#include "gltfio.h.slang"

#define PI 3.14159265358979323846

[[vk::push_constant]]
shaderio::RasterPushConstant pushConst;

[[vk::binding(shaderio::DescriptorBindingPoints::eTextures, 0)]]
Texture2D textures[];

[[vk::binding(shaderio::DescriptorBindingPoints::eSamplers, 0)]]
SamplerState samplers[];

struct VertexOutput {
    float4 pos : SV_Position;
    float3 worldPos;
    float3 normal;
    float4 tangent;
    float4 color;
    float2 texCoords;
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

float3 fresnelSchlick(float cosTheta, float3 F0) {
    return F0 + (1.0f - F0) * pow(1.0f - cosTheta, 5.0f);
}

// D
float distributionGGX(float NdotH, float roughness) {
    float a = roughness * roughness;
    float a2 = a * a;

    float NdotH2 = NdotH * NdotH;

    float denom = NdotH2 * (a2 - 1.0f) + 1.0f;
    denom = PI * denom * denom;

    return a2 / max(denom, 1e-7f);
}

// G1
float geometrySchlickGGX(float NdotX, float roughness) {
    float r = roughness + 1.0f;
    float k = (r * r) / 8.0f;

    return NdotX / (NdotX * (1.0f - k) + k);
}
// G2
float geometrySmith(float NdotV, float NdotL, float roughness) {
    float ggxV = geometrySchlickGGX(NdotV, roughness);
    float ggxL = geometrySchlickGGX(NdotL, roughness);

    return ggxV * ggxL;
}

float3 evaluateBRDF(float3 albedo, float metallic, float roughness, float3 N, float3 V, float3 L) {
    float3 H = normalize(V + L);

    float NdotV = saturate(dot(N, V));
    float NdotL = saturate(dot(N, L));
    float NdotH = saturate(dot(N, H));
    float HdotV = saturate(dot(H, V));

    if (NdotL <= 0.0f || NdotV <= 0.0f)
        return 0.0f;

    // Dielectrics use ~4% reflectance at normal incidence.
    // Metals use their base color as F0.
    float3 F0 = lerp(float3(0.04f, 0.04f, 0.04f), albedo, metallic);

    float3 F = fresnelSchlick(HdotV, F0);
    float D = distributionGGX(NdotH, roughness);
    float G = geometrySmith(NdotV, NdotL, roughness);

    float3 specular = (D * G * F) / max(4.0f * NdotV * NdotL, 1e-7f);

    float3 kS = F;
    float3 kD = (1.0f - kS) * (1.0f - metallic);

    float3 diffuse = kD * albedo / PI;

    return (diffuse + specular) * NdotL;
}

[shader("vertex")]
VertexOutput vertMain(shaderio::Vertex input) {
    shaderio::GltfRenderNode renderNode = pushConst.sceneInfo->gltfNodes[pushConst.renderNodeID];
    float3 pos = mul(renderNode.objectToWorld, float4(input.position, 1.0)).xyz;

    VertexOutput output;
    output.worldPos = pos;
    output.pos = mul(pushConst.frameData->viewProjMatrix, float4(pos, 1.0));
    output.color = input.color;
    output.texCoords = input.texCoords;

    float3x3 normalMatrix = transpose((float3x3)renderNode.worldToObject);
    output.normal = mul(normalMatrix, input.normal);
    output.tangent.xyz = mul(normalMatrix, input.tangent.xyz);
    output.tangent.w = input.tangent.w;

    return output;
}

[shader("fragment")]
float4 fragMain(VertexOutput vertIn) : SV_Target {
    shaderio::GltfSceneInfo* scn = pushConst.sceneInfo;
    shaderio::GltfMaterial material = scn->gltfMaterials[pushConst.materialID];
    float3 albedo = material.baseColor.xyz;
    float metallic = saturate(material.metallic);
    float roughness = clamp(material.roughness, 0.04f, 1.0f);
    float opacity = 1.0f;

    if (material.baseColorTextureID >= 0) {
        shaderio::GltfTextureInfo texInfo = pushConst.sceneInfo->gltfTexturesInfos[material.baseColorTextureID];
        Texture2D texture = textures[NonUniformResourceIndex(texInfo.imageID)];
        SamplerState sampler = samplers[NonUniformResourceIndex(texInfo.samplerID)];
        float4 texel = texture.Sample(sampler, vertIn.texCoords);
        opacity = texel.w;
        albedo *= texel.xyz;
    }

    float3 N = normalize(vertIn.normal);

    float3 V = normalize(pushConst.frameData->cameraPosition.xyz - vertIn.worldPos);

    float3 Lo = 0.0f;

    for (int i = 0; i < scn->numLights; ++i) {
        shaderio::GltfLight light = scn->gltfLights[i];

        float3 L;
        float attenuation = 1.0f;

        if (light.type == shaderio::LightType::eDirectional) {
            L = normalize(-light.direction);
        } else {
            // Point / spot light
            float3 toLight = light.position - vertIn.worldPos;

            float distance = length(toLight);

            L = toLight / max(distance, 1e-6f);

            attenuation = 1.0f / max(distance * distance, 1e-4f);

            // cutoff if exceeds range
            if (light.range > 0.0f) {
                float rangeFactor = saturate(1.0f - distance / light.range);

                attenuation *= rangeFactor * rangeFactor;
            }
        }

        // Spotlight cone
        if (light.type == shaderio::LightType::eSpot) {
            float3 lightToSurface = normalize(vertIn.worldPos - light.position);

            float cosTheta = dot(normalize(light.direction), lightToSurface);

            float cosInner = cos(light.innerAngle);
            float cosOuter = cos(light.outerAngle);

            float spotFactor = saturate((cosTheta - cosOuter) / max(cosInner - cosOuter, 1e-5f));

            attenuation *= spotFactor;
        }

        // GGX
        float3 radiance = light.color * light.intensity * attenuation;
        Lo += evaluateBRDF(albedo, metallic, roughness, N, V, L) * radiance;
    }
    float3 ambient = albedo * float3(0.02f);
    float3 color = Lo;
    float exposure = .005f;

    return float4(tonemapACES(exposure * color) + ambient, opacity);
}