#include "Shader.h"

#include <filesystem>
#include <fstream>

namespace ptvk {
    Shader::Shader(const vk::raii::Device& device, const std::string& fileName) : m_device(device) {
        CreateShaderModule(ReadFile(fileName));
    }

    std::vector<char> Shader::ReadFile(const std::string &fileName) {
        auto shaderPath = "shaders/" + fileName;
        std::ifstream file(shaderPath, std::ios::ate | std::ios::binary);
        if (!file.is_open()) {
            printf("[Error] Failed to open file %s\n", shaderPath.c_str());
        }

        printf("[INFO] Shader loaded %s \n", fileName.c_str());
        std::vector<char> buffer(file.tellg());

        file.seekg(0);
        file.read(buffer.data(), buffer.size());
        file.close();
        return buffer;
    }

    void Shader::CreateShaderModule(const std::vector<char> &code) {
        vk::ShaderModuleCreateInfo createInfo{
            .codeSize = code.size() * sizeof(char),
            .pCode = reinterpret_cast<const uint32_t*>(code.data()),
        };
        m_shader = vk::raii::ShaderModule(m_device, createInfo);
    }

    vk::PipelineShaderStageCreateInfo Shader::CreateShaderStage(const vk::ShaderStageFlagBits stage, const char* pName = "main") const {
        return vk::PipelineShaderStageCreateInfo {
            .sType = vk::StructureType::ePipelineShaderStageCreateInfo,
            .stage = stage,
            .module = m_shader,
            .pName = pName
        };
    }
}
