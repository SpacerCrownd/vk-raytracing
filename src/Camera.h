#ifndef VK_RAYTRACING_CAMERA_H
#define VK_RAYTRACING_CAMERA_H

#define GLM_ENABLE_EXPERIMENTAL

#include <glm/glm.hpp>
#include <GLFW/glfw3.h>

namespace app {
class Camera {
public:
    explicit Camera(glm::vec3 pos);

    glm::vec3   velocity{0.f, 0.f, 0.f};
    glm::vec3   position;
    float       pitch{0.f};
    float       yaw{0.f};
    float       speed{5.0f};

    double sensitivity{0.001f};

    bool    dragging{false};
    double  lastX{0.f};
    double  lastY{0.f};

    bool changed{false};

    int exposureEV{0};

    struct {
        bool w = false;
        bool a = false;
        bool s = false;
        bool d = false;
        bool e = false;
        bool q = false;
    } keys;

    glm::mat4 getViewMatrix() const;
    glm::mat4 getRotationMatrix() const;

    void onKeyChanged(int key, int scancode, int action, int mods);
    void onMouseButtonChanged(GLFWwindow* window, int button, int action, int mods);
    void onCursorPositionChanged(double xpos, double ypos);
    void onScroll(double xoffset, double yoffset);
    void update(double deltaTime);

    void updateVelocity();
};
}


#endif //VK_RAYTRACING_CAMERA_H