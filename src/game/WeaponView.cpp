#include "game/WeaponView.h"
#include "core/Logger.h"
#include <cmath>

namespace ps {

void WeaponView::init() {
    LOG_INFO("Weapon view initialized");
}

void WeaponView::update(float dt, const Camera& camera, const WeaponSystem& weapons, bool isMoving) {
    // Bob timer (only when moving)
    if (isMoving) {
        m_bobTimer += dt * m_bobSpeed;
    } else {
        // Slowly reset bob
        m_bobTimer += dt * 0.5f;
    }

    // Sway (lag behind camera rotation)
    float pitchDelta = camera.pitch() - m_prevPitch;
    float yawDelta = camera.yaw() - m_prevYaw;
    m_prevPitch = camera.pitch();
    m_prevYaw = camera.yaw();

    glm::vec2 targetSway(yawDelta * m_swayIntensity, pitchDelta * m_swayIntensity);
    m_swayOffset += (targetSway - m_swayOffset) * m_swaySpeed * dt;

    // Clamp sway
    float maxSway = 0.05f;
    m_swayOffset.x = std::clamp(m_swayOffset.x, -maxSway, maxSway);
    m_swayOffset.y = std::clamp(m_swayOffset.y, -maxSway, maxSway);

    // Recoil kick (visual Z pushback)
    glm::vec2 recoil = weapons.recoilOffset();
    if (recoil.x > 0.5f) {
        m_recoilKick = 0.05f; // Push back on fire
    }
    m_recoilKick = std::max(0.0f, m_recoilKick - dt * m_recoilKickSpeed);

    // Smooth ADS
    m_adsSmooth += (weapons.adsBlend() - m_adsSmooth) * 10.0f * dt;
}

glm::mat4 WeaponView::computeViewModel(const Camera& camera, const WeaponSystem& weapons) const {
    const auto* w = weapons.currentWeapon();
    if (!w) return glm::mat4(1.0f);

    // Start with camera view
    glm::mat4 view = camera.viewMatrix();

    // Invert to get camera world transform, then apply weapon offset
    glm::mat4 camWorld = glm::inverse(view);

    // Weapon offset (lerp between hip and ADS)
    glm::vec3 offset = glm::mix(w->stats.viewOffset, w->stats.adsOffset, m_adsSmooth);

    // Add bob
    float bobX = 0.0f, bobY = 0.0f;
    if (!weapons.isADS()) {
        bobX = std::sin(m_bobTimer) * m_bobIntensity;
        bobY = std::abs(std::cos(m_bobTimer)) * m_bobIntensity;
    }

    // Add sway
    float swayX = m_swayOffset.x;
    float swayY = m_swayOffset.y;

    // Add recoil kick
    float recoilZ = m_recoilKick;

    // Apply offsets in camera space
    glm::mat4 weaponOffset = glm::translate(glm::mat4(1.0f),
        offset + glm::vec3(bobX + swayX, bobY + swayY, recoilZ));

    // Scale
    float scale = w->stats.viewScale;
    weaponOffset = glm::scale(weaponOffset, glm::vec3(scale));

    // Rotate model to face forward (GLB models typically point +X, need -Z)
    float rotY = glm::radians(w->stats.viewRotateY);
    glm::mat4 modelRot = glm::rotate(glm::mat4(1.0f), rotY, glm::vec3(0.0f, 1.0f, 0.0f));

    return view * camWorld * weaponOffset * modelRot;
}

void WeaponView::render(const Shader& shader, const glm::mat4& view, const glm::mat4& projection) {
    // The actual model rendering is done in Engine::renderScene using this view model matrix
    // This is a placeholder - the Engine will call computeViewModel and set the model matrix
}

} // namespace ps
