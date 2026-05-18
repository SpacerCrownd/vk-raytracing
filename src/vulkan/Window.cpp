#include "Window.h"
#include "../Renderer.h"
#include <iostream>

namespace ptvk {
void Window::GlfwErrorCallback(const int error, const char* const description)
{
    std::cerr << "[ERROR] GLFW: " << description << " (code: " << error << ")" << std::endl;
}

void Window::GlfwKeyCallback(GLFWwindow* window, int key, int scancode, int action, int mods) {
    auto* const this_ = static_cast<Window*>(glfwGetWindowUserPointer(window));

    for (auto& callback : this_->onKeyChanged)
        callback(key, scancode, action, mods);
}

void Window::GlfwCursorPositionCallback(GLFWwindow* window, const double xpos, const double ypos)
{
    auto* const this_ = static_cast<Window*>(glfwGetWindowUserPointer(window));
    for (auto& callback : this_->onCursorPositionChanged)
        callback(xpos, ypos);
}

void Window::GlfwMouseButtonCallback(GLFWwindow* window, const int button, const int action, const int mods)
{
    auto* const this_ = static_cast<Window*>(glfwGetWindowUserPointer(window));
    for (auto& callback : this_->onMouseButtonChanged)
        callback(button, action, mods);
}

void Window::GlfwScrollCallback(GLFWwindow* window, const double xoffset, const double yoffset)
{
    auto* const this_ = static_cast<Window*>(glfwGetWindowUserPointer(window));
    for (auto& callback : this_->onScrollChanged)
        callback(xoffset, yoffset);
}

void Window::AddOnKeyChanged(std::function<void(int key, int scancode, int action, int mods)> callback) {
    onKeyChanged.push_back(std::move(callback));
}

void Window::AddOnCursorPositionChanged(std::function<void(double xpos, double ypos)> callback) {
    onCursorPositionChanged.push_back(std::move(callback));
}

void Window::AddOnMouseButtonChanged(std::function<void(int button, int action, int mods)> callback) {
    onMouseButtonChanged.push_back(std::move(callback));
}

void Window::AddOnScrollChanged(std::function<void(double xoffset, double yoffset)> callback) {
    onScrollChanged.push_back(std::move(callback));
}

Window::Window(int width, int height, const char* pName) : m_width(width), m_height(height), m_pName(pName) {
    glfwSetErrorCallback(GlfwErrorCallback);

    if (!glfwInit()) {
        throw std::runtime_error("Failed to initialize GLFW");
    }

    if (!glfwVulkanSupported()) {
        throw std::runtime_error("Vulkan not supported");
    }

    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);

    m_window = glfwCreateWindow(width, height, pName, nullptr, nullptr);

    glfwSetWindowUserPointer(m_window, this);

    glfwSetCursorPosCallback(m_window, GlfwCursorPositionCallback);
    glfwSetMouseButtonCallback(m_window, GlfwMouseButtonCallback);
    glfwSetScrollCallback(m_window, GlfwScrollCallback);
    glfwSetKeyCallback(m_window, GlfwKeyCallback);
};

Window::~Window() {
    glfwDestroyWindow(m_window);
    glfwTerminate();
}

}