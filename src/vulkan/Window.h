#ifndef VK_RAYTRACING_WINDOW_H
#define VK_RAYTRACING_WINDOW_H

#include "Vulkan.h"
#include <functional>

namespace ptvk {

class Window {
public:
    Window(int width, int height, const char* pName);
    ~Window();

    GLFWwindow* getWindow() const { return m_window; }

    void run();
    void addOnKeyChanged(std::function<void(int key, int scancode, int action, int mods)> callback);
    void addOnCursorPositionChanged(std::function<void(double xpos, double ypos)> callback);
    void addOnMouseButtonChanged(std::function<void(int button, int action, int mods)> callback);
    void addOnScrollChanged(std::function<void(double xoffset, double yoffset)> callback);
    void addOnFramebufferSizeChanged(std::function<void(int width, int height)> callback);

private:
    GLFWwindow* m_window{};
    int         m_width{};
    int         m_height{};
    const char* m_pName{};

    std::vector<std::function<void(int key, int scancode, int action, int mods)>> onKeyChanged;
    std::vector<std::function<void(double xPos, double yPos)>>                    onCursorPositionChanged;
    std::vector<std::function<void(int button, int action, int mods)>>            onMouseButtonChanged;
    std::vector<std::function<void(double xOffset, double yOffset)>>              onScrollChanged;
    std::vector<std::function<void(int width, int height)>>                       onFramebufferSizeChanged;

    static void glfwErrorCallback(int error, const char* description);
    static void glfwKeyCallback(GLFWwindow* window, int key, int scancode, int action, int mods);
    static void glfwCursorPositionCallback(GLFWwindow* window, double xpos, double ypos);
    static void glfwMouseButtonCallback(GLFWwindow* window, int button, int action, int mods);
    static void glfwScrollCallback(GLFWwindow* window, double xOffset, double yOffset);
    static void glfwFramebufferSizeCallback(GLFWwindow* window, int width, int height);
};

}

#endif //VK_RAYTRACING_WINDOW_H