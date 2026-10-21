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

float3 sampleNormalMap(
    Texture2D normalTexture,
    SamplerState normalSampler,
    float2 uv,
    float3 N,
    float4 tangent,
    float normalScale)
{
    float3 normalSample = normalTexture.Sample(normalSampler, uv).xyz;
    float3 tangentNormal = normalSample * 2.0f - 1.0f;

    tangentNormal.xy *= normalScale;

    tangentNormal.z = sqrt(saturate(1.0f - dot(tangentNormal.xy, tangentNormal.xy)));

    float3 T = normalize(tangent.xyz);
    float3 B = tangent.w * cross(N, T);
    float3x3 TBN = float3x3(T, B, N);

    return normalize(mul(tangentNormal, TBN));
}

float3 fresnelSchlick(float HdotV, float3 F0)
{
    return F0 + (1.0f - F0) * pow(1.0f - HdotV, 5.0f);
}

// D
float distributionGGX(float NdotH, float roughness)
{
    float a = roughness * roughness;
    float a2 = a * a;

    float NdotH2 = NdotH * NdotH;

    float denom = NdotH2 * (a2 - 1.0f) + 1.0f;
    denom = PI * denom * denom;

    return a2 / max(denom, 1e-7f);
}

// smith exact Geometry function
float smithGeometryMasking(float NdotX, float roughness)
{
    float alpha = roughness * roughness;
    float alpha2 = alpha * alpha;

    return (2.0f * NdotX) / (NdotX + sqrt(alpha2 + (1.0f - alpha2) * NdotX * NdotX));
}

// Smith G with Schlick Approximation
float geometrySmith(float NdotV, float NdotL, float roughness) {
    float ggxV = smithGeometryMasking(NdotV, roughness);
    float ggxL = smithGeometryMasking(NdotL, roughness);

    return ggxV * ggxL;
}

float3 evaluateBRDF(float3 albedo, float metallic, float roughness, float3 N, float3 V, float3 L) {
    float3 H = normalize(V + L);

    float NdotV = saturate(dot(N, V));
    float NdotL = saturate(dot(N, L));
    float NdotH = saturate(dot(N, H));
    float HdotV = saturate(dot(H, V));

    if (NdotL == 0.0f || NdotV == 0.0f)
        return 0.0f;

    float3 F0 = lerp(float3(0.04f), albedo, metallic);

    float3 F = fresnelSchlick(HdotV, F0);
    float D = distributionGGX(NdotH, roughness);
    float G = geometrySmith(NdotV, NdotL, roughness);

    float3 kS = F;
    float3 kD = (1.0f - kS) * (1.0f - metallic);

    float3 specular = (D * G * kS) / (4.0f * NdotV);
    float3 diffuse = (kD * albedo * NdotL) / PI;

    return (diffuse + specular);
}

[shader("vertex")]
VertexOutput vertMain(shaderio::Vertex input) {
    shaderio::GltfRenderNode renderNode = pushConst.sceneInfo->gltfNodes[pushConst.renderNodeID];
    float3 pos = mul(renderNode.objectToWorld, float4(input.position, 1.0f)).xyz;

    VertexOutput output;
    output.worldPos = pos;
    output.pos = mul(pushConst.frameData->viewProjMatrix, float4(pos, 1.0));
    output.color = input.color;
    output.texCoords = float2(input.u, input.v);

    output.normal = mul(float3x3(transpose(renderNode.worldToObject)), input.normal);
    output.tangent.xyz = mul(renderNode.objectToWorld, float4(input.tangent.xyz, 0)).xyz;
    output.tangent.w = input.tangent.w;

    return output;
}

[shader("fragment")]
float4 fragMain(VertexOutput vertIn) : SV_Target {
    shaderio::GltfSceneInfo* scn = pushConst.sceneInfo;
    shaderio::GltfMaterial material = scn->gltfMaterials[pushConst.materialID];
    float4 baseColor = material.baseColor * vertIn.color;
    float metallic = saturate(material.metallic);
    float roughness = clamp(material.roughness, 0.04f, 1.0f);
    float opacity = 1.0f;

    if (material.baseColorTextureID >= 0) {
        shaderio::GltfTextureInfo texInfo = pushConst.sceneInfo->gltfTexturesInfos[material.baseColorTextureID];
        Texture2D texture = textures[NonUniformResourceIndex(texInfo.imageID)];
        SamplerState sampler = samplers[NonUniformResourceIndex(texInfo.samplerID)];
        float4 texel = texture.Sample(sampler, vertIn.texCoords);
        baseColor *= texel;
        opacity = baseColor.a * texel.a;
    }

    if (material.metallicRoughnessTextureID >= 0) {
        shaderio::GltfTextureInfo texInfo = pushConst.sceneInfo->gltfTexturesInfos[material.metallicRoughnessTextureID];
        Texture2D texture = textures[NonUniformResourceIndex(texInfo.imageID)];
        SamplerState sampler = samplers[NonUniformResourceIndex(texInfo.samplerID)];
        float4 texel = texture.Sample(sampler, vertIn.texCoords);
        roughness = texel.y;
        metallic = texel.z;
    }

    float3 N = normalize(vertIn.normal);

    if (material.normalTextureID >= 0) {
        shaderio::GltfTextureInfo texInfo = scn->gltfTexturesInfos[material.normalTextureID];

        Texture2D texture = textures[NonUniformResourceIndex(texInfo.imageID)];
        SamplerState sampler = samplers[NonUniformResourceIndex(texInfo.samplerID)];

        N = sampleNormalMap(
            texture,
            sampler,
            vertIn.texCoords,
            N,
            vertIn.tangent,
            material.normalTextureScale
        );
    }

    float3 V = normalize(pushConst.frameData->cameraPosition.xyz - vertIn.worldPos);

    float3 Lo = 0.0f;

    if(pushConst.frameData->useLight == 1) {
        float3 L = normalize(-pushConst.frameData->lightDir);
        float3 Li = pushConst.frameData->lightColor.rgb * pushConst.frameData->lightColor.a;
        Lo += evaluateBRDF(baseColor.rgb, metallic, roughness, N, V, L) * Li;
    }

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
        float3 Li = light.color * light.intensity * attenuation;
        // Lo = Le + integral(f * Li * NdotL)
        Lo += evaluateBRDF(baseColor.rgb, metallic, roughness, N, V, L) * Li;

    }

    //float3 ambient = albedo * float3(0.02f);
    float3 color = Lo;
    return float4(color, opacity);
}