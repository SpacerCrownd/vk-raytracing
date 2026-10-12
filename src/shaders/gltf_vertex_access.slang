#include "gltfio.h.slang"

__generic<let N : int> public vector<float, N> mixBary(vector<float, N> a, vector<float, N> b, vector<float, N> c, float3 bary)
{
  return a * bary.x + b * bary.y + c * bary.z;
}

float3 getInterpolatedVertexPosition(shaderio::GltfRenderPrimitive primitive, uint3 idx, float3 barycentrics)
{
    float3 positions[3];
    positions[0] = primitive.vertices[idx.x].position;
    positions[1] = primitive.vertices[idx.y].position;
    positions[2] = primitive.vertices[idx.z].position;

    return mixBary(positions[0], positions[1], positions[2], barycentrics);
}

float3 getInterpolatedVertexNormal(shaderio::GltfRenderPrimitive primitive, uint3 idx, float3 barycentrics)
{
    float3 normals[3];
    normals[0] = primitive.vertices[idx.x].normal;
    normals[1] = primitive.vertices[idx.y].normal;
    normals[2] = primitive.vertices[idx.z].normal;

    return mixBary(normals[0], normals[1], normals[2], barycentrics);
}

float2 getInterpolatedVertexTexCoords(shaderio::GltfRenderPrimitive primitive, uint3 idx, float3 barycentrics)
{
    float2 texCoords[3];
    texCoords[0] = float2(primitive.vertices[idx.x].u, primitive.vertices[idx.x].v);
    texCoords[1] = float2(primitive.vertices[idx.y].u, primitive.vertices[idx.y].v);
    texCoords[2] = float2(primitive.vertices[idx.z].u, primitive.vertices[idx.z].v);

    return mixBary(texCoords[0], texCoords[1], texCoords[2], barycentrics);
}

float4 getInterpolatedVertexColor(shaderio::GltfRenderPrimitive primitive, uint3 idx, float3 barycentrics)
{
    float4 colors[3];
    colors[0] = primitive.vertices[idx.x].color;
    colors[1] = primitive.vertices[idx.y].color;
    colors[2] = primitive.vertices[idx.z].color;

    return mixBary(colors[0], colors[1], colors[2], barycentrics);
}

float4 getInterpolatedVertexTangent(shaderio::GltfRenderPrimitive primitive, uint3 idx, float3 barycentrics)
{
    float3 tangents[3];
    tangents[0] = primitive.vertices[idx.x].tangent.xyz;
    tangents[1] = primitive.vertices[idx.y].tangent.xyz;
    tangents[2] = primitive.vertices[idx.z].tangent.xyz;

    return float4(mixBary(tangents[0], tangents[1], tangents[2], barycentrics), primitive.vertices[idx.x].tangent.w);
}

