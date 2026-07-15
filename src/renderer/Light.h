#pragma once

#include <glm/glm.hpp>
#include <vector>

namespace ps {

struct DirectionalLight {
    glm::vec3 direction{0.0f, -1.0f, -0.5f};
    glm::vec3 color{1.0f, 0.95f, 0.9f};
    float intensity = 1.0f;
    bool castShadows = true;
};

struct PointLight {
    glm::vec3 position{0.0f};
    glm::vec3 color{1.0f};
    float intensity = 1.0f;
    float constant = 1.0f;
    float linear = 0.09f;
    float quadratic = 0.032f;
    bool castShadows = false;
};

struct SpotLight {
    glm::vec3 position{0.0f};
    glm::vec3 direction{0.0f, -1.0f, 0.0f};
    glm::vec3 color{1.0f};
    float intensity = 1.0f;
    float cutOff = 12.5f;      // Inner cone angle (degrees)
    float outerCutOff = 17.5f; // Outer cone angle (degrees)
    float constant = 1.0f;
    float linear = 0.09f;
    float quadratic = 0.032f;
};

// Max lights for uniform arrays
constexpr int MAX_POINT_LIGHTS = 16;
constexpr int MAX_SPOT_LIGHTS = 4;

} // namespace ps
