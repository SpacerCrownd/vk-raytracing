#include "shaderio.h.slang"
#include "gltfio.h.slang"
#include "gltf_vertex_access.slang"
#include "random.slang"

#define INFINITY 1e32F
#define RR_MIN_DEPTH 3

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

float3 sampleNormalMap(
    Texture2D normalTexture,
    SamplerState normalSampler,
    float2 uv,
    float3 N,
    float4 tangent,
    float normalScale)
{
    float3 normalSample = normalTexture.SampleLevel(normalSampler, uv, 0).xyz;
    float3 normal = normalSample * 2.0f - 1.0f;

    normal.xy *= normalScale;
    normal.z = sqrt(saturate(1.0f - dot(normal.xy, normal.xy)));

    float3 T = normalize(tangent.xyz);
    float3 B = tangent.w * cross(N, T);
    float3x3 TBN = float3x3(T, B, N);

    return normalize(mul(normal, TBN));
}

float3 fresnelSchlick(float HdotV, float3 F0)
{
    return F0 + (1.0f - F0) * pow(1.0f - HdotV, 5.0f);
}

float fresnelSchlick(float HdotV, float F0)
{
    return F0 + (1.0f - F0) * pow(1.0f - HdotV, 5.0f);
}

// D (NDF)
float distributionGGX(float NdotH, float roughness)
{
    float a2 = roughness * roughness;

    float NdotH2 = NdotH * NdotH;

    float denom = NdotH2 * (a2 - 1.0f) + 1.0f;
    denom = PI * denom * denom;

    return a2 / max(denom, 1e-7f);
}

// G1
float geometrySchlickGGX(float NdotX, float roughness)
{
    float r = roughness + 1.0f;
    float k = (r * r) / 8.0f;

    return NdotX / (NdotX * (1.0f - k) + k);
}

// G2
float geometrySmith(float NdotV, float NdotL, float roughness)
{
    float ggxV = geometrySchlickGGX(NdotV, roughness);
    float ggxL = geometrySchlickGGX(NdotL, roughness);

    return ggxV * ggxL;
}

// see https://jcgt.org/published/0007/04/01/paper.pdf
// Input V: view direction
// Input roughness
// Input xi: 2 uniform random numbers
// Output Ne: normal sampled with PDF D_Ve(Ne) = G1(Ve) * max(0, dot(Ve, Ne)) * D(Ne) / Ve.z
float3 sampleVNDF(float3 V, float roughness, float2 xi)
{
    float3 Vh = normalize(float3(V.x * roughness, V.y * roughness, V.z));

    float3 T1 = (V.z < 0.99999f) ? normalize(cross(V, float3(0.0f, 0.0f, 1.0f))) : float3(1.0f, 0.0f, 0.0f);

    float3 T2 = cross(T1, Vh);

    // Sample a point on the projected disk
    float r = sqrt(xi.x);
    float phi = 2.0f * PI * xi.y;

    float p1 = r * cos(phi);
    float p2 = r * sin(phi);

    float s = 0.5f * (1.0f + Vh.z);
    p2 = (1.0f - s) * sqrt(max(0.0f, 1.0f - p1 * p1)) + s * p2;

    float p3 = sqrt(max(0.0f, 1.0f - p1 * p1 - p2 * p2));
    // reproject onto hemisphere
    float3 Nh = p1 * T1 + p2 * T2 + p3 * Vh;

    float3 H = normalize(float3(Nh.x * roughness, Nh.y * roughness, max(0.0f, Nh.z)));

    return H;
}

