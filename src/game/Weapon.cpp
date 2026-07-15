#include "game/Weapon.h"

namespace ps {

namespace Weapons {

WeaponStats ak74m() {
    WeaponStats s;
    s.name = "AK-74M Zenitco";
    s.category = WeaponCategory::RIFLE;
    s.fireMode = FireMode::FULL_AUTO;
    s.damage = 28.0f;
    s.fireRate = 0.09f;       // ~660 RPM
    s.range = 250.0f;
    s.magSize = 30;
    s.reserveAmmo = 120;
    s.reloadTime = 2.8f;
    s.recoilVertical = 2.0f;
    s.recoilHorizontal = 0.5f;
    s.recoilRecovery = 4.0f;
    s.adsZoomFov = 50.0f;
    s.adsTransitionTime = 0.2f;
    s.viewOffset = {0.2f, -0.18f, -0.3f};
    s.adsOffset = {0.0f, -0.08f, -0.5f};
    s.viewScale = 0.12f;
    s.viewRotateY = -90.0f;
    s.muzzleOffset = {0.0f, 0.04f, -0.9f};
    return s;
}

WeaponStats pistol9mm() {
    WeaponStats s;
    s.name = "9mm Pistol";
    s.category = WeaponCategory::PISTOL;
    s.fireMode = FireMode::SEMI;
    s.damage = 20.0f;
    s.fireRate = 0.15f;
    s.range = 100.0f;
    s.magSize = 15;
    s.reserveAmmo = 60;
    s.reloadTime = 1.8f;
    s.recoilVertical = 3.0f;
    s.recoilHorizontal = 0.8f;
    s.recoilRecovery = 6.0f;
    s.adsZoomFov = 55.0f;
    s.adsTransitionTime = 0.15f;
    s.viewOffset = {0.15f, -0.15f, -0.25f};
    s.adsOffset = {0.0f, -0.08f, -0.4f};
    s.viewScale = 0.13f;
    s.viewRotateY = -90.0f;
    s.muzzleOffset = {0.0f, 0.03f, -0.4f};
    return s;
}

WeaponStats shotgun() {
    WeaponStats s;
    s.name = "Benelli M4";
    s.category = WeaponCategory::SHOTGUN;
    s.fireMode = FireMode::SEMI;
    s.damage = 12.0f;  // Per pellet (8 pellets = 96 total)
    s.fireRate = 0.4f;
    s.range = 50.0f;
    s.magSize = 7;
    s.reserveAmmo = 28;
    s.reloadTime = 3.5f;
    s.recoilVertical = 5.0f;
    s.recoilHorizontal = 1.0f;
    s.recoilRecovery = 3.0f;
    s.adsZoomFov = 55.0f;
    s.adsTransitionTime = 0.25f;
    s.viewOffset = {0.2f, -0.2f, -0.3f};
    s.adsOffset = {0.0f, -0.1f, -0.5f};
    s.viewScale = 0.11f;
    s.viewRotateY = -90.0f;
    s.muzzleOffset = {0.0f, 0.04f, -1.0f};
    return s;
}

WeaponStats sniper() {
    WeaponStats s;
    s.name = "Sniper Rifle";
    s.category = WeaponCategory::SNIPER;
    s.fireMode = FireMode::SEMI;
    s.damage = 100.0f;
    s.fireRate = 1.5f;
    s.range = 500.0f;
    s.magSize = 5;
    s.reserveAmmo = 20;
    s.reloadTime = 3.2f;
    s.recoilVertical = 8.0f;
    s.recoilHorizontal = 1.5f;
    s.recoilRecovery = 2.0f;
    s.adsZoomFov = 20.0f;
    s.adsTransitionTime = 0.3f;
    s.viewOffset = {0.22f, -0.2f, -0.3f};
    s.adsOffset = {0.0f, -0.1f, -0.55f};
    s.viewScale = 0.1f;
    s.viewRotateY = -90.0f;
    s.muzzleOffset = {0.0f, 0.05f, -1.2f};
    return s;
}

WeaponStats meleeBlade() {
    WeaponStats s;
    s.name = "Combat Blade";
    s.category = WeaponCategory::MELEE;
    s.fireMode = FireMode::SEMI;
    s.damage = 50.0f;
    s.fireRate = 0.5f;
    s.range = 3.0f;
    s.magSize = 0;
    s.reserveAmmo = 0;
    s.reloadTime = 0.0f;
    s.recoilVertical = 0.0f;
    s.recoilHorizontal = 0.0f;
    s.recoilRecovery = 0.0f;
    s.viewOffset = {0.25f, -0.25f, -0.25f};
    s.adsOffset = {0.08f, -0.15f, -0.35f};
    s.viewScale = 0.12f;
    s.viewRotateY = -90.0f;
    return s;
}

} // namespace Weapons

} // namespace ps
