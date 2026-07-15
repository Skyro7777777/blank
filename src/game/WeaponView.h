#pragma once

#include "game/WeaponSystem.h"
#include "renderer/Camera.h"
#include "renderer/Shader.h"
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

namespace ps {

class WeaponView {
public:
    WeaponView() = default;

    void init();

    // Update weapon view each frame
    void update(float dt, const Camera& camera, const WeaponSystem& weapons, bool isMoving);

    // Render the weapon model in first person
    void render(const Shader& shader, const glm::mat4& view, const glm::mat4& projection);

    // Bob and sway parameters
    void setBobIntensity(float intensity) { m_bobIntensity = intensity; }
    void setSwayIntensity(float intensity) { m_swayIntensity = intensity; }

    // Compute the view-model matrix for the weapon
    glm::mat4 computeViewModel(const Camera& camera, const WeaponSystem& weapons) const;

private:

    // Bob
    float m_bobTimer = 0.0f;
    float m_bobIntensity = 0.003f;
    float m_bobSpeed = 8.0f;

    // Sway (follows mouse movement with lag)
    glm::vec2 m_swayOffset{0.0f};
    float m_swayIntensity = 0.002f;
    float m_swaySpeed = 5.0f;

    // Previous camera angles for sway calculation
    float m_prevPitch = 0.0f;
    float m_prevYaw = 0.0f;

    // Recoil visual kick
    float m_recoilKick = 0.0f;
    float m_recoilKickSpeed = 15.0f;

    // ADS interpolation
    float m_adsSmooth = 0.0f;
};

} // namespace ps
