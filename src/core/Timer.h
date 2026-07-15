#pragma once

#include <GLFW/glfw3.h>

namespace ps {

class Timer {
public:
    Timer();

    // Call once per frame to update delta time
    void tick();

    // Get time since last tick (seconds)
    float deltaTime() const { return m_deltaTime; }

    // Get unscaled delta time (before clamping)
    float rawDeltaTime() const { return m_rawDeltaTime; }

    // Get total elapsed time since start
    double elapsed() const;

    // FPS counter
    float fps() const { return m_fps; }
    int frameCount() const { return m_frameCount; }

    // Delta time clamping (prevents spiral of death)
    void setMaxDeltaTime(float maxDt) { m_maxDeltaTime = maxDt; }

    // Time scale (1.0 = normal, 0.5 = slow-mo, 2.0 = fast-forward)
    void setTimeScale(float scale) { m_timeScale = scale; }
    float timeScale() const { return m_timeScale; }

private:
    double m_lastTime = 0.0;
    float m_deltaTime = 0.0f;
    float m_rawDeltaTime = 0.0f;
    float m_maxDeltaTime = 0.1f;  // Clamp to 100ms max
    float m_timeScale = 1.0f;

    // FPS tracking
    int m_frameCount = 0;
    double m_fpsTimer = 0.0;
    float m_fps = 0.0f;
};

} // namespace ps
