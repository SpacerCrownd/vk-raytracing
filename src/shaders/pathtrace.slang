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
RWTexture2D outImages[];

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

    float3 T = normalize(tangent.xyz - N * dot(N, tangent.xyz));
    float3 B = normalize(cross(N, T)) * tangent.w;
    float3x3 TBN = float3x3(T, B, N);

    return normalize(mul(normal, TBN));
}

float fresnelSchlick(float F0, float cosTheta)
{
    return F0 + (1.0 - F0) * pow(1.0F - cosTheta, 5.0);
}

float3 fresnelSchlick(float3 F0, float cosTheta) 
{
    return F0 + (1.0 - F0) * pow(1.0F - cosTheta, 5.0);
}

// D (NDF) GGX/Trowbridge-Reitz Distribution
float distributionGGX(float NdotH, float roughness)
{
    float a = roughness * roughness; // gltf roughness parameter needs to be squared
    float a2 = a * a;
    float NdotH2 = NdotH * NdotH;

    float denom = (NdotH2 * (a2 - 1.0) + 1.0);
    denom = PI * denom * denom;

    return a2 / denom;
}

// smith exact Geometry function
float smithGeometryMasking(float NdotX, float roughness)
{
    float alpha = roughness * roughness;
    float alpha2 = alpha * alpha;

    return (2.0f * NdotX) / (NdotX + sqrt(alpha2 + (1.0f - alpha2) * NdotX * NdotX));
}

// Geometry function Smith's method with Schlick-GGX approximation
float geometrySmith(float NdotV, float NdotL, float roughness)
{
    float ggxV = smithGeometryMasking(NdotV, roughness);
    float ggxL = smithGeometryMasking(NdotL, roughness);

    return ggxV * ggxL;
}

// see https://jcgt.org/published/0007/04/01/paper.pdf
// Input V: view direction
// Input roughness
// Input xi: 2 uniform random numbers
// Output Ne: normal sampled with PDF D_Ve(Ne) = G1(Ve) * max(0, dot(Ve, Ne)) * D(Ne) / Ve.z
float3 sampleVNDF(float3 V, float roughness, float2 xi)
{
    if (V.z <= 0.0f)
    {
        return float3(0.0f, 0.0f, 1.0f); // Fallback to normal
    }
    float a = roughness * roughness;
    float3 Vh = normalize(float3(V.x * a, V.y * a, V.z));

    float3 T1 = (Vh.z < 0.99999f) ? normalize(cross(Vh, float3(0.0f, 0.0f, 1.0f))) : float3(1.0f, 0.0f, 0.0f);

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

    float3 H = normalize(float3(Nh.x * a, Nh.y * a, max(0.0f, Nh.z)));

    return H;
}

// Returns the probability that bsdfSampleSimple samples a glossy lobe.
float brdfGlossyProbability(float NdotV, float metallic)
{
    // mix dielectric fresnel and metal fresnel by metallic factor as probability
    return lerp(fresnelSchlick(0.04F, NdotV), 1.0F, metallic);
}

void evaluateBRDF(float3 albedo, float metallic, float roughness, float3 N, float3 V, float3 L, inout float3 brdfDiffuse, inout float3 brdfGlossy, inout float pdf)
{
    float3 H = normalize(L + V);
    float NdotV = saturate(dot(N, V));
    float NdotL = saturate(dot(N, L));
    float NdotH = saturate(dot(N, H));
    float HdotV = saturate(dot(V, H));

    if(NdotV == 0.0f || NdotL == 0.0f || HdotV == 0.0f || NdotH == 0.0f)
    {
        brdfDiffuse = brdfGlossy = 0;
        pdf = 0;
        return;
    }

    float D = distributionGGX(NdotH, roughness);
    float G = geometrySmith(NdotV, NdotL, roughness);
    float3 F0 = lerp(float3(0.04f), albedo, metallic); // how reflective it is at normal incidence
    float3 fGlossy = fresnelSchlick(F0, HdotV);
    float3 fDiffuse = (float3(1.0) - fGlossy) * (1.0 - metallic); // energy conservation, diffuse is the rest of the energy

    float G1V = smithGeometryMasking(NdotV, roughness);

    brdfGlossy = (D * G * fGlossy) / (4 * NdotV); // premul by NdotL
    brdfDiffuse = (albedo * fDiffuse / PI) * NdotL; // premul by NdotL
    float pdfGlossy = (G1V * D) / (4 * NdotV);
    float pdfDiffuse = (NdotL/PI);
    pdf = lerp(pdfDiffuse, pdfGlossy, brdfGlossyProbability(NdotV, metallic));
}

