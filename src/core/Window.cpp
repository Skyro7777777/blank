#include "core/Window.h"
#include "core/Logger.h"

namespace ps {

Window::Window() = default;

Window::~Window() {
    destroy();
}

bool Window::create(const Config& config) {
    glfwSetErrorCallback(errorCallback);

    if (!glfwInit()) {
        LOG_FATAL("Failed to initialize GLFW");
        return false;
    }

    // Window hints
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GLFW_TRUE);
    glfwWindowHint(GLFW_RESIZABLE, GLFW_TRUE);
    glfwWindowHint(GLFW_SAMPLES, config.msaaSamples);

    m_width = config.width;
    m_height = config.height;

    // Try OpenGL versions from 4.6 down to 3.3
    struct GLVersion { int major; int minor; };
    GLVersion versions[] = {{4,6},{4,5},{4,4},{4,3},{4,2},{4,1},{4,0},{3,3}};

    GLFWmonitor* monitor = config.fullscreen ? glfwGetPrimaryMonitor() : nullptr;

    for (const auto& ver : versions) {
        glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, ver.major);
        glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, ver.minor);
        m_window = glfwCreateWindow(m_width, m_height, config.title.c_str(), monitor, nullptr);
        if (m_window) {
            LOG_INFO("Created OpenGL %d.%d context", ver.major, ver.minor);
            break;
        }
        LOG_WARN("OpenGL %d.%d not available, trying lower...", ver.major, ver.minor);
    }

    if (!m_window) {
        LOG_FATAL("Failed to create GLFW window with any OpenGL version");
        glfwTerminate();
        return false;
    }

    glfwMakeContextCurrent(m_window);
    glfwSetFramebufferSizeCallback(m_window, framebufferSizeCallback);

    // Note: glfwSetWindowUserPointer is NOT used here.
    // The Engine class manages user pointer for Input callbacks.

    // Store actual size
    glfwGetFramebufferSize(m_window, &m_width, &m_height);

    // VSync
    glfwSwapInterval(config.vsync ? 1 : 0);

    // Enable raw mouse motion
    if (glfwRawMouseMotionSupported()) {
        glfwSetInputMode(m_window, GLFW_RAW_MOUSE_MOTION, GLFW_TRUE);
    }

    m_initialized = true;

    LOG_INFO("Window created: %dx%d", m_width, m_height);
    return true;
}

void Window::destroy() {
    if (m_window) {
        glfwDestroyWindow(m_window);
        m_window = nullptr;
    }
    if (m_initialized) {
        glfwTerminate();
        m_initialized = false;
    }
}

bool Window::shouldClose() const {
    return m_window && glfwWindowShouldClose(m_window);
}

void Window::swapBuffers() {
    if (m_window) glfwSwapBuffers(m_window);
}

float Window::aspectRatio() const {
    if (m_height == 0) return 1.0f;
    return static_cast<float>(m_width) / static_cast<float>(m_height);
}

int Window::framebufferWidth() const {
    int w, h;
    glfwGetFramebufferSize(m_window, &w, &h);
    return w;
}

int Window::framebufferHeight() const {
    int w, h;
    glfwGetFramebufferSize(m_window, &w, &h);
    return h;
}

bool Window::isMinimized() const {
    return glfwGetWindowAttrib(m_window, GLFW_ICONIFIED) == GLFW_TRUE;
}

bool Window::isFocused() const {
    return glfwGetWindowAttrib(m_window, GLFW_FOCUSED) == GLFW_TRUE;
}

void Window::captureCursor(bool capture) {
    m_cursorCaptured = capture;
    if (m_window) {
        glfwSetInputMode(m_window, GLFW_CURSOR,
            capture ? GLFW_CURSOR_DISABLED : GLFW_CURSOR_NORMAL);
    }
}

void Window::setVSync(bool enabled) {
    glfwSwapInterval(enabled ? 1 : 0);
}

void Window::setTitle(const std::string& title) {
    if (m_window) glfwSetWindowTitle(m_window, title.c_str());
}

void Window::pollSize() {
    int w, h;
    glfwGetWindowSize(m_window, &w, &h);
    if (w != m_width || h != m_height) {
        m_width = w;
        m_height = h;
        m_resized = true;
    }
}

void Window::framebufferSizeCallback(GLFWwindow* window, int width, int height) {
    // Simple callback - just update viewport.
    // Engine handles window resize tracking via glfwGetWindowSize.
    glViewport(0, 0, width, height);
}

void Window::errorCallback(int code, const char* description) {
    LOG_ERROR("GLFW Error %d: %s", code, description);
}

} // namespace ps
