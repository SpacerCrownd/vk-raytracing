#include "Camera.h"

#include <algorithm>
#include <ostream>

#include "glm/gtc/matrix_transform.hpp"
#include "glm/gtx/quaternion.hpp"

namespace app {
Camera::Camera(glm::vec3 pos) : position(pos) {}

void Camera::Update() {
    glm::mat4 cameraRotation = GetRotationMatrix();
    position += glm::vec3(cameraRotation * glm::vec4(velocity * 0.5f, 0.f));
}

void Camera::OnKeyChanged(int key, int scan, int action, int mods) {

    if (action == GLFW_PRESS) {
        //printf("Pressed Key code: %d\n", key);
        if (key == GLFW_KEY_W) {
            velocity.z = -1;
        }

        if (key == GLFW_KEY_A) {
            velocity.x = -1;
        }

        if (key == GLFW_KEY_S) {
            velocity.z = 1;
        }

        if (key == GLFW_KEY_D) {
            velocity.x = 1;
        }
    }

    if (action == GLFW_RELEASE) {
        //printf("Released Key code: %d\n", key);
        if (key == GLFW_KEY_W) {
            velocity.z = 0;
        }

        if (key == GLFW_KEY_A) {
            velocity.x = 0;
        }

        if (key == GLFW_KEY_S) {
            velocity.z = 0;
        }

        if (key == GLFW_KEY_D) {
            velocity.x = 0;
        }
    }
}

void Camera::OnMouseButtonChanged(GLFWwindow* window, int button, int action, int mods) {
    if (button == GLFW_MOUSE_BUTTON_LEFT)
    {
        if (action == GLFW_PRESS)
        {
            glfwGetCursorPos(window, &lastX, &lastY);
            dragging = true;
            //std::cout << "Dragging" << std::endl;
        }
        else if (action == GLFW_RELEASE)
        {
            dragging = false;
            //std::cout << "Stopped Dragging" << std::endl;
        }
    }
}

void Camera::OnCursorPositionChanged(double xpos, double ypos) {
    if (!dragging)
        return;

    double dx = xpos - lastX;
    double dy = ypos - lastY;

    lastX = xpos;
    lastY = ypos;

    // apply drag to camera
    double sensitivity{0.1};
    yaw   += static_cast<float>(dx) * static_cast<float>(sensitivity);
    pitch += static_cast<float>(dy) * static_cast<float>(sensitivity);

    // optional clamp
    pitch = std::clamp(pitch, -89.0f, 89.0f);

    //std::cout << "pitch: " << pitch << std::endl;
    //std::cout << "yaw: " << yaw << std::endl;
}

glm::mat4 Camera::GetViewMatrix() const {
    glm::mat4 cameraTranslation = glm::translate(glm::mat4(1.f), position);
    glm::mat4 cameraRotation = GetRotationMatrix();
    return glm::inverse(cameraTranslation * cameraRotation);
}

glm::mat4 Camera::GetRotationMatrix() const {
    glm::quat pitchRotation = glm::angleAxis(pitch, glm::vec3 { 1.f, 0.f, 0.f });
    glm::quat yawRotation = glm::angleAxis(yaw, glm::vec3 { 0.f, -1.f, 0.f });
    return glm::toMat4(yawRotation) * glm::toMat4(pitchRotation);
}
}

