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

    glm::mat4 getViewMatrix() const;
    glm::mat4 getRotationMatrix() const;

    void onKeyChanged(int key, int scancode, int action, int mods);
    void onMouseButtonChanged(GLFWwindow* window, int button, int action, int mods);
    void onCursorPositionChanged(double xpos, double ypos);
    void update();
};
}


#endif //VK_RAYTRACING_CAMERA_H