#include "Window.h"
#include "../Renderer.h"
#include <iostream>

namespace ptvk {
void Window::glfwErrorCallback(const int error, const char* const description)
{
    std::cerr << "[ERROR] GLFW: " << description << " (code: " << error << ")" << std::endl;
}

void Window::glfwKeyCallback(GLFWwindow* window, int key, int scancode, int action, int mods) {
    auto* const this_ = static_cast<Window*>(glfwGetWindowUserPointer(window));

    for (auto& callback : this_->onKeyChanged)
        callback(key, scancode, action, mods);
}

void Window::glfwCursorPositionCallback(GLFWwindow* window, const double xpos, const double ypos)
{
    auto* const this_ = static_cast<Window*>(glfwGetWindowUserPointer(window));
    for (auto& callback : this_->onCursorPositionChanged)
        callback(xpos, ypos);
}

void Window::glfwMouseButtonCallback(GLFWwindow* window, const int button, const int action, const int mods)
{
    auto* const this_ = static_cast<Window*>(glfwGetWindowUserPointer(window));
    for (auto& callback : this_->onMouseButtonChanged)
        callback(button, action, mods);
}

void Window::glfwScrollCallback(GLFWwindow* window, const double xoffset, const double yoffset)
{
    auto* const this_ = static_cast<Window*>(glfwGetWindowUserPointer(window));
    for (auto& callback : this_->onScrollChanged)
        callback(xoffset, yoffset);
}

void Window::glfwFramebufferSizeCallback(GLFWwindow *window, int width, int height) {
    auto* const this_ = static_cast<Window*>(glfwGetWindowUserPointer(window));
    for (auto& callback : this_->onFramebufferSizeChanged)
        callback(width, height);
}

void Window::addOnKeyChanged(std::function<void(int key, int scancode, int action, int mods)> callback) {
    onKeyChanged.push_back(std::move(callback));
}

void Window::addOnCursorPositionChanged(std::function<void(double xpos, double ypos)> callback) {
    onCursorPositionChanged.push_back(std::move(callback));
}

void Window::addOnMouseButtonChanged(std::function<void(int button, int action, int mods)> callback) {
    onMouseButtonChanged.push_back(std::move(callback));
}

void Window::addOnScrollChanged(std::function<void(double xoffset, double yoffset)> callback) {
    onScrollChanged.push_back(std::move(callback));
}

void Window::addOnFramebufferSizeChanged(std::function<void(int width, int height)> callback) {
    onFramebufferSizeChanged.push_back(std::move(callback));
}

Window::Window(int width, int height, const char* pName) : m_width(width), m_height(height), m_pName(pName) {
    glfwSetErrorCallback(glfwErrorCallback);

    if (!glfwInit()) {
        throw std::runtime_error("Failed to initialize GLFW");
    }

    if (!glfwVulkanSupported()) {
        throw std::runtime_error("Vulkan not supported");
    }

    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);

    m_window = glfwCreateWindow(width, height, pName, nullptr, nullptr);

    glfwSetWindowUserPointer(m_window, this);

    glfwSetCursorPosCallback(m_window, glfwCursorPositionCallback);
    glfwSetMouseButtonCallback(m_window, glfwMouseButtonCallback);
    glfwSetScrollCallback(m_window, glfwScrollCallback);
    glfwSetKeyCallback(m_window, glfwKeyCallback);
    glfwSetFramebufferSizeCallback(m_window, glfwFramebufferSizeCallback);
};

Window::~Window() {
    glfwDestroyWindow(m_window);
    glfwTerminate();
}

}