// returns pdf
float evaluateBRDF(float3 albedo, float metallic, float roughness, float3 N, float3 V, float3 L, inout float3 brdfDiffuse, inout float3 brdfGlossy)
{
    // GGX BRDF Evaluation
    float3 H = normalize(L + V);
    float NdotV = saturate(dot(N, V));
    float NdotL = saturate(dot(N, L));
    float NdotH = saturate(dot(N, H));
    float VdotH = saturate(dot(V, H));

    if(NdotV == 0.0f || NdotL == 0.0f || VdotH == 0.0f || NdotH == 0.0f)
    {
        return 0.0f;
    }

    float3 F0 = lerp(float3(0.04f), albedo, metallic); // how reflective it is at normal incidence
    float3 fGlossy = fresnelSchlick(VdotH, F0);
    float fDielectric = fresnelSchlick(VdotH, 0.04f);
    float fDiffuse = (1 - fDielectric) * (1 - metallic); // at grazing angle decrease diffuse brdf (complementary to glossy Fresnel) -> VdotH closer to 0 -> f closer to 1

    float D = distributionGGX(NdotH, roughness);
    float G1V = geometrySchlickGGX(NdotV, roughness);
    float G1L = geometrySchlickGGX(NdotL, roughness);

    float diffusePdf  = (1 / PI) * NdotL;
    float specularPdf = (G1V * D) / max(4 * NdotV, 0);

    brdfDiffuse = albedo * fDiffuse * diffusePdf; // premultiply by NdotL
    brdfGlossy = (G1L * fGlossy) * specularPdf; // premultiply by NdotL

    return lerp(diffusePdf, specularPdf, lerp(fresnelSchlick( NdotV, 0.04F), 1.0F, metallic));
}

struct HitPayload
{
    float3 radiance;
    float3 scatterDir;
    bool   doScatter;
    float  hitT;

    uint   seed;
    float3 throughput = float3(1.0f);
    bool   nextEventValid = true;
};

[shader("raygeneration")]
void raygenMain()
{
    bool isFirstFrame = pushConst.totalSamples == 0;

    float2 launchID = (float2)DispatchRaysIndex().xy;        // Current pixel (x,y)
    float2 launchSize = (float2)DispatchRaysDimensions().xy; // Image size (width,height)

    // if I have enough samples skip to keep framerate for ui navigation
    if(pushConst.totalSamples >= pushConst.maxSamples) {
        return;
    }

    uint rng = uint(uint(launchID.x) * uint(1973) + uint(launchID.y) * uint(9277) + uint(pushConst.totalSamples) * uint(26699)) | uint(1);

    float3 color = float3(0.0f);

    // samples
    for (uint i = 0; i < pushConst.numSamples; i++)
    {
        HitPayload payload = {};
        payload.seed = rng;
        payload.throughput = float3(1.0f);
        payload.nextEventValid = true;

        // Convert pixel coordinates
        float2 jitter = float2(randomFloat01(rng), randomFloat01(rng)); // for antialiasing
        float2 clipCoords = (launchID + jitter) / launchSize * 2.0 - 1.0;
        float4 viewCoords = mul(pushConst.frameData->projInvMatrix, float4(clipCoords, 1.0, 1.0)); // Transform from NDC to view space using inverse projection matrix

        RayDesc ray;
        ray.Origin = mul(pushConst.frameData->viewInvMatrix, float4(0.0, 0.0, 0.0, 1.0)).xyz;                // Transform camera origin (0,0,0) from view space to world space
        ray.Direction = mul(pushConst.frameData->viewInvMatrix, float4(normalize(viewCoords.xyz), 0.0)).xyz; // Transform ray direction from view space to world space
        ray.TMin = 0.001;
        ray.TMax = INFINITY;

        float3 sampleColor = float3(0.0f);
        for (uint j = 0; j <= pushConst.maxDepth; j++)
        {
            // Parameters: AS, flags, instance mask, sbt offset, sbt stride, miss offset, ray, payload
            TraceRay(tlas, 0, 0xFF, 0, 0, 0, ray, payload);
            sampleColor += payload.radiance;

            if (payload.hitT == INFINITY || !payload.doScatter)
            {
                break;
            }

            ray.Origin += payload.hitT * ray.Direction;
            ray.Direction = payload.scatterDir;

            // Russian Roulette
            if (j >= RR_MIN_DEPTH) {
                float pcont = min(max(payload.throughput.x, max(payload.throughput.y, payload.throughput.z)) + 0.001f, 0.95f);
                if(randomFloat01(payload.seed) >= pcont){
                    break;
                }
                payload.throughput /= pcont; // compensate for discarded paths to be unbiased
            }
        }
        color += sampleColor;
    }
    color /= pushConst.numSamples;

    // Removing fireflies
    float lum = dot(color.rgb, float3(1.0F / 3.0F));
    if(lum > 10.0f)
    {
        color *= 10.0f / lum;
    }

    // uniform weighted temporal accumulation
    if (isFirstFrame)
    {
        //outImage[int2(launchID)] = float4(1.0f, 0.0f, 1.0f, 1.0f);
        outImage[int2(launchID)] = float4(color, 1.0f);
    }
    else
    {
        float newTotal = pushConst.totalSamples + pushConst.numSamples;
        float4 oldColor = outImage[int2(launchID)];
        color = (oldColor.rgb * pushConst.totalSamples + color * pushConst.numSamples) / newTotal;

        outImage[int2(launchID)] = float4(color, 1.0f);
    }
}

