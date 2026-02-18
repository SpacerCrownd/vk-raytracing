#ifndef VK_RAYTRACING_CAMERA_H
#define VK_RAYTRACING_CAMERA_H

#define GLM_ENABLE_EXPERIMENTAL
#include <glm/glm.hpp>

namespace PathTracingVk {
class Camera {
public:
    glm::vec3 velocity;
    glm::vec3 position;
    float pitch { 0.f };
    float yaw { 0.f };

    bool dragging { false };
    double lastX { 0.f };
    double lastY { 0.f };

    glm::mat4 GetViewMatrix();
    glm::mat4 GetRotationMatrix();

    void OnKeyChanged(int key, int scancode, int action, int mods);
    void OnMouseButtonChanged(int button, int action, int mods);
    void OnCursorPositionChanged(double xpos, double ypos);
    void Update();
};
}


#endif //VK_RAYTRACING_CAMERA_H