#ifndef VK_RAYTRACING_MODEL_H
#define VK_RAYTRACING_MODEL_H

#include <glm/glm.hpp>
#include "VulkanCore.h"
#include "Resources.h"

namespace PathTracingVk {

struct Vertex {
    glm::vec3 pos;
    glm::vec3 normal;
    glm::vec2 uv;
};

// TODO: Create abstraction later
class Model {
public:
    explicit Model(VulkanCore& core);
    ~Model() = default;

    const Buffer* GetVertexBuffer() const { return &m_vBuff; };
    const Buffer* GetIndexBuffer() const { return &m_iBuff; };

private:
    VulkanCore& m_vkCore;
    Buffer m_vBuff;
    Buffer m_iBuff;

};
}


#endif //VK_RAYTRACING_MODEL_H