[shader("closesthit")]
void closesthitMain(inout HitPayload payload, in BuiltInTriangleIntersectionAttributes attr)
{
    payload.nextEventValid = false;
    payload.doScatter = true;

    // hit data
    float3 barycentrics = float3(1 - attr.barycentrics.x - attr.barycentrics.y, attr.barycentrics.x, attr.barycentrics.y);
    float hitT = RayTCurrent();
    uint renderNodeID = InstanceIndex();
    uint renderPrimID = InstanceID();
    uint triID = PrimitiveIndex();

    shaderio::GltfSceneInfo sceneInfo = *pushConst.sceneInfo;
    shaderio::GltfRenderPrimitive primitive = sceneInfo.gltfPrimitives[renderPrimID];
    shaderio::GltfRenderNode node = sceneInfo.gltfNodes[renderNodeID];
    shaderio::GltfMaterial material = sceneInfo.gltfMaterials[node.materialID];
    shaderio::GltfTextureInfo* textureInfos = sceneInfo.gltfTexturesInfos;

    uint3 indices = primitive.indices[triID];

    float3 pos = getInterpolatedVertexPosition(primitive, indices, barycentrics);
    pos = mul(node.objectToWorld, float4(pos, 1.0f)).xyz;

    float3 normal = getInterpolatedVertexNormal(primitive, indices, barycentrics);
    normal = mul(transpose(node.worldToObject), float4(normal, .0f)).xyz;

    float4 tangent = getInterpolatedVertexTangent(primitive, indices, barycentrics);
    tangent = mul(node.objectToWorld, tangent);

    float2 texCoords = getInterpolatedVertexTexCoords(primitive, indices, barycentrics);

    float4 baseColor = material.baseColor;
    baseColor *= getInterpolatedVertexColor(primitive, indices, barycentrics);
    if (material.baseColorTextureID >= 0)
    {
        shaderio::GltfTextureInfo texInfo = textureInfos[material.baseColorTextureID];
        Texture2D texture = textures[NonUniformResourceIndex(texInfo.imageID)];
        SamplerState sampler = samplers[NonUniformResourceIndex(texInfo.samplerID)];
        baseColor *= texture.SampleLevel(sampler, texCoords, 0);
    }

    float3 albedo = baseColor.rgb;
    float roughness = material.roughness;
    float metallic = material.metallic;
    if (material.metallicRoughnessTextureID >= 0)
    {
        shaderio::GltfTextureInfo texInfo = pushConst.sceneInfo->gltfTexturesInfos[material.metallicRoughnessTextureID];
        Texture2D texture = textures[NonUniformResourceIndex(texInfo.imageID)];
        SamplerState sampler = samplers[NonUniformResourceIndex(texInfo.samplerID)];
        float4 texel = texture.SampleLevel(sampler, texCoords, 0);
        roughness *= texel.y;
        metallic *= texel.z;
    }

    roughness = clamp(roughness, 0.05f, 1.0f);
    metallic = saturate(metallic);

    float3 N = normalize(normal);
    if (material.normalTextureID >= 0)
    {
        shaderio::GltfTextureInfo texInfo = textureInfos[material.normalTextureID];
        Texture2D texture = textures[NonUniformResourceIndex(texInfo.imageID)];
        SamplerState sampler = samplers[NonUniformResourceIndex(texInfo.samplerID)];

        N = sampleNormalMap(
            texture,
            sampler,
            texCoords,
            N,
            tangent,
            material.normalTextureScale);
    }

    float3 V = normalize(pushConst.frameData->cameraPosition.xyz - pos);
    float3 Lo = float3(0.0f);

    // light contribution
    int randomLight = int(randomFloat01(payload.seed) * sceneInfo.numLights);
    shaderio::GltfLight light = sceneInfo.gltfLights[randomLight];
    float lightSelectionPdf = max(1.0f / sceneInfo.numLights, 1e-10F);

    float3 L;
    float attenuation = 1.0f;
    float distance = INFINITY;
    if (light.type == shaderio::LightType::eDirectional)
    {
        L = normalize(-light.direction);
    }
    else
    {
        // Point / spot light
        float3 toLight = light.position - pos;

        distance = length(toLight);

        L = toLight / max(distance, 1e-6f);

        attenuation = 1.0f / max(distance * distance, 1e-4f);

        // cutoff if exceeds range
        if (light.range > 0.0f)
        {
            float rangeFactor = saturate(1.0f - distance / light.range);

            attenuation *= rangeFactor * rangeFactor;
        }
    }

    // Spotlight cone
    if (light.type == shaderio::LightType::eSpot)
    {
        float3 lightToSurface = normalize(pos - light.position);

        float cosTheta = dot(normalize(light.direction), lightToSurface);

        float cosInner = cos(light.innerAngle);
        float cosOuter = cos(light.outerAngle);

        float spotFactor = saturate((cosTheta - cosOuter) / max(cosInner - cosOuter, 1e-5f));

        attenuation *= spotFactor;
    }

    payload.nextEventValid = dot(L, N) > 0.0f && attenuation > 0.0f;

    if(payload.nextEventValid)
    {
        // trace shadow ray to check visibility
        RayDesc shadowRay;
        shadowRay.Origin = pos;
        shadowRay.Direction = L;
        shadowRay.TMin = 0.0f;
        shadowRay.TMax = distance;

        ShadowPayload shadowPayload;
        shadowPayload.seed = payload.seed;

        TraceRay(tlas, 0, 0xFF, 1, 0, 1, shadowRay, shadowPayload);

        if(!shadowPayload.blocked)
        {
            float3 Li = light.color * light.intensity * attenuation;

            float3 brdfGlossy;
            float3 brdfDiffuse;

            float pdf = evaluateBRDF(albedo, metallic, roughness, N, V, L, brdfDiffuse, brdfGlossy);
            if(pdf > 0.0f)
            {
                Lo += (brdfGlossy + brdfDiffuse) * Li * payload.throughput / lightSelectionPdf;
            }
        }
    }

    // GGX VNDF scatter direction sampling
    // sample new direction
    float NdotV = saturate(dot(N, V));
    float rand = randomFloat01(payload.seed);
    float glossyProb = lerp(fresnelSchlick( NdotV, 0.04F), 1.0F, metallic);
    // depending on probability given by glossyness choose between vndf ggx sampling or cosine weighted sampling for diffuse
    if(rand <= glossyProb)
    {
        // transform V into local coordinates
        float r1 = randomFloat01(payload.seed);
        float r2 = randomFloat01(payload.seed);
        float3 T = normalize(tangent.xyz);
        float3 B = normalize(cross(N, T) * tangent.w);
        float3 localV = float3(dot(T, V), dot(B, V), NdotV); // transform view vector into tangent space
        float3 H = sampleVNDF(localV, roughness, float2(r1, r2));
        // transform back into world space
        H = T * H.x + B * H.y + N * H.z;

        if(dot(H, V) <= 0.0f)
        {
            payload.doScatter = false;
        }

        payload.scatterDir = reflect(-V, H);
    }
    else
    {
        float3 T = normalize(tangent.xyz);
        float3 B = normalize(cross(N, T) * tangent.w);
        float3 localDir = cosineSampleHemisphere(randomFloat01(payload.seed), randomFloat01(payload.seed));
        payload.scatterDir = T * localDir.x + B * localDir.y + N * localDir.z;
    }

    if(dot(payload.scatterDir, N) > 0.0f)
    {
        float3 brdfGlossy;
        float3 brdfDiffuse;
        float pdf = evaluateBRDF(albedo,metallic, roughness, N, V, payload.scatterDir, brdfGlossy, brdfDiffuse);
        float3 brdfTotal = brdfDiffuse + brdfGlossy;

        if(pdf <= 0.00001F || any(isnan(brdfTotal)))
        {
            payload.doScatter = false;
        }
        else
        {
            payload.throughput *= brdfTotal / max(pdf, 1e-6f);
        }
    } else {
        payload.doScatter = false;
    }

    // update radiance
    payload.radiance = Lo;
    payload.hitT = hitT;
}

