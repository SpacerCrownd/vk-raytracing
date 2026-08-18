#ifndef IO_GLTF_H
#define IO_GLTF_H

#include "slang_types.h"

namespace shaderio {
struct VertexBuffer {
    float3* positions;
    float3* normals;
    float3* tangents;
    float4* colors;
    float2* texCoords[2];
};

struct GltfNode { // object instances
    float4x4 objectToWorld;
    float4x4 worldToObject;
    int      primitiveID = -1;
};

struct GltfPrimitive { // gltf calls submeshes primitives
    uint3* indices;
    int    materialID = -1;
};

struct GltfMaterial {
    float4 baseColor;             // albedo (RGBA format)
    float  metallic;              // metallicness
    float  roughness;             // roughness
    int    baseColorTextureID = -1;  // texture index (optional)
    int    roughnessTextureID = -1;
    int    metallicTextureID  = -1;
};

struct GltfSceneInfo {
    GltfSceneInfo* gltfSceneInfo;
    GltfPrimitive* gltfPrimitives;
    GltfNode*      gltfNodes;
    GltfMaterial*  gltfMaterials;
    // textures
    // lights
    // samplers?
};

struct FrameData {
    float4x4 projection;
    float4x4 view;
    float3   cameraPosition;
    int      useSky;
    float3   backgroundColor;
    int      numLights;
};

struct PushConstants {
    FrameData frameData;
};
}

#endif