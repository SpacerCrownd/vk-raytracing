#ifndef VK_RAYTRACING_IMGUIVULKAN_H
#define VK_RAYTRACING_IMGUIVULKAN_H

#include <imgui.h>
#include <imgui_impl_glfw.h>
#include <imgui_impl_vulkan.h>

#include "Core.h"
#include "Vulkan.h"

bool IsMouseControlledByImGUI();

namespace ptvk {
class ImguiVulkan {
public:
    ImguiVulkan(Core& vkCore);
    ~ImguiVulkan();

private:
    Core& m_vkCore;

    int m_framebufferWidth{0};
    int m_framebufferHeight{0};

    vk::raii::DescriptorPool m_descriptorPool{nullptr};       // Pool for allocating descriptor sets
};
}

#endif //VK_RAYTRACING_IMGUIVULKAN_H
