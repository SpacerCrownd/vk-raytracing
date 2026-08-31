#ifndef SHADERIO_H
#define SHADERIO_H

#include "gltfio.h.slang"

namespace shaderio {

enum DescriptorBindingPoints {
    eTlas,
    eOutImages,
    eTextures,
    eEnvCube,
    eSamplers
};

struct FrameData {
    float4x4 projectionMat;
    float4x4 viewMat;
    float3   cameraPosition;
    int      useSky;
    float3   backgroundColor;
};

struct RtPushConstant {
    int             maxDepth     = 3;
    int             frameCount   = 0;
    int             numSamples   = 1;
    int             totalSamples = 0;
    FrameData*      frameData;
    GltfSceneInfo*  sceneInfo;
};

struct RasterPushConstant {
    int             materialID;
    int             renderNodeID;
    int             renderPrimID;
    FrameData*      frameData;
    GltfSceneInfo*  sceneInfo;
};

}
#endif
