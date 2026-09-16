#ifndef SHADERIO_H
#define SHADERIO_H

#include "gltfio.h.slang"

namespace shaderio {

enum DescriptorBindingPoints {
    eTlas = 0,
    eOutImage,
    eTextures = 0,
    eSamplers,
    eEnvCube,
};

struct FrameData {
    float4x4 projectionMat;
    float4x4 viewMat;
    float4   cameraPosition;
    float4   backgroundColor;
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
