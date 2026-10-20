#include "ImguiVulkan.h"

#include <filesystem>

bool IsMouseControlledByImGUI()
{
    ImGuiIO& io = ImGui::GetIO();

    bool ret = io.WantCaptureMouse;

    return ret;
}

namespace ptvk {
    ImguiVulkan::ImguiVulkan(Core& vkCore) : m_vkCore(vkCore) {
        vkCore.getFramebufferSize(m_framebufferWidth, m_framebufferHeight);

        // create descriptor pool
        vk::DescriptorPoolSize poolSizes[] = {
            { vk::DescriptorType::eSampler, 1000 },
            { vk::DescriptorType::eCombinedImageSampler, 1000 },
            { vk::DescriptorType::eSampledImage, 1000 },
            { vk::DescriptorType::eStorageImage, 1000 },
            { vk::DescriptorType::eUniformTexelBuffer, 1000 },
            { vk::DescriptorType::eStorageTexelBuffer, 1000 },
            { vk::DescriptorType::eUniformBuffer, 1000 },
            { vk::DescriptorType::eStorageBuffer, 1000 },
            { vk::DescriptorType::eUniformBufferDynamic, 1000 },
            { vk::DescriptorType::eStorageBufferDynamic, 1000 },
            { vk::DescriptorType::eInputAttachment, 1000 }
        };

        vk::DescriptorPoolCreateInfo poolCreateInfo{};
        poolCreateInfo.flags = vk::DescriptorPoolCreateFlagBits::eFreeDescriptorSet;
        poolCreateInfo.maxSets = 1000 * static_cast<uint32_t>(std::size(poolSizes));
        poolCreateInfo.poolSizeCount = static_cast<uint32_t>(std::size(poolSizes));
        poolCreateInfo.pPoolSizes = poolSizes;
        m_descriptorPool = m_vkCore.getDevice().getVkDevice().createDescriptorPool(poolCreateInfo);

        ImGui::CreateContext();

        ImGuiIO& io = ImGui::GetIO();
        io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
        io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;
        io.ConfigNavMoveSetMousePos = true;
        io.DisplaySize.x = static_cast<float>(m_framebufferWidth);
        io.DisplaySize.y = static_cast<float>(m_framebufferHeight);

        ImGui::StyleColorsDark();

        ImGui_ImplGlfw_InitForVulkan(m_vkCore.getWindow(), true);

        vk::PipelineRenderingCreateInfoKHR pipelineRenderingCreateInfo{};
        pipelineRenderingCreateInfo.colorAttachmentCount = 1;
        pipelineRenderingCreateInfo.pColorAttachmentFormats = &m_vkCore.getSwapchain().GetSwapchainFormat();
        pipelineRenderingCreateInfo.depthAttachmentFormat = vk::Format::eUndefined;
        pipelineRenderingCreateInfo.stencilAttachmentFormat = vk::Format::eUndefined;

        ImGui_ImplVulkan_InitInfo vkInitInfo = {};
        vkInitInfo.ApiVersion = m_vkCore.getInstanceVersion();
        vkInitInfo.Instance = m_vkCore.getInstance();
        vkInitInfo.PhysicalDevice = *m_vkCore.getDevice().getPhysicalDevice().m_physDevice;
        vkInitInfo.Device = *m_vkCore.getDevice().getVkDevice();
        vkInitInfo.QueueFamily = m_vkCore.getDevice().queueFamilyIndices.graphics;
        vkInitInfo.Queue = *m_vkCore.getQueue();
        vkInitInfo.DescriptorPool = *m_descriptorPool;
        vkInitInfo.MinImageCount = m_vkCore.getDevice().getPhysicalDevice().m_surfaceCapabilities.minImageCount;
        vkInitInfo.ImageCount = m_vkCore.getSwapchain().GetSwapchainImageCount();
        vkInitInfo.PipelineInfoMain.RenderPass = VK_NULL_HANDLE;
        vkInitInfo.PipelineInfoMain.Subpass = 0;
        vkInitInfo.PipelineInfoMain.MSAASamples = VK_SAMPLE_COUNT_1_BIT;
        vkInitInfo.PipelineInfoMain.PipelineRenderingCreateInfo = *&pipelineRenderingCreateInfo;
        vkInitInfo.UseDynamicRendering = true;

        ImGui_ImplVulkan_Init(&vkInitInfo);
    }

    ImguiVulkan::~ImguiVulkan() {
        ImGui_ImplVulkan_Shutdown();
        ImGui_ImplGlfw_Shutdown();
    }
}
