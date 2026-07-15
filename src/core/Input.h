#pragma once

#include <GLFW/glfw3.h>
#include <glm/glm.hpp>
#include <unordered_map>

namespace ps {

class Input {
public:
    // Register input callbacks on the window (window user pointer must point to Input)
    void registerCallbacks(GLFWwindow* window);

    // Call once per frame to update input state
    void update();

    // ── Keyboard ──
    bool isKeyPressed(int key) const;
    bool isKeyJustPressed(int key) const;
    bool isKeyJustReleased(int key) const;

    // ── Mouse Buttons ──
    bool isMouseButtonPressed(int button) const;
    bool isMouseButtonJustPressed(int button) const;
    bool isMouseButtonJustReleased(int button) const;

    // ── Mouse Position ──
    glm::vec2 mousePosition() const { return m_mousePos; }
    glm::vec2 mouseDelta() const { return m_mouseDelta; }
    float scrollDelta() const { return m_scrollDelta; }

    // ── Cursor ──
    void setCursorCaptured(bool captured);
    bool isCursorCaptured() const;

    // ── Convenience: Common keys ──
    bool isEscapePressed() const { return isKeyPressed(GLFW_KEY_ESCAPE); }
    bool isTabPressed() const { return isKeyPressed(GLFW_KEY_TAB); }

private:
    static void keyCallback(GLFWwindow* window, int key, int scancode, int action, int mods);
    static void mouseButtonCallback(GLFWwindow* window, int button, int action, int mods);
    static void cursorPosCallback(GLFWwindow* window, double xpos, double ypos);
    static void scrollCallback(GLFWwindow* window, double xoffset, double yoffset);

    GLFWwindow* m_window = nullptr;

    // Key state: 0=released, 1=pressed, 2=just_pressed, 3=just_released
    std::unordered_map<int, int> m_keyState;
    std::unordered_map<int, int> m_mouseButtonState;

    glm::vec2 m_mousePos{0.0f};
    glm::vec2 m_lastMousePos{0.0f};
    glm::vec2 m_mouseDelta{0.0f};
    float m_scrollDelta = 0.0f;
    bool m_firstMouse = true;

    // Raw pending state (set by callbacks, consumed in update())
    glm::vec2 m_pendingMousePos{0.0f};
    float m_pendingScroll = 0.0f;
};

} // namespace ps
