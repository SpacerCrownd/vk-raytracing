#ifndef IO_GLTF_H
#define IO_GLTF_H

#include "slang_types.h"

// data structures shared by shaders and application classes
namespace shaderio {
struct VertexBuffer {
    float3* positions;
    float3* normals;
    float3* tangents;
    float4* colors;
    float2* texCoords[2];
};

struct GltfRenderNode { // object instances
    float4x4 objectToWorld; // model matrix
    float4x4 worldToObject;
    int      renderPrimID;
    int      materialID;
};

struct GltfRenderPrimitive { // gltf calls submeshes primitives
    uint3*       indices;
    VertexBuffer vertexBuffer;
};

struct GltfTextureInfo {
    int imageID;
    int samplerID;
}; // 8 bytes

struct GltfMaterial {
    float4 baseColor;                // albedo          0, 16
    float  metallic;                 // metallicness    16, 4
    float  roughness;                // roughness       20, 4
    int    baseColorTextureID = -1;  // texture index   24, 4
    int    normalTextureID    = -1;  //                 28, 4
    int    roughnessTextureID = -1;  //                 32, 4
    int    metallicTextureID  = -1;  //                 36, 4
}; // 40 bytes


enum LightType {
    eDirectional,
    ePoint,
    eSpot,
};

struct GltfLight {
    float3 direction;
    int    type;
    float3 position;
    float  radius;
    float3 color;
    float  intensity;

    float angularSizeOrInvRange;
    float innerAngle;
    float outerAngle;
};

struct GltfSceneInfo {
    GltfRenderPrimitive* gltfPrimitives;
    GltfRenderNode*      gltfNodes;
    GltfMaterial*        gltfMaterials;
    GltfLight*           gltfLights;
    GltfTextureInfo*     gltfTexturesInfos;
    int                  numLights;
};
}

#endif