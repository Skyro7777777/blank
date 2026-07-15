#pragma once

#include "physics/PhysicsWorld.h"
#include <glm/glm.hpp>

namespace ps {

// Collision groups for filtering
enum CollisionGroup : short {
    GROUP_NONE      = 0,
    GROUP_STATIC    = 1 << 0,  // Environment, ground, walls
    GROUP_PLAYER    = 1 << 1,  // Player capsule
    GROUP_DYNAMIC   = 1 << 2,  // Pickups, physics props
    GROUP_VEHICLE   = 1 << 3,  // Car chassis
    GROUP_WHEEL     = 1 << 4,  // Vehicle wheels
    GROUP_PROJECTILE= 1 << 5,  // Bullets, grenades
    GROUP_INTERACT  = 1 << 6,  // Doors, interactive objects
};

// Helper to build collision mask (what this group collides WITH)
inline short collideWith(short g1, short g2 = 0, short g3 = 0, short g4 = 0) {
    return g1 | g2 | g3 | g4;
}

// Common masks
namespace Mask {
    constexpr short STATIC   = GROUP_PLAYER | GROUP_DYNAMIC | GROUP_VEHICLE | GROUP_WHEEL;
    constexpr short PLAYER   = GROUP_STATIC | GROUP_DYNAMIC | GROUP_VEHICLE | GROUP_INTERACT;
    constexpr short DYNAMIC  = GROUP_STATIC | GROUP_PLAYER | GROUP_DYNAMIC | GROUP_VEHICLE;
    constexpr short VEHICLE  = GROUP_STATIC | GROUP_DYNAMIC | GROUP_WHEEL;
    constexpr short WHEEL    = GROUP_STATIC | GROUP_VEHICLE;
    constexpr short PROJECTILE = GROUP_STATIC | GROUP_DYNAMIC | GROUP_VEHICLE | GROUP_PLAYER;
    constexpr short RAYCAST  = GROUP_STATIC | GROUP_DYNAMIC | GROUP_VEHICLE | GROUP_INTERACT;
}

// Ground plane setup helper
btRigidBody* createGroundPlane(PhysicsWorld& world, float y = 0.0f);

// Static box obstacle (wall, crate, etc.)
btRigidBody* createStaticBox(PhysicsWorld& world, const glm::vec3& halfExtents,
                              const glm::vec3& position, const glm::vec3& rotation = {0,0,0});

} // namespace ps
