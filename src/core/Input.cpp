#include "core/Input.h"

namespace ps {

void Input::registerCallbacks(GLFWwindow* window) {
    // IMPORTANT: glfwSetWindowUserPointer must already point to this Input instance
    // before calling this function (done by Engine).
    glfwSetKeyCallback(window, keyCallback);
    glfwSetMouseButtonCallback(window, mouseButtonCallback);
    glfwSetCursorPosCallback(window, cursorPosCallback);
    glfwSetScrollCallback(window, scrollCallback);
    m_window = window;

    // Initialize mouse position
    double xpos, ypos;
    glfwGetCursorPos(window, &xpos, &ypos);
    m_mousePos = glm::vec2(static_cast<float>(xpos), static_cast<float>(ypos));
    m_lastMousePos = m_mousePos;
    m_pendingMousePos = m_mousePos;
}

void Input::update() {
    // Compute mouse delta from pending position vs last known position
    if (m_firstMouse) {
        m_lastMousePos = m_pendingMousePos;
        m_mousePos = m_pendingMousePos;
        m_firstMouse = false;
    }
    m_mouseDelta = m_pendingMousePos - m_lastMousePos;
    m_lastMousePos = m_mousePos;
    m_mousePos = m_pendingMousePos;
    m_scrollDelta = m_pendingScroll;
    m_pendingScroll = 0.0f;

    // Decay just-pressed/released states
    for (auto& [key, state] : m_keyState) {
        if (state == 2) state = 1;      // just_pressed -> pressed
        else if (state == 3) state = 0;  // just_released -> released
    }
    for (auto& [btn, state] : m_mouseButtonState) {
        if (state == 2) state = 1;
        else if (state == 3) state = 0;
    }
}

// ── Keyboard ──
bool Input::isKeyPressed(int key) const {
    auto it = m_keyState.find(key);
    return it != m_keyState.end() && (it->second == 1 || it->second == 2);
}

bool Input::isKeyJustPressed(int key) const {
    auto it = m_keyState.find(key);
    return it != m_keyState.end() && it->second == 2;
}

bool Input::isKeyJustReleased(int key) const {
    auto it = m_keyState.find(key);
    return it != m_keyState.end() && it->second == 3;
}

// ── Mouse Buttons ──
bool Input::isMouseButtonPressed(int button) const {
    auto it = m_mouseButtonState.find(button);
    return it != m_mouseButtonState.end() && (it->second == 1 || it->second == 2);
}

bool Input::isMouseButtonJustPressed(int button) const {
    auto it = m_mouseButtonState.find(button);
    return it != m_mouseButtonState.end() && it->second == 2;
}

bool Input::isMouseButtonJustReleased(int button) const {
    auto it = m_mouseButtonState.find(button);
    return it != m_mouseButtonState.end() && it->second == 3;
}

// ── Cursor ──
void Input::setCursorCaptured(bool captured) {
    if (m_window) {
        glfwSetInputMode(m_window, GLFW_CURSOR,
            captured ? GLFW_CURSOR_DISABLED : GLFW_CURSOR_NORMAL);
        // Reset mouse tracking to avoid jump when cursor is captured/released
        double xpos, ypos;
        glfwGetCursorPos(m_window, &xpos, &ypos);
        m_pendingMousePos = glm::vec2(static_cast<float>(xpos), static_cast<float>(ypos));
        m_mousePos = m_pendingMousePos;
        m_lastMousePos = m_pendingMousePos;
        m_firstMouse = true;
    }
}

bool Input::isCursorCaptured() const {
    if (!m_window) return false;
    return glfwGetInputMode(m_window, GLFW_CURSOR) == GLFW_CURSOR_DISABLED;
}

// ── Callbacks ──
void Input::keyCallback(GLFWwindow* window, int key, int /*scancode*/, int action, int /*mods*/) {
    auto* input = static_cast<Input*>(glfwGetWindowUserPointer(window));
    if (!input) return;

    if (action == GLFW_PRESS) {
        input->m_keyState[key] = 2;  // just_pressed
    } else if (action == GLFW_RELEASE) {
        input->m_keyState[key] = 3;  // just_released
    }
    // GLFW_REPEAT is ignored - use isKeyPressed for held keys
}

void Input::mouseButtonCallback(GLFWwindow* window, int button, int action, int /*mods*/) {
    auto* input = static_cast<Input*>(glfwGetWindowUserPointer(window));
    if (!input) return;

    if (action == GLFW_PRESS) {
        input->m_mouseButtonState[button] = 2;
    } else if (action == GLFW_RELEASE) {
        input->m_mouseButtonState[button] = 3;
    }
}

void Input::cursorPosCallback(GLFWwindow* window, double xpos, double ypos) {
    auto* input = static_cast<Input*>(glfwGetWindowUserPointer(window));
    if (!input) return;

    input->m_pendingMousePos = glm::vec2(static_cast<float>(xpos), static_cast<float>(ypos));
}

void Input::scrollCallback(GLFWwindow* window, double /*xoffset*/, double yoffset) {
    auto* input = static_cast<Input*>(glfwGetWindowUserPointer(window));
    if (!input) return;

    input->m_pendingScroll += static_cast<float>(yoffset);
}

} // namespace ps
