#include "renderer/Camera.h"
#include <algorithm>
#include <cmath>

namespace ps {

Camera::Camera() {
    recalculateVectors();
}

void Camera::update(float /*deltaTime*/) {
    // Apply accumulated movement
    m_position += m_velocity;
    m_velocity = glm::vec3(0.0f);

    // Clamp pitch
    m_pitch = std::clamp(m_pitch, -89.0f, 89.0f);

    recalculateVectors();
}

void Camera::moveForward(float amount) {
    // Move along the front vector (horizontal only, no vertical for FPS)
    glm::vec3 frontHorizontal = glm::normalize(
        glm::vec3(m_front.x, 0.0f, m_front.z)
    );
    m_velocity += frontHorizontal * amount * m_moveSpeed;
}

void Camera::moveRight(float amount) {
    m_velocity += m_right * amount * m_moveSpeed;
}

void Camera::moveUp(float amount) {
    m_velocity += m_worldUp * amount * m_moveSpeed;
}

void Camera::look(float xOffset, float yOffset) {
    m_yaw += xOffset * m_sensitivity;
    m_pitch += (m_invertY ? -yOffset : yOffset) * m_sensitivity;
}

glm::mat4 Camera::viewMatrix() const {
    return glm::lookAt(m_position, m_position + m_front, m_up);
}

glm::mat4 Camera::projectionMatrix(float aspectRatio) const {
    return glm::perspective(glm::radians(m_fov), aspectRatio, m_nearPlane, m_farPlane);
}

void Camera::recalculateVectors() {
    glm::vec3 front;
    front.x = std::cos(glm::radians(m_yaw)) * std::cos(glm::radians(m_pitch));
    front.y = std::sin(glm::radians(m_pitch));
    front.z = std::sin(glm::radians(m_yaw)) * std::cos(glm::radians(m_pitch));
    m_front = glm::normalize(front);

    m_right = glm::normalize(glm::cross(m_front, m_worldUp));
    m_up = glm::normalize(glm::cross(m_right, m_front));
}

} // namespace ps
