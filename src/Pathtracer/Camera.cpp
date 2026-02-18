#include "Camera.h"

#include <algorithm>

#include "glm/gtc/matrix_transform.hpp"
#include "glm/gtx/quaternion.hpp"
#include "GLFW/glfw3.h"

namespace PathTracingVk {
void Camera::Update() {
    glm::mat4 cameraRotation = GetRotationMatrix();
    position += glm::vec3(cameraRotation * glm::vec4(velocity * 0.5f, 0.f));
}

void Camera::OnKeyChanged(int key, int scan, int action, int mods) {
    printf("Key code: %d\n", key);
    if (action == GLFW_PRESS) {
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

void Camera::OnMouseButtonChanged(int button, int action, int mods) {
    if (button == GLFW_MOUSE_BUTTON_LEFT)
    {
        if (action == GLFW_PRESS)
        {
            dragging = true;
            glfwGetCursorPos(glfwGetCurrentContext(), &lastX, &lastY);
        }
        else if (action == GLFW_RELEASE)
        {
            dragging = false;
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
}

glm::mat4 Camera::GetViewMatrix() {
    // to create a correct model view, we need to move the world in opposite
    // direction to the camera
    // so we will create the camera model matrix and invert
    glm::mat4 cameraTranslation = glm::translate(glm::mat4(1.f), position);
    glm::mat4 cameraRotation = GetRotationMatrix();
    return glm::inverse(cameraTranslation * cameraRotation);
}

glm::mat4 Camera::GetRotationMatrix() {
    // fairly typical FPS style camera. we join the pitch and yaw rotations into
    // the final rotation matrix
    glm::quat pitchRotation = glm::angleAxis(pitch, glm::vec3 { 1.f, 0.f, 0.f });
    glm::quat yawRotation = glm::angleAxis(yaw, glm::vec3 { 0.f, -1.f, 0.f });
    return glm::toMat4(yawRotation) * glm::toMat4(pitchRotation);
}
}

