#include "game/Flashlight.h"
#include <algorithm>

namespace ps {

void Flashlight::update(const glm::vec3& position, const glm::vec3& direction) {
    m_position = position;
    m_direction = glm::normalize(direction);
}

void Flashlight::updateBattery(float dt) {
    if (m_on && m_drainBattery) {
        m_battery -= m_batteryDrainRate * dt;
        m_battery = std::max(0.0f, m_battery);
        if (m_battery <= 0.0f) {
            m_on = false; // Auto-off when dead
        }
    }
}

} // namespace ps