struct HitPayload
{
    float3 radiance;
    float3 albedo;
    float3 normal;
    float3 scatterDir;
    bool   doScatter;
    float  hitT;


    uint   seed;
    float3 throughput;
};

[shader("raygeneration")]
void raygenMain()
{
    bool isFirstFrame = pushConst.totalSamples == 0;

    float2 pixelPos = (float2)DispatchRaysIndex().xy;        // Current pixel (x,y)
    float2 imageSize = (float2)DispatchRaysDimensions().xy;

    // if I have enough samples skip to keep framerate for ui navigation
    if(pushConst.maxSamples > 0 && pushConst.totalSamples >= pushConst.maxSamples) {
        return;
    }

    uint rng = uint(uint(pixelPos.x) * uint(1973) + uint(pixelPos.y) * uint(9277) + uint(pushConst.totalSamples) * uint(26699)) | uint(1);

    HitPayload payload = {};
    payload.seed = rng;

    float3 color = float3(0);
    float3 albedo = float3(0);
    float3 normal = float3(0);
    for (uint i = 0; i < pushConst.numSamples; i++)
    {
        payload.throughput = float3(1.0f);
        payload.doScatter = true;

        // Convert pixel coordinates
        float2 jitter = float2(randomFloat01(rng), randomFloat01(rng)); // for antialiasing
        float2 clipCoords = (pixelPos + jitter) / imageSize * 2.0 - 1.0;
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
            TraceRay(tlas, RAY_FLAG_CULL_BACK_FACING_TRIANGLES, 0xFF, 0, 0, 0, ray, payload);
            sampleColor += payload.radiance;

            // if is first frame and 0th bounce
            if(isFirstFrame && j == 0){
                albedo = payload.albedo;
                normal = payload.normal;
            }

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
        //outImage[int2(pixelPos)] = float4(1.0f, 0.0f, 1.0f, 1.0f);
        outImages[int(shaderio::OutputImage::eResult)][int2(pixelPos)] = float4(color, 1.0f);
        outImages[int(shaderio::OutputImage::eAccumulation)][int2(pixelPos)] = float4(color, 1.0f);
        outImages[int(shaderio::OutputImage::eAlbedo)][int2(pixelPos)] = float4(albedo, 1.0f);
        outImages[int(shaderio::OutputImage::eNormal)][int2(pixelPos)] = float4(normal, 1.0f);
    }
    else
    {
        float newTotal = pushConst.totalSamples + pushConst.numSamples;
        float4 oldColor = outImages[int(shaderio::OutputImage::eAccumulation)][int2(pixelPos)];
        float4 oldAlbedo = outImages[int(shaderio::OutputImage::eAlbedo)][int2(pixelPos)];
        float4 oldNormal = outImages[int(shaderio::OutputImage::eNormal)][int2(pixelPos)];
        color = (oldColor.rgb * pushConst.totalSamples + color * pushConst.numSamples) / newTotal;
        float3 albedo = (oldAlbedo.rgb * pushConst.totalSamples + albedo * pushConst.numSamples) / newTotal;
        float3 normal = (oldNormal.rgb * pushConst.totalSamples + normal * pushConst.numSamples) / newTotal;

        outImages[int(shaderio::OutputImage::eResult)][int2(pixelPos)] = float4(color, 1.0f);
        outImages[int(shaderio::OutputImage::eAccumulation)][int2(pixelPos)] = float4(color, 1.0f);
        outImages[int(shaderio::OutputImage::eAlbedo)][int2(pixelPos)] = float4(albedo, 1.0f);
        outImages[int(shaderio::OutputImage::eNormal)][int2(pixelPos)] = float4(normal, 1.0f);
    }
}

