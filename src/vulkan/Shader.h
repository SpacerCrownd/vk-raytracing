#ifndef VK_RAYTRACING_SHADER_H
#define VK_RAYTRACING_SHADER_H

#include "Vulkan.h"
#include <vector>

namespace ptvk {

class Shader {
public:
    Shader(const vk::raii::Device& device, const std::string& fileName);
    ~Shader() = default;

    const vk::raii::ShaderModule& getShaderModule() const {
        return m_shader;
    }

    vk::PipelineShaderStageCreateInfo createShaderStage(vk::ShaderStageFlagBits stage, const char *pName) const;

private:
    const vk::raii::Device& m_device;
    vk::raii::ShaderModule  m_shader{VK_NULL_HANDLE};

    static std::vector<char> readFile(const std::string& fileName);

    void createShaderModule(const std::vector<char>& code);
};

}

#endif //VK_RAYTRACING_SHADER_H