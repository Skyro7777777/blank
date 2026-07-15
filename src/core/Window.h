#pragma once

#include <GLFW/glfw3.h>
#include <string>

namespace ps {

class Window {
public:
    struct Config {
        int width = 1280;
        int height = 720;
        std::string title = "Phantom Strike";
        bool fullscreen = false;
        bool vsync = true;
        int msaaSamples = 4;
    };

    Window();
    ~Window();

    // Initialize the window with given config
    bool create(const Config& config);

    // Destroy the window
    void destroy();

    // Check if window should close
    bool shouldClose() const;

    // Swap buffers
    void swapBuffers();

    // Get window dimensions
    int width() const { return m_width; }
    int height() const { return m_height; }
    float aspectRatio() const;

    // Get framebuffer dimensions (may differ from window on HiDPI)
    int framebufferWidth() const;
    int framebufferHeight() const;

    // Window state
    bool isMinimized() const;
    bool isFocused() const;
    bool wasResized() const { return m_resized; }
    void clearResizedFlag() { m_resized = false; }

    // Cursor control
    void captureCursor(bool capture);
    bool isCursorCaptured() const { return m_cursorCaptured; }

    // VSync
    void setVSync(bool enabled);

    // Get the raw GLFW window pointer
    GLFWwindow* handle() const { return m_window; }

    // Set window title
    void setTitle(const std::string& title);

    // Poll current window size (call each frame to detect resizes)
    void pollSize();

private:
    static void framebufferSizeCallback(GLFWwindow* window, int width, int height);
    static void errorCallback(int code, const char* description);

    GLFWwindow* m_window = nullptr;
    int m_width = 0;
    int m_height = 0;
    bool m_resized = false;
    bool m_cursorCaptured = false;
    bool m_initialized = false;
};

} // namespace ps