float getOpacity(shaderio::GltfRenderNode renderNode, shaderio::GltfRenderPrimitive renderPrim, int triangleID, float3 barycentrics)
{
    uint matIndex  = renderNode.materialID;
    shaderio::GltfMaterial* materials = pushConst.sceneInfo->gltfMaterials;
    shaderio::GltfMaterial mat = materials[matIndex];
    shaderio::GltfTextureInfo* texInfos  = pushConst.sceneInfo->gltfTexturesInfos;

    if(mat.alphaMode == shaderio::AlphaMode::eAlphaModeOpaque)
        return 1.0;

    uint3 triangleIndex = renderPrim.indices[triangleID];

    float alpha = 1;

    alpha = mat.baseColor.a;
    if(mat.baseColorTextureID >= 0)
    {
        shaderio::GltfTextureInfo texInfo = texInfos[mat.baseColorTextureID];
        float2 uv = getInterpolatedVertexTexCoords(renderPrim, triangleIndex, barycentrics);

        alpha *= textures[texInfo.imageID].SampleLevel(samplers[texInfo.samplerID], uv, 0).a;
    }

    alpha *= getInterpolatedVertexColor(renderPrim, triangleIndex, barycentrics).a;

    if(mat.alphaMode == shaderio::AlphaMode::eAlphaModeMask)
    {
        return alpha >= mat.alphaCutoff ? 1.0 : 0.0;
    }

    return alpha;
}

