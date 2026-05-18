#ifndef VK_RAYTRACING_WINDOW_H
#define VK_RAYTRACING_WINDOW_H

#include "Vulkan.h"
#include <functional>

namespace ptvk {

class Window {
public:
    Window(int width, int height, const char* pName);
    ~Window();

    void Run();
    void AddOnKeyChanged(std::function<void(int key, int scancode, int action, int mods)> callback);
    void AddOnCursorPositionChanged(std::function<void(double xpos, double ypos)> callback);
    void AddOnMouseButtonChanged(std::function<void(int button, int action, int mods)> callback);
    void AddOnScrollChanged(std::function<void(double xoffset, double yoffset)> callback);

    [[nodiscard]] GLFWwindow* GetWindow() const {
        return m_window;
    }
private:
    GLFWwindow* m_window{};
    int m_width{};
    int m_height{};
    const char* m_pName{};

    std::vector<std::function<void(int key, int scancode, int action, int mods)>> onKeyChanged;
    std::vector<std::function<void(double xPos, double yPos)>> onCursorPositionChanged;
    std::vector<std::function<void(int button, int action, int mods)>> onMouseButtonChanged;
    std::vector<std::function<void(double xOffset, double yOffset)>> onScrollChanged;

    static void GlfwErrorCallback(int error, const char* description);
    static void GlfwKeyCallback(GLFWwindow* window, int key, int scancode, int action, int mods);
    static void GlfwCursorPositionCallback(GLFWwindow* window, double xpos, double ypos);
    static void GlfwMouseButtonCallback(GLFWwindow* window, int button, int action, int mods);
    static void GlfwScrollCallback(GLFWwindow* window, double xOffset, double yOffset);
};

}

#endif //VK_RAYTRACING_WINDOW_H