[shader("closesthit")]
void closesthitMain(inout HitPayload payload, in BuiltInTriangleIntersectionAttributes attr)
{
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

    float4 tangentOS = getInterpolatedVertexTangent(primitive, indices, barycentrics);
    float4 tangent = float4(mul((float3x3)node.objectToWorld, tangentOS.xyz), tangentOS.w);

    float3 normal = normalize(getInterpolatedVertexNormal(primitive, indices, barycentrics));
    normal = normalize(mul(float3x3(transpose(node.worldToObject)), normal));

    float2 texCoords = getInterpolatedVertexTexCoords(primitive, indices, barycentrics);

    float4 baseColor = material.baseColor;

    baseColor *= getInterpolatedVertexColor(primitive, indices, barycentrics);

    if (material.baseColorTextureID >= 0)
    {
        shaderio::GltfTextureInfo texInfo = textureInfos[material.baseColorTextureID];
        Texture2D texture = textures[NonUniformResourceIndex(texInfo.imageID)];
        SamplerState sampler = samplers[NonUniformResourceIndex(texInfo.samplerID)];
        float4 texel = texture.SampleLevel(sampler, texCoords, 0);
        baseColor *= texel;
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

    float3 N = normal;
    float3 V = -WorldRayDirection();
    float3 Lo = float3(0.0f); // outgoing radiance (toward V)

    if (material.normalTextureID >= 0)
    {
        shaderio::GltfTextureInfo texInfo = textureInfos[material.normalTextureID];
        Texture2D texture = textures[NonUniformResourceIndex(texInfo.imageID)];
        SamplerState sampler = samplers[NonUniformResourceIndex(texInfo.samplerID)];
        N = sampleNormalMap(texture, sampler, texCoords, N, tangent, material.normalTextureScale);
    }

    payload.albedo = albedo;
    payload.normal = N;

    // check if has tangent
    if (length(tangent) == 0) {
        float3 up = (abs(N.z) < 0.999f) ? float3(0.0f, 0.0f, 1.0f) : float3(1.0f, 0.0f, 0.0f);
        tangent = float4(normalize(cross(up, N)), 1.0f);
    }

    // light contribution
    int numLights = pushConst.frameData->useLight == 1 ? sceneInfo.numLights + 1 : sceneInfo.numLights;
    int randomLight = int(randomFloat01(payload.seed) * numLights);

    shaderio::GltfLight light;
    if(numLights > 0)
    {
        if(randomLight < sceneInfo.numLights)
        {
            // gltf scene light selected
            light = sceneInfo.gltfLights[randomLight];
        }
        else
        {
            // global directional light selected
            light.color = pushConst.frameData->lightColor.rgb;
            light.intensity = pushConst.frameData->lightColor.a;
            light.direction = pushConst.frameData->lightDir;
            light.type = shaderio::LightType::eDirectional;
        }

        float lightSelectionPdf = max(1.0f / numLights, 1e-10F);

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
            distance = sqrt(dot(toLight, toLight));
            L = toLight / distance;

            if (light.range > 0.0)
            {
                float distanceOverRange = distance / light.range;
                attenuation = saturate(1.0f - (distanceOverRange * distanceOverRange) * (distanceOverRange * distanceOverRange));
            }

            attenuation /= distance * distance;
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

        bool nextEventValid = saturate(dot(L, N)) > 0.0f && attenuation > 0.0f;
        if(nextEventValid)
        {
            // trace shadow ray to check visibility
            RayDesc shadowRay;
            shadowRay.Origin = pos;
            shadowRay.Direction = L;
            shadowRay.TMin = 0.001;
            shadowRay.TMax = distance;

            ShadowPayload shadowPayload;
            shadowPayload.seed = payload.seed;

            TraceRay(tlas, RAY_FLAG_CULL_BACK_FACING_TRIANGLES, 0xFF, 1, 0, 1, shadowRay, shadowPayload);

            if(!shadowPayload.blocked)
            {
                float3 Li = light.color * light.intensity * attenuation;

                float3 brdfGlossy;
                float3 brdfDiffuse;
                float pdf;
                evaluateBRDF(albedo, metallic, roughness, N, V, L, brdfDiffuse, brdfGlossy, pdf);

                Lo += (brdfGlossy + brdfDiffuse) * (Li / lightSelectionPdf) * payload.throughput;
            }
        }
    }

    // GGX VNDF scatter direction sampling
    // sample new direction
    float NdotV = saturate(dot(N, V));
    float rand = randomFloat01(payload.seed);
    float glossyProb = brdfGlossyProbability(NdotV, metallic);

    // if visible
    if(NdotV > 0) { // normal mapping creates artifacting (for possible solutions see https://dl.acm.org/doi/10.1145/3130800.3130806)
        // depending on probability given by glossyness choose between vndf sampling or cosine weighted sampling for diffuse
        if(rand <= glossyProb)
        {
            // transform to local space
            float r1 = randomFloat01(payload.seed);
            float r2 = randomFloat01(payload.seed);
            float3 T = normalize(tangent.xyz - N * dot(N, tangent.xyz));
            float3 B = normalize(cross(N, T)) * tangent.w;
            float3 localV = float3(dot(T, V), dot(B, V), NdotV); // transform view vector into tangent space
            float3 H = sampleVNDF(localV, roughness, float2(r1, r2));
            // transform to world space
            H = T * H.x + B * H.y + N * H.z;
            payload.scatterDir = reflect(-V, H);
        }
        else
        {
            float3 T = normalize(tangent.xyz - N * dot(N, tangent.xyz));
            float3 B = normalize(cross(N, T)) * tangent.w;
            float3 localDir = cosineSampleHemisphere(randomFloat01(payload.seed), randomFloat01(payload.seed));
            payload.scatterDir = T * localDir.x + B * localDir.y + N * localDir.z;
        }

        float3 brdfGlossy;
        float3 brdfDiffuse;
        float pdf;
        evaluateBRDF(albedo,metallic, roughness, N, V, payload.scatterDir, brdfGlossy, brdfDiffuse, pdf);

        float3 brdfTotal = brdfDiffuse + brdfGlossy;

        if(any(isnan(brdfTotal)) || pdf <= 0.00001F)
        {
            payload.doScatter = false;
        }
        else
        {
            payload.throughput *= brdfTotal / pdf;
        }
    } else {
        // absorbed
        payload.doScatter = false;
    }

    // update radiance
    payload.radiance = Lo;
    payload.hitT = hitT;
    /*
    payload.radiance = float3(saturate(N));
    payload.doScatter = false;
    return;
    */
}

float getOpacity(shaderio::GltfRenderNode renderNode, shaderio::GltfRenderPrimitive renderPrim, int triangleID, float3 barycentrics)
{
    uint3 triangleIndex = renderPrim.indices[triangleID];
    uint matIndex  = renderNode.materialID;
    shaderio::GltfMaterial* materials = pushConst.sceneInfo->gltfMaterials;
    shaderio::GltfMaterial mat = materials[matIndex];
    shaderio::GltfTextureInfo* texInfos  = pushConst.sceneInfo->gltfTexturesInfos;

    if(mat.alphaMode == shaderio::AlphaMode::eAlphaModeOpaque)
        return 1.0;

    float alpha = 1;
    alpha = mat.baseColor.a;
    alpha *= getInterpolatedVertexColor(renderPrim, triangleIndex, barycentrics).a;

    if(mat.baseColorTextureID >= 0)
    {
        shaderio::GltfTextureInfo texInfo = texInfos[mat.baseColorTextureID];
        float2 uv = getInterpolatedVertexTexCoords(renderPrim, triangleIndex, barycentrics);

        alpha *= textures[texInfo.imageID].SampleLevel(samplers[texInfo.samplerID], uv, 0).a;
    }

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
    float r = randomFloat01(payload.seed);
    if(r > opacity) {
        IgnoreHit();
    }
}

[shader("miss")]
void missMain(inout HitPayload payload)
{
    payload.hitT = INFINITY;
    payload.radiance = pushConst.frameData.backgroundColor * payload.throughput;
    //payload.radiance = float3(0);
}

struct ShadowPayload {
    uint seed;
    bool blocked = false;
};

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

    if (r <= opacity)
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