[shader("anyhit")]
void anyhitMain(inout HitPayload payload, in BuiltInTriangleIntersectionAttributes attr)
{
    float3 barycentrics = float3(1 - attr.barycentrics.x - attr.barycentrics.y, attr.barycentrics.x, attr.barycentrics.y);

    uint instanceID   = InstanceIndex();
    uint renderPrimID = InstanceID();
    uint triangleID   = PrimitiveIndex();

    shaderio::GltfRenderNode renderNode = pushConst.sceneInfo->gltfNodes[instanceID];
    shaderio::GltfRenderPrimitive renderPrim = pushConst.sceneInfo->gltfPrimitives[renderPrimID];

    // stochastic opacity
    float opacity = getOpacity(renderNode, renderPrim, triangleID, barycentrics);
    if(randomFloat01(payload.seed) >= opacity)
    {
        IgnoreHit();
    }
}

[shader("miss")]
void missMain(inout HitPayload payload)
{
    // TODO: sample env cubemap
    payload.hitT = INFINITY;
    payload.radiance = float3(0.5, 0.5, 0.5) * payload.throughput;
}

struct ShadowPayload {
    uint seed;
    bool blocked = false;
}

[shader("closesthit")]
void closesthitShadow(inout ShadowPayload payload, in BuiltInTriangleIntersectionAttributes attr)
{
    payload.blocked = true;
}

[shader("anyhit")]
void anyhitShadow(inout ShadowPayload payload, in BuiltInTriangleIntersectionAttributes attr)
{
    float3 barycentrics = float3(1 - attr.barycentrics.x - attr.barycentrics.y, attr.barycentrics.x, attr.barycentrics.y);

    uint renderNodeID   = InstanceIndex();
    uint renderPrimID = InstanceID();
    uint primitiveID  = PrimitiveIndex();

    shaderio::GltfRenderNode renderNode = pushConst.sceneInfo->gltfNodes[renderNodeID];
    shaderio::GltfRenderPrimitive renderPrim = pushConst.sceneInfo->gltfPrimitives[renderPrimID];

    float opacity = getOpacity(renderNode, renderPrim, primitiveID, barycentrics);
    float r = randomFloat01(payload.seed);

    if (r < opacity)
    {
        AcceptHitAndEndSearch();
    }

    IgnoreHit();
}

[shader("miss")]
void missShadow(inout ShadowPayload payload)
{
    payload.blocked = false;
}