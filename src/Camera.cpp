#include "Camera.h"

#include <algorithm>
#include <iostream>

#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtx/quaternion.hpp>

namespace app {
Camera::Camera(glm::vec3 pos) : position(pos) {}

void Camera::update(double deltaTime) {
    if (glm::length(velocity) > 0) {
        changed = true;
    }

    glm::mat4 cameraRotation = getRotationMatrix();
    position += glm::vec3(cameraRotation * glm::vec4(velocity * static_cast<float>(deltaTime), 0.f));
}

void Camera::onKeyChanged(int key, int scancode, int action, int mods) {
    if (action == GLFW_PRESS) {
        //printf("Pressed Key code: %d\n", key);
        if (key == GLFW_KEY_W) {
            velocity.z = -speed;
        }

        if (key == GLFW_KEY_A) {
            velocity.x = -speed;
        }

        if (key == GLFW_KEY_S) {
            velocity.z = speed;
        }

        if (key == GLFW_KEY_D) {
            velocity.x = speed;
        }

        if (key == GLFW_KEY_E) {
            velocity.y = speed;
        }

        if (key == GLFW_KEY_Q) {
            velocity.y = -speed;
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

        if (key == GLFW_KEY_E) {
            velocity.y = 0;
        }

        if (key == GLFW_KEY_Q) {
            velocity.y = 0;
        }
    }
}

void Camera::onMouseButtonChanged(GLFWwindow* window, int button, int action, int mods) {
    if (button == GLFW_MOUSE_BUTTON_LEFT)
    {
        if (action == GLFW_PRESS)
        {
            glfwGetCursorPos(window, &lastX, &lastY);
            glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
            dragging = true;
            //std::cout << "Dragging" << std::endl;
        }
        else if (action == GLFW_RELEASE)
        {
            dragging = false;
            glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_NORMAL);
            //std::cout << "Stopped Dragging" << std::endl;
        }
    }
}

void Camera::onCursorPositionChanged(double xpos, double ypos) {
    if (!dragging)
        return;

    double dx = xpos - lastX;
    double dy = ypos - lastY;

    lastX = xpos;
    lastY = ypos;

    // apply drag to camera
    yaw   += static_cast<float>(dx) * static_cast<float>(sensitivity);
    pitch -= static_cast<float>(dy) * static_cast<float>(sensitivity);

    pitch = std::clamp(pitch, glm::radians(-89.0f), glm::radians(89.0f));

    if (dx > 0 || dy > 0)
        changed = true;
    //std::cout << "pitch: " << pitch << std::endl;
    //std::cout << "yaw: " << yaw << std::endl;
}

void Camera::onScroll(double xoffset, double yoffset) {
    if (yoffset < 0) {
        speed = std::max(speed - 0.5f, 1.0f);
    } else if (yoffset > 0) {
        speed += 0.5f;
    }
}

glm::mat4 Camera::getViewMatrix() const {
    glm::mat4 cameraTranslation = glm::translate(glm::mat4(1.f), position);
    glm::mat4 cameraRotation = getRotationMatrix();
    return glm::inverse(cameraTranslation * cameraRotation);
}

glm::mat4 Camera::getRotationMatrix() const {
    glm::quat pitchRotation = glm::angleAxis(pitch, glm::vec3 { 1.f, 0.f, 0.f });
    glm::quat yawRotation = glm::angleAxis(yaw, glm::vec3 { 0.f, -1.f, 0.f });
    return glm::toMat4(yawRotation) * glm::toMat4(pitchRotation);
}
}

