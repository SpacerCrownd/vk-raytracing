#include "Window.h"
#include <iostream>

namespace PathTracingVk {
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

Window::Window(int width, int height, const char* pName) : m_width(width), m_height(height), m_pName(pName) {
    glfwSetErrorCallback(GlfwErrorCallback);

    if (!glfwInit()) {
        throw std::runtime_error("Failed to initialize GLFW");
    }

    if (!glfwVulkanSupported()) {
        throw std::runtime_error("Vulkan not supported");
    }

    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
    //glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);

    m_window = glfwCreateWindow(width, height, pName, nullptr, nullptr);

    glfwSetWindowUserPointer(m_window, this);
};

Window::~Window() {
    glfwDestroyWindow(m_window);
    glfwTerminate();
}

vk::Extent2D Window::GetExtent() const {
    int width, height;
    glfwGetWindowSize(m_window, &width, &height);
    return vk::Extent2D{static_cast<uint32_t>(width), static_cast<uint32_t>(height)};
}


}