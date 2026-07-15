#pragma once

#include "physics/PhysicsWorld.h"
#include "renderer/Model.h"
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <string>
#include <vector>
#include <set>

namespace ps {

struct WheelInfo {
    glm::vec3 connectionPoint{0.0f};
    glm::vec3 wheelDirection{0, -1, 0};
    glm::vec3 wheelAxle{-1, 0, 0};
    float suspensionRestLength = 0.3f;
    float wheelRadius = 0.35f;
    bool isFrontWheel = false;
};

struct VehicleConfig {
    // Chassis
    glm::vec3 chassisHalfExtents{1.0f, 0.5f, 2.2f};
    float chassisMass = 1200.0f;

    // Suspension
    float suspensionStiffness = 30.0f;
    float suspensionDamping = 2.5f;
    float suspensionCompression = 4.0f;
    float maxSuspensionTravel = 0.3f;
    float frictionSlip = 1000.0f;

    // Engine
    float maxEngineForce = 2500.0f;
    float maxBrakingForce = 100.0f;
    float maxSteerAngle = 0.5f;  // ~30 degrees

    // Wheels (relative to chassis center)
    std::vector<WheelInfo> wheels;
};

class Vehicle {
public:
    Vehicle();
    ~Vehicle();

    // Load car model and setup physics
    bool init(PhysicsWorld& world, const std::string& modelPath,
              const glm::vec3& spawnPos, const VehicleConfig& config = defaultConfig());

    // Update vehicle (called each frame after physics step)
    void update(float dt);

    // Controls
    void setThrottle(float amount);   // -1 (reverse) to 1 (forward)
    void setSteering(float amount);   // -1 (left) to 1 (right)
    void setBrake(float amount);      // 0 to 1
    void setHandbrake(bool active);

    // State
    glm::vec3 position() const;
    glm::mat4 chassisTransform() const;
    glm::mat4 wheelTransform(int index) const;
    float speedKmh() const;
    bool isActive() const { return m_active; }
    void setActive(bool active) { m_active = active; }

    // Model
    Model& model() { return m_model; }
    const Model& model() const { return m_model; }
    float modelYOffset() const { return m_modelYOffset; }

    // Wheel mesh identification
    bool isWheelMesh(int meshIndex) const { return m_wheelMeshGroup.size() > 0 && findWheelGroup(meshIndex) >= 0; }
    int wheelCount() const { return m_wheelCount; }
    // Returns wheel group index (0-3) or -1 if not a wheel mesh
    int findWheelGroup(int meshIndex) const;
    // Get the local-space center offset for a wheel group's meshes
    const glm::vec3& wheelMeshCenter(int groupIndex) const { return m_wheelMeshCenters[groupIndex]; }

    // Cleanup
    void destroy(PhysicsWorld& world);

    // Default sedan config
    static VehicleConfig defaultConfig();
    // SUV config (larger, heavier)
    static VehicleConfig suvConfig();

private:
    void setupWheels(PhysicsWorld& world, const VehicleConfig& config);

    // Physics
    btCollisionShape* m_chassisShape = nullptr;
    btRigidBody* m_chassisBody = nullptr;
    btVehicleRaycaster* m_raycaster = nullptr;
    btRaycastVehicle* m_vehicle = nullptr;
    btRaycastVehicle::btVehicleTuning m_tuning;

    // Model
    Model m_model;

    // Controls
    float m_throttle = 0.0f;
    float m_steering = 0.0f;
    float m_brake = 0.0f;
    bool m_handbrake = false;

    // State
    bool m_active = false;
    VehicleConfig m_config;
    float m_modelYOffset = 0.0f;  // Y offset to align model origin with chassis center

    // Wheel indices in btRaycastVehicle
    int m_wheelCount = 0;

    // Wheel mesh groups: m_wheelMeshGroup[i] = list of mesh indices for wheel i
    std::vector<std::vector<int>> m_wheelMeshGroup;
    // Local-space center of each wheel group's vertices (in model space)
    std::vector<glm::vec3> m_wheelMeshCenters;
    void identifyWheelMeshes();
    void computeWheelMeshCenters();
};

} // namespace ps
