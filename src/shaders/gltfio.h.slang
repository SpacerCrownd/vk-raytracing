#ifndef IO_GLTF_H
#define IO_GLTF_H

#include "slang_types.h"

// data structures shared by shaders and application classes
namespace shaderio {
struct Vertex {
    float3 position;
    float3 normal;
    float2 texCoords;
    float4 color;
    float4 tangent;
};

struct GltfRenderNode { // object instances
    float4x4 objectToWorld; // model matrix
    float4x4 worldToObject;
    int      renderPrimID;
    int      materialID;
};

struct GltfRenderPrimitive { // gltf calls submeshes -> primitives
    uint3*  indices; // bda of array of tuple of indices of each triangle
    Vertex* vertices; // bda of array of vertices
};

struct GltfTextureInfo {
    int imageID;
    int samplerID;
};

enum AlphaMode {
  eAlphaModeOpaque,
  eAlphaModeMask,
  eAlphaModeBlend,
};

struct GltfMaterial {
    float4 baseColor = float4(1);
    float3 emissiveColor = float3(0);
    float  normalTextureScale = 1;

    float  metallic = 1;
    float  roughness = 1;
    int    alphaMode = AlphaMode::eAlphaModeOpaque;
    float  alphaCutoff = 0.5f;

    float  occlusionStrength = 1;
    int    doubleSided = 0;

    int baseColorTextureID = -1;
    int normalTextureID    = -1;
    int occlusionTextureID = -1;
    int metallicRoughnessTextureID = -1;
    int emissiveTextureID = -1;
};

enum LightType {
    eDirectional,
    ePoint,
    eSpot,
};

struct GltfLight {
    float3 direction;
    int    type;
    float3 position;
    float  intensity;
    float3 color;
    float  range;
    float  innerAngle;
    float  outerAngle;
};

// right now these don't change because these buffers are not resized/recreated but only contents are modified
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