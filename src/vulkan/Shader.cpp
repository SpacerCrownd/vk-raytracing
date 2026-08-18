#include "Shader.h"

#include <iostream>
#include <filesystem>
#include <fstream>

namespace ptvk {
    Shader::Shader(const vk::raii::Device& device, const std::string& fileName) : m_device(device) {
        createShaderModule(readFile(fileName));
    }

    std::vector<char> Shader::readFile(const std::string &fileName) {
        auto shaderPath = "shaders/" + fileName;

        std::ifstream file(shaderPath, std::ios::ate | std::ios::binary);
        if (!file.is_open()) {
            std::cout << "[Error] Failed to open file " << shaderPath.c_str() << std::endl;
        }
        std::cout << "[INFO] Shader loaded %s \n" << fileName.c_str() << std::endl;

        std::vector<char> buffer(file.tellg());

        file.seekg(0);
        file.read(buffer.data(), buffer.size());
        file.close();
        return buffer;
    }

    void Shader::createShaderModule(const std::vector<char> &code) {
        vk::ShaderModuleCreateInfo createInfo{
            .codeSize = code.size() * sizeof(char),
            .pCode = reinterpret_cast<const uint32_t*>(code.data()),
        };

        m_shader = vk::raii::ShaderModule(m_device, createInfo);
    }

    vk::PipelineShaderStageCreateInfo Shader::createShaderStage(const vk::ShaderStageFlagBits stage, const char* pName = "main") const {
        return vk::PipelineShaderStageCreateInfo {
            .sType = vk::StructureType::ePipelineShaderStageCreateInfo,
            .stage = stage,
            .module = m_shader,
            .pName = pName
        };
    }
}
