#include "game/ImpactSystem.h"
#include "core/Logger.h"
#include <cstdlib>
#include <algorithm>

namespace ps {

void ImpactSystem::init() {
    m_decals.reserve(MAX_DECALS);
    LOG_INFO("Impact system initialized");
}

void ImpactSystem::update(float dt) {
    for (auto& d : m_decals) {
        d.lifetime += dt;
    }
    // Remove expired decals
    cleanup();
}

void ImpactSystem::addImpact(const HitResult& hit) {
    if (!hit.hit) return;

    ImpactDecal decal;
    decal.position = hit.hitPoint;
    decal.normal = hit.hitNormal;
    decal.maxLifetime = 15.0f;

    // Random: spark or bullet hole
    float r = (float)std::rand() / RAND_MAX;
    decal.isSpark = (r < 0.3f); // 30% chance of spark

    // Limit total decals
    if ((int)m_decals.size() >= MAX_DECALS) {
        m_decals.erase(m_decals.begin()); // Remove oldest
    }
    m_decals.push_back(decal);
}

void ImpactSystem::cleanup() {
    m_decals.erase(
        std::remove_if(m_decals.begin(), m_decals.end(),
            [](const ImpactDecal& d) { return d.lifetime >= d.maxLifetime; }),
        m_decals.end()
    );
}

} // namespace ps
