#ifndef VK_RAYTRACING_CAMERA_H
#define VK_RAYTRACING_CAMERA_H

#define GLM_ENABLE_EXPERIMENTAL

#include <glm/glm.hpp>
#include "GLFW/glfw3.h"

namespace app {
class Camera {
public:
    Camera(glm::vec3 pos);

    glm::vec3 velocity {0.f, 0.f, 0.f};
    glm::vec3 position;
    float pitch { 0.f };
    float yaw { 0.f };

    bool dragging { false };
    double lastX { 0.f };
    double lastY { 0.f };

    [[nodiscard]] glm::mat4 GetViewMatrix() const;
    [[nodiscard]] glm::mat4 GetRotationMatrix() const;

    void OnKeyChanged(int key, int scancode, int action, int mods);
    void OnMouseButtonChanged(GLFWwindow* window, int button, int action, int mods);
    void OnCursorPositionChanged(double xpos, double ypos);
    void Update();
};
}


#endif //VK_RAYTRACING_CAMERA_H