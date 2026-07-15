#include "game/WeaponSystem.h"
#include "core/Logger.h"
#include "physics/CollisionShapes.h"
#include <cstdlib>
#include <cmath>
#include <algorithm>

namespace ps {

void WeaponSystem::init(PhysicsWorld* world) {
    m_world = world;
    LOG_INFO("Weapon system initialized");
}

int WeaponSystem::addWeapon(const WeaponStats& stats, const std::string& modelPath) {
    WeaponSlot slot;
    slot.stats = stats;
    slot.modelPath = modelPath;
    slot.currentMag = stats.magSize;
    slot.reserveAmmo = stats.reserveAmmo;
    slot.model = std::make_unique<Model>();

    if (!modelPath.empty() && stats.category != WeaponCategory::MELEE) {
        if (slot.model->load(modelPath)) {
            slot.loaded = true;
            LOG_INFO("Weapon loaded: %s (%zu meshes)", stats.name.c_str(), slot.model->meshes().size());
        } else {
            LOG_WARN("Weapon model failed: %s", modelPath.c_str());
        }
    } else if (stats.category == WeaponCategory::MELEE) {
        if (!modelPath.empty() && slot.model->load(modelPath)) {
            slot.loaded = true;
            LOG_INFO("Melee weapon loaded: %s", stats.name.c_str());
        }
    }

    m_slots.push_back(std::move(slot));
    if (m_currentSlot < 0) m_currentSlot = 0;
    return (int)m_slots.size() - 1;
}

void WeaponSystem::update(float dt) {
    // Fire cooldown
    if (m_fireTimer > 0.0f) m_fireTimer -= dt;

    // Reload timer
    if (m_reloading) {
        m_reloadTimer += dt;
        if (m_reloadTimer >= m_reloadDuration) {
            // Finish reload
            auto* w = const_cast<WeaponSlot*>(currentWeapon());
            if (w) {
                int needed = w->stats.magSize - w->currentMag;
                int available = std::min(needed, w->reserveAmmo);
                w->currentMag += available;
                w->reserveAmmo -= available;
            }
            m_reloading = false;
            m_reloadTimer = 0.0f;
        }
    }

    // ADS transition
    float adsSpeed = 1.0f / (currentWeapon() ? currentWeapon()->stats.adsTransitionTime : 0.2f);
    if (m_adsActive) {
        m_adsBlend = std::min(m_adsBlend + adsSpeed * dt, 1.0f);
    } else {
        m_adsBlend = std::max(m_adsBlend - adsSpeed * dt, 0.0f);
    }

    // Muzzle flash decay
    if (m_muzzleFlashTimer > 0.0f) {
        m_muzzleFlashTimer -= dt;
        if (m_muzzleFlashTimer < 0.0f) m_muzzleFlashTimer = 0.0f;
    }

    // Recoil recovery
    consumeRecovery(dt);
}

bool WeaponSystem::tryFire(const glm::vec3& origin, const glm::vec3& direction) {
    if (m_currentSlot < 0 || m_reloading) return false;

    auto* w = &m_slots[m_currentSlot];
    if (w->stats.category == WeaponCategory::MELEE) {
        // Melee: no ammo needed
        if (m_fireTimer > 0.0f) return false;
        m_fireTimer = w->stats.fireRate;
        fireRaycast(origin, direction);
        return true;
    }

    if (m_fireTimer > 0.0f) return false;
    if (w->currentMag <= 0) {
        // Auto-reload when empty
        tryReload();
        return false;
    }

    w->currentMag--;
    m_fireTimer = w->stats.fireRate;
    m_firing = true;

    // Muzzle flash
    m_muzzleFlashTimer = w->stats.muzzleFlashDuration;

    // Apply recoil
    applyRecoil();

    // Fire raycast
    fireRaycast(origin, direction);

    return true;
}

bool WeaponSystem::tryReload() {
    if (m_currentSlot < 0 || m_reloading) return false;

    auto* w = &m_slots[m_currentSlot];
    if (w->stats.category == WeaponCategory::MELEE) return false;
    if (w->currentMag >= w->stats.magSize) return false;
    if (w->reserveAmmo <= 0) return false;

    m_reloading = true;
    m_reloadTimer = 0.0f;
    m_reloadDuration = w->stats.reloadTime;
    return true;
}

void WeaponSystem::startADS() {
    m_adsActive = true;
}

void WeaponSystem::stopADS() {
    m_adsActive = false;
}

void WeaponSystem::switchTo(int slotIndex) {
    if (slotIndex >= 0 && slotIndex < (int)m_slots.size()) {
        m_currentSlot = slotIndex;
        m_reloading = false;
        m_reloadTimer = 0.0f;
        m_adsActive = false;
        m_adsBlend = 0.0f;
        m_fireTimer = 0.3f; // Brief equip delay
    }
}

void WeaponSystem::switchNext() {
    if (m_slots.empty()) return;
    switchTo((m_currentSlot + 1) % (int)m_slots.size());
}

void WeaponSystem::switchPrev() {
    if (m_slots.empty()) return;
    switchTo((m_currentSlot - 1 + (int)m_slots.size()) % (int)m_slots.size());
}

const WeaponSlot* WeaponSystem::currentWeapon() const {
    if (m_currentSlot < 0 || m_currentSlot >= (int)m_slots.size()) return nullptr;
    return &m_slots[m_currentSlot];
}

Model* WeaponSystem::currentModel() {
    if (m_currentSlot < 0 || m_currentSlot >= (int)m_slots.size()) return nullptr;
    return m_slots[m_currentSlot].model.get();
}

void WeaponSystem::consumeRecovery(float dt) {
    if (m_recoilOffset.x != 0.0f || m_recoilOffset.y != 0.0f) {
        auto* w = currentWeapon();
        float recovery = w ? w->stats.recoilRecovery : 5.0f;
        float step = recovery * dt;

        // Recover pitch
        if (m_recoilOffset.x > 0.0f) {
            m_recoilOffset.x = std::max(0.0f, m_recoilOffset.x - step);
        } else if (m_recoilOffset.x < 0.0f) {
            m_recoilOffset.x = std::min(0.0f, m_recoilOffset.x + step);
        }

        // Recover yaw
        if (m_recoilOffset.y > 0.0f) {
            m_recoilOffset.y = std::max(0.0f, m_recoilOffset.y - step);
        } else if (m_recoilOffset.y < 0.0f) {
            m_recoilOffset.y = std::min(0.0f, m_recoilOffset.y + step);
        }
    }
}

void WeaponSystem::fireRaycast(const glm::vec3& origin, const glm::vec3& direction) {
    if (!m_world) return;

    auto* w = currentWeapon();
    if (!w) return;

    float range = w->stats.range;
    glm::vec3 end = origin + glm::normalize(direction) * range;

    // Use physics raycast
    short mask = Mask::RAYCAST;
    auto result = m_world->raycast(origin, end, mask);

    HitResult hit;
    hit.hit = result.hit;
    hit.hitPoint = result.hitPoint;
    hit.hitNormal = result.hitNormal;
    hit.distance = result.hitFraction * range;
    hit.body = result.body;

    if (m_hitCallback) {
        m_hitCallback(hit);
    }
}

void WeaponSystem::applyRecoil() {
    auto* w = currentWeapon();
    if (!w) return;

    // Vertical recoil (always up)
    float vertRecoil = w->stats.recoilVertical;
    // Horizontal recoil (random side)
    float horizRecoil = w->stats.recoilHorizontal;
    float randH = ((float)std::rand() / RAND_MAX - 0.5f) * 2.0f * horizRecoil;

    // Increase recoil with sustained fire
    m_recoilPattern += 0.1f;
    float patternMult = 1.0f + std::min(m_recoilPattern, 3.0f); // Max 4x at sustained fire

    m_recoilOffset.x += vertRecoil * patternMult;
    m_recoilOffset.y += randH * patternMult;

    // Reset pattern after not firing
    if (m_fireTimer <= 0.0f) {
        m_recoilPattern = 0.0f;
    }
}

} // namespace ps
