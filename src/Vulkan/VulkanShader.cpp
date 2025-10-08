#include "VulkanShader.h"

#include <filesystem>
#include <fstream>

namespace PathTracingVk {
    VulkanShader::VulkanShader(const vk::raii::Device& device, const std::string& fileName) : m_device(device) {
        CreateShaderModule(ReadFile(fileName));
    }

    std::vector<char> VulkanShader::ReadFile(const std::string &fileName) {
        auto shaderPath = "shaders/" + fileName;
        std::ifstream file(shaderPath, std::ios::ate | std::ios::binary);
        if (!file.is_open()) {
            printf("[Error] Failed to open file %s\n", shaderPath.c_str());
        }

        printf("[INFO] Shader loaded %s", fileName.c_str());
        const auto size = static_cast<size_t>(file.tellg());
        std::vector<char> buffer(size);

        file.seekg(0);
        file.read(buffer.data(), size);
        file.close();
        return buffer;
    }

    void VulkanShader::CreateShaderModule(const std::vector<char> &code) {
        vk::ShaderModuleCreateInfo createInfo{
            .codeSize = code.size() * sizeof(char),
            .pCode = reinterpret_cast<const uint32_t*>(code.data()),
        };
        m_shader = vk::raii::ShaderModule(m_device, createInfo);
    }

    vk::PipelineShaderStageCreateInfo VulkanShader::CreateShaderStage(const vk::ShaderStageFlagBits stage, const char* pName = "main") const {
        vk::PipelineShaderStageCreateInfo info = {
            .sType = vk::StructureType::ePipelineShaderStageCreateInfo,
            .stage = stage,
            .module = m_shader,
            .pName = pName
        };

        return info;
    }
}
