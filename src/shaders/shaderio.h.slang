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
    float4   cameraPosition; // float 4 for buffer alignment
    float4   backgroundColor;
};

struct RtPushConstant {
    FrameData*      frameData;
    GltfSceneInfo*  sceneInfo;
    int             maxDepth     = 3;
    int             frameCount   = 0;
    int             numSamples   = 1;
    int             totalSamples = 0;
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
