#pragma once

#include <glm/glm.hpp>

namespace ps {

class Flashlight {
public:
    Flashlight() = default;

    // Toggle on/off
    void toggle() { m_on = !m_on; }
    void setOn(bool on) { m_on = on; }
    bool isOn() const { return m_on; }

    // Update position/direction (follows camera)
    void update(const glm::vec3& position, const glm::vec3& direction);

    // Properties
    glm::vec3 position() const { return m_position; }
    glm::vec3 direction() const { return m_direction; }
    glm::vec3 color() const { return m_color; }
    float intensity() const { return m_on ? m_intensity : 0.0f; }
    float innerCone() const { return m_innerCone; }
    float outerCone() const { return m_outerCone; }
    float range() const { return m_range; }

    // Battery (optional gameplay feature)
    float battery() const { return m_battery; }
    void setBatteryDrain(bool drain) { m_drainBattery = drain; }
    void updateBattery(float dt);
    void recharge() { m_battery = 100.0f; }

    // Settings
    float m_intensity = 15.0f;
    float m_innerCone = 12.5f;   // Degrees
    float m_outerCone = 22.0f;   // Degrees
    float m_range = 50.0f;
    glm::vec3 m_color{1.0f, 0.95f, 0.85f}; // Warm white

private:
    glm::vec3 m_position{0.0f};
    glm::vec3 m_direction{0.0f, 0.0f, -1.0f};
    bool m_on = false;
    float m_battery = 100.0f;
    bool m_drainBattery = false;
    float m_batteryDrainRate = 2.0f; // % per second
};

} // namespace ps
