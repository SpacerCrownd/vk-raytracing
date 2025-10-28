#include "VulkanWindow.h"
#include "../Pathtracer/Renderer.h"
#include <iostream>

namespace PathTracingVk {
static void FrameBufferResizeCallback(GLFWwindow* window, int width, int height)
{
    // explicit resize
}

void VulkanWindow::GlfwErrorCallback(const int error, const char* const description)
{
    std::cerr << "[ERROR] GLFW: " << description << " (code: " << error << ")" << std::endl;
}

void VulkanWindow::GlfwKeyCallback(GLFWwindow* window, int key, int scancode, int action, int mods) {
    auto* const this_ = static_cast<VulkanWindow*>(glfwGetWindowUserPointer(window));
    for (auto& callback : this_->onKeyChanged)
        callback(key, scancode, action, mods);
}

void VulkanWindow::GlfwCursorPositionCallback(GLFWwindow* window, const double xpos, const double ypos)
{
    auto* const this_ = static_cast<VulkanWindow*>(glfwGetWindowUserPointer(window));
    for (auto& callback : this_->onCursorPositionChanged)
        callback(xpos, ypos);
}

void VulkanWindow::GlfwMouseButtonCallback(GLFWwindow* window, const int button, const int action, const int mods)
{
    auto* const this_ = static_cast<VulkanWindow*>(glfwGetWindowUserPointer(window));
    for (auto& callback : this_->onMouseButtonChanged)
        callback(button, action, mods);
}

void VulkanWindow::GlfwScrollCallback(GLFWwindow* window, const double xoffset, const double yoffset)
{
    auto* const this_ = static_cast<VulkanWindow*>(glfwGetWindowUserPointer(window));
    for (auto& callback : this_->onScrollChanged)
        callback(xoffset, yoffset);
}

VulkanWindow::VulkanWindow(int width, int height, const char* pName) : m_width(width), m_height(height), m_pName(pName) {
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

VulkanWindow::~VulkanWindow() {
    glfwDestroyWindow(m_window);
    glfwTerminate();
}

}