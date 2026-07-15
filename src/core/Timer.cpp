#include "core/Timer.h"
#include <algorithm>

namespace ps {

Timer::Timer() {
    m_lastTime = glfwGetTime();
}

void Timer::tick() {
    double currentTime = glfwGetTime();
    m_rawDeltaTime = static_cast<float>(currentTime - m_lastTime);
    m_lastTime = currentTime;

    // Clamp delta time to prevent spiral of death
    float clampedDt = std::min(m_rawDeltaTime, m_maxDeltaTime);

    // Apply time scale
    m_deltaTime = clampedDt * m_timeScale;

    // FPS calculation
    m_frameCount++;
    m_fpsTimer += m_rawDeltaTime;
    if (m_fpsTimer >= 1.0) {
        m_fps = static_cast<float>(m_frameCount / m_fpsTimer);
        m_frameCount = 0;
        m_fpsTimer = 0.0;
    }
}

double Timer::elapsed() const {
    return glfwGetTime();
}

} // namespace ps
