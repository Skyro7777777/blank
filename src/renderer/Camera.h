#pragma once

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

namespace ps {

class Camera {
public:
    Camera();

    // Update camera each frame
    void update(float deltaTime);

    // ── Movement (called from player/input system) ──
    void moveForward(float amount);
    void moveRight(float amount);
    void moveUp(float amount);

    // ── Look (called from mouse input) ──
    void look(float xOffset, float yOffset);

    // ── Getters ──
    glm::vec3 position() const { return m_position; }
    glm::vec3 front() const { return m_front; }
    glm::vec3 right() const { return m_right; }
    glm::vec3 up() const { return m_up; }
    float pitch() const { return m_pitch; }
    float yaw() const { return m_yaw; }
    float fov() const { return m_fov; }

    // ── Setters ──
    void setPosition(const glm::vec3& pos) { m_position = pos; }
    void setFov(float fov) { m_fov = fov; }

    // ── Matrices ──
    glm::mat4 viewMatrix() const;
    glm::mat4 projectionMatrix(float aspectRatio) const;

    // ── Mouse sensitivity ──
    void setSensitivity(float sensitivity) { m_sensitivity = sensitivity; }
    float sensitivity() const { return m_sensitivity; }

    // ── Movement speed ──
    void setMoveSpeed(float speed) { m_moveSpeed = speed; }
    float moveSpeed() const { return m_moveSpeed; }

    // ── Invert Y axis ──
    void setInvertY(bool invert) { m_invertY = invert; }
    bool invertY() const { return m_invertY; }

private:
    void recalculateVectors();

    // Position and orientation
    glm::vec3 m_position{0.0f, 1.6f, 3.0f};  // Eye height ~1.6m
    glm::vec3 m_front{0.0f, 0.0f, -1.0f};
    glm::vec3 m_right{1.0f, 0.0f, 0.0f};
    glm::vec3 m_up{0.0f, 1.0f, 0.0f};
    glm::vec3 m_worldUp{0.0f, 1.0f, 0.0f};

    // Euler angles
    float m_pitch = 0.0f;   // Up/down
    float m_yaw = -90.0f;   // Left/right (starts looking at -Z)

    // Settings
    float m_fov = 70.0f;
    float m_nearPlane = 0.1f;
    float m_farPlane = 1000.0f;
    float m_sensitivity = 0.1f;
    float m_moveSpeed = 5.0f;
    bool m_invertY = false;

    // Accumulated movement (applied in update)
    glm::vec3 m_velocity{0.0f};
};

} // namespace ps
