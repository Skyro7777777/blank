#pragma once

#include "game/WeaponSystem.h"
#include <glm/glm.hpp>
#include <vector>

namespace ps {

// Simple bullet hole / spark decal
struct ImpactDecal {
    glm::vec3 position{0.0f};
    glm::vec3 normal{0.0f};
    float lifetime = 0.0f;
    float maxLifetime = 10.0f; // Seconds before fade
    bool isSpark = false;
};

class ImpactSystem {
public:
    ImpactSystem() = default;

    void init();
    void update(float dt);

    // Add impact at hit point
    void addImpact(const HitResult& hit);

    // Access decals for rendering
    const std::vector<ImpactDecal>& decals() const { return m_decals; }

    // Cleanup old decals
    void cleanup();

    static constexpr int MAX_DECALS = 100;

private:
    std::vector<ImpactDecal> m_decals;
};

} // namespace ps
