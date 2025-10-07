#ifndef VK_RAYTRACING_SHADER_H
#define VK_RAYTRACING_SHADER_H

#include "Vulkan.h"
#include <vector>

namespace PathTracingVk {

class Shader {
public:
    Shader(const vk::raii::Device& device, const std::string& fileName);
    ~Shader() = default;

    [[nodiscard]] const vk::raii::ShaderModule& Get() const {
        return m_shader;
    }

    vk::PipelineShaderStageCreateInfo CreateShaderStage(vk::ShaderStageFlagBits stage, const char *pName) const;

private:
    const vk::raii::Device& m_device;
    vk::raii::ShaderModule m_shader = VK_NULL_HANDLE;

    static std::vector<char> ReadFile(const std::string& fileName);
    void CreateShaderModule(const std::vector<char>& code);
};

}

#endif //VK_RAYTRACING_SHADER_H