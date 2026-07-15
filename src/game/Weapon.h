#pragma once

#include <glm/glm.hpp>
#include <string>

namespace ps {

enum class FireMode {
    SEMI,       // Single shot per click
    BURST,      // 3-round burst
    FULL_AUTO   // Continuous while held
};

enum class WeaponCategory {
    PISTOL,
    RIFLE,
    SHOTGUN,
    SNIPER,
    SMG,
    MELEE
};

struct WeaponStats {
    std::string name;
    WeaponCategory category = WeaponCategory::RIFLE;
    FireMode fireMode = FireMode::FULL_AUTO;

    float damage = 25.0f;
    float fireRate = 0.1f;        // Seconds between shots
    float range = 200.0f;         // Max raycast distance (meters)

    int magSize = 30;
    int reserveAmmo = 90;
    float reloadTime = 2.5f;      // Seconds

    // Recoil
    float recoilVertical = 1.5f;   // Degrees up per shot
    float recoilHorizontal = 0.3f; // Degrees side per shot (random)
    float recoilRecovery = 5.0f;   // Degrees/sec recovery speed

    // ADS (Aim Down Sights)
    float adsZoomFov = 45.0f;     // FOV when ADS
    float adsTransitionTime = 0.2f; // Time to enter/exit ADS

    // Visual offsets for first-person view
    glm::vec3 viewOffset{0.25f, -0.2f, -0.5f};   // Right, Down, Forward
    glm::vec3 adsOffset{0.0f, -0.15f, -0.35f};   // Centered when ADS
    float viewScale = 0.15f;                      // Model scale in view (real-world models need heavy downscaling)
    float viewRotateY = -90.0f;                   // Rotate model to face forward (GLB models typically point +X)

    // Muzzle flash
    glm::vec3 muzzleOffset{0.0f, 0.05f, -0.8f};  // Relative to weapon
    float muzzleFlashDuration = 0.05f;
    float muzzleLightRadius = 5.0f;
};

// Predefined weapon configs
namespace Weapons {
    WeaponStats ak74m();
    WeaponStats pistol9mm();
    WeaponStats shotgun();
    WeaponStats sniper();
    WeaponStats meleeBlade();
}

} // namespace ps
