#ifndef IO_GLTF_H
#define IO_GLTF_H

#include "slang_types.h"

namespace shaderio {
struct BufferView {
    uint32_t offset;
    uint32_t count;
    uint32_t stride;
};

struct GltfMesh { // just a view into geometry data to locate meshes
    uint8_t* gltfDataAddress; // address of geometry data buffer on gpu
    int primitiveCount;
    int primitiveOffset;
    int indexType;
};

struct GltfPrimitive {
    BufferView positions;
    BufferView normals;
    BufferView uvs;
    BufferView indices;
    uint32_t materialIndex;
};

struct GltfInstance {
    float4x4 transform;
    uint32_t meshIndex;
};

struct GltfMaterial {
    float4 baseColorFactor;             // albedo (RGBA format)
    float  metallicFactor;              // metallicness
    float  roughnessFactor;             // roughness
    int    baseColorTextureIndex = -1;  // texture index (optional)
    // Future development: Normal texture, roughness texture, metallic texture
};

struct GltfSceneInfo {
    float4x4 projection;
    float4x4 view;
    float3 cameraPosition;
    int useSky;
    float3 backgroundColor;
    int numLights;
    GltfMesh* meshes;
    GltfPrimitive* primitives;
    GltfInstance* instances;
    GltfMaterial* materials;
    // TODO: lights gpu address here
};
}

#endif