#ifndef SHADERIO_H
#define SHADERIO_H

#include "gltfio.h.slang"

namespace shaderio {
enum DescriptorBindingPoints {
    eTlas = 0,
    eOutImage,
    eTextures = 0,
    eSamplers,
};

enum OutputImage {
    eResult = 0,
    eAlbedo,
    eNormal,
    eAccumulation
};

struct FrameData {
    float4x4 viewProjMatrix;
    float4x4 projInvMatrix;
    float4x4 viewInvMatrix;
    float3   cameraPosition;
    int      exposureEV;
    float3   backgroundColor;
    float    _padding;
    float3   lightDir;
    int      useLight;
    float4   lightColor; // w is intensity
};

struct RtPushConstant {
    FrameData*      frameData;
    GltfSceneInfo*  sceneInfo;
    int             maxDepth     = 3;
    int             numSamples   = 1;
    int             totalSamples = 0;
    int             maxSamples = 0;
};

struct RasterPushConstant {
    FrameData*      frameData;
    GltfSceneInfo*  sceneInfo;
    int             materialID;
    int             renderNodeID;
    int             renderPrimID;
};

}
#endif
