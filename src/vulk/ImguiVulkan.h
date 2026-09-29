#ifndef VK_RAYTRACING_IMGUIVULKAN_H
#define VK_RAYTRACING_IMGUIVULKAN_H

#include <imgui/imgui.h>
#include "Vulkan.h"
#include "Resources.h"


namespace ptvk {
class ImguiVulkan {
public:

private:
    vk::raii::Sampler sampler{nullptr};

    Buffer vertexBuffer;
    Buffer indexBuffer;
};
}

#endif //VK_RAYTRACING_IMGUIVULKAN_H
