#pragma once

#include "game/Weapon.h"
#include "physics/PhysicsWorld.h"
#include "renderer/Model.h"
#include <glm/glm.hpp>
#include <vector>
#include <string>
#include <memory>
#include <functional>

namespace ps {

struct HitResult {
    bool hit = false;
    glm::vec3 hitPoint{0.0f};
    glm::vec3 hitNormal{0.0f};
    float distance = 0.0f;
    btRigidBody* body = nullptr;
    bool isHeadshot = false; // Placeholder for future hitbox system
};

using HitCallback = std::function<void(const HitResult&)>;

struct WeaponSlot {
    WeaponStats stats;
    std::unique_ptr<Model> model;
    std::string modelPath;
    int currentMag = 0;
    int reserveAmmo = 0;
    bool loaded = false;
};

class WeaponSystem {
public:
    WeaponSystem() = default;
    ~WeaponSystem() = default;

    // Initialize with physics world reference
    void init(PhysicsWorld* world);

    // Add a weapon to inventory
    int addWeapon(const WeaponStats& stats, const std::string& modelPath);

    // Update (called each frame)
    void update(float dt);

    // Actions
    bool tryFire(const glm::vec3& origin, const glm::vec3& direction);
    bool tryReload();
    void startADS();
    void stopADS();

    // Weapon switching
    void switchTo(int slotIndex);
    void switchNext();
    void switchPrev();

    // Set hit callback
    void setHitCallback(HitCallback cb) { m_hitCallback = cb; }

    // State queries
    const WeaponSlot* currentWeapon() const;
    int currentSlot() const { return m_currentSlot; }
    int weaponCount() const { return (int)m_slots.size(); }
    bool isFiring() const { return m_firing; }
    bool isReloading() const { return m_reloading; }
    bool isADS() const { return m_adsActive; }
    float adsBlend() const { return m_adsBlend; }
    float reloadProgress() const { return m_reloadTimer / (m_reloadDuration > 0 ? m_reloadDuration : 1.0f); }

    // Recoil state (applied to camera)
    glm::vec2 recoilOffset() const { return m_recoilOffset; }
    void consumeRecovery(float dt);

    // Muzzle flash state
    bool showMuzzleFlash() const { return m_muzzleFlashTimer > 0.0f; }
    float muzzleFlashIntensity() const { return m_muzzleFlashTimer / 0.05f; }

    // Model for current weapon (for rendering)
    Model* currentModel();

private:
    void fireRaycast(const glm::vec3& origin, const glm::vec3& direction);
    void applyRecoil();

    std::vector<WeaponSlot> m_slots;
    PhysicsWorld* m_world = nullptr;
    HitCallback m_hitCallback;

    int m_currentSlot = -1;
    float m_fireTimer = 0.0f;
    float m_reloadTimer = 0.0f;
    float m_reloadDuration = 0.0f;
    bool m_reloading = false;
    bool m_firing = false;
    bool m_adsActive = false;
    float m_adsBlend = 0.0f;  // 0 = hip, 1 = ADS

    // Recoil
    glm::vec2 m_recoilOffset{0.0f}; // pitch, yaw offsets in degrees
    float m_recoilPattern = 0.0f;   // Accumulated shots for pattern

    // Muzzle flash
    float m_muzzleFlashTimer = 0.0f;
};

} // namespace ps
