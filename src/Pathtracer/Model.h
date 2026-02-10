#ifndef VK_RAYTRACING_MODEL_H
#define VK_RAYTRACING_MODEL_H

#include <glm/glm.hpp>
#include "tinyobjloader/tiny_obj_loader.h"

namespace PathTracingVk {

struct Vertex {
    glm::vec3 position;
    glm::vec3 normal;
    glm::vec2 uv;
};

class Model {
public:
    Model() = default;
    ~Model() = default;

    tinyobj::attrib_t attrib;
    std::vector<tinyobj::shape_t> shapes;
    std::vector<tinyobj::material_t> materials;
private:

};
}


#endif //VK_RAYTRACING_MODEL_H