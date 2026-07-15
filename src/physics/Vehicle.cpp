#include "physics/Vehicle.h"
#include "physics/CollisionShapes.h"
#include "core/Logger.h"
#include <BulletDynamics/Vehicle/btRaycastVehicle.h>
#include <BulletCollision/CollisionDispatch/btGhostObject.h>
#include <glm/gtc/quaternion.hpp>
#include <algorithm>
#include <cctype>

namespace ps {

Vehicle::Vehicle() = default;

Vehicle::~Vehicle() {
    // Caller must call destroy() first
}

bool Vehicle::init(PhysicsWorld& world, const std::string& modelPath,
                    const glm::vec3& spawnPos, const VehicleConfig& config) {
    m_config = config;

    // Load car model
    if (!modelPath.empty()) {
        if (!m_model.load(modelPath)) {
            LOG_WARN("Vehicle model not loaded: %s", modelPath.c_str());
        } else {
            LOG_INFO("Vehicle model loaded: %s (%zu meshes)",
                     modelPath.c_str(), m_model.meshes().size());
            // Model origin is typically at the bottom (ground level).
            // Chassis center is at the middle of the box, so offset = -halfHeight.
            m_modelYOffset = -config.chassisHalfExtents.y;

            // Identify wheel meshes by name and compute their local centers
            identifyWheelMeshes();
            computeWheelMeshCenters();
        }
    }

    // Create chassis box shape
    m_chassisShape = shapes::createBox(config.chassisHalfExtents);

    // Create chassis rigid body
    btTransform transform;
    transform.setIdentity();
    // Spawn low enough that wheels are near ground (chassis bottom at spawnY - halfHeight)
    float spawnY = spawnPos.y + config.chassisHalfExtents.y + 0.55f;
    transform.setOrigin(btVector3(spawnPos.x, spawnY, spawnPos.z));

    auto* motionState = new btDefaultMotionState(transform);
    btVector3 localInertia(0, 0, 0);
    m_chassisShape->calculateLocalInertia(config.chassisMass, localInertia);

    btRigidBody::btRigidBodyConstructionInfo ci(
        config.chassisMass, motionState, m_chassisShape, localInertia);
    ci.m_friction = 0.3f;
    ci.m_linearDamping = 0.2f;
    ci.m_angularDamping = 0.99f;  // Very high to prevent flipping

    m_chassisBody = new btRigidBody(ci);
    m_chassisBody->setActivationState(DISABLE_DEACTIVATION);

    world.addRigidBody(m_chassisBody, GROUP_VEHICLE, Mask::VEHICLE);

    // Create raycast vehicle
    m_raycaster = new btDefaultVehicleRaycaster(world.world());
    m_vehicle = new btRaycastVehicle(m_tuning, m_chassisBody, m_raycaster);
    m_vehicle->setCoordinateSystem(0, 1, 2); // right, up, forward

    world.addAction(m_vehicle);

    // Setup wheels
    setupWheels(world, config);

    // IMPORTANT: Reset suspension to stabilize initial state
    m_vehicle->resetSuspension();

    // Pre-simulate to let the car settle on the ground before activating
    for (int i = 0; i < 60; ++i) {
        world.world()->stepSimulation(1.0f / 60.0f, 1, 1.0f / 60.0f);
        m_vehicle->resetSuspension();
    }

    // Pre-update to establish initial wheel contact
    for (int i = 0; i < m_vehicle->getNumWheels(); ++i) {
        m_vehicle->updateWheelTransform(i, true);
    }

    m_active = true;
    LOG_INFO("Vehicle initialized at (%.1f, %.1f, %.1f) with %d wheels",
             spawnPos.x, spawnPos.y, spawnPos.z, m_wheelCount);
    return true;
}

void Vehicle::setupWheels(PhysicsWorld& world, const VehicleConfig& config) {
    if (config.wheels.empty()) {
        // Default 4-wheel layout if none specified
        float w = config.chassisHalfExtents.x * 0.85f;
        float h = -config.chassisHalfExtents.y;  // Bottom of chassis
        float l = config.chassisHalfExtents.z * 0.7f;

        // Front-left, Front-right, Rear-left, Rear-right
        config.wheels; // Can't modify const ref, use defaults below
        std::vector<WheelInfo> defaultWheels = {
            {{-w, h,  l}, {0,-1,0}, {-1,0,0}, 0.5f, 0.35f, true},   // FL
            {{ w, h,  l}, {0,-1,0}, {-1,0,0}, 0.5f, 0.35f, true},   // FR
            {{-w, h, -l}, {0,-1,0}, {-1,0,0}, 0.5f, 0.35f, false},  // RL
            {{ w, h, -l}, {0,-1,0}, {-1,0,0}, 0.5f, 0.35f, false},  // RR
        };

        for (const auto& w_info : defaultWheels) {
            btWheelInfo& wheel = m_vehicle->addWheel(
                btVector3(w_info.connectionPoint.x, w_info.connectionPoint.y, w_info.connectionPoint.z),
                btVector3(w_info.wheelDirection.x, w_info.wheelDirection.y, w_info.wheelDirection.z),
                btVector3(w_info.wheelAxle.x, w_info.wheelAxle.y, w_info.wheelAxle.z),
                w_info.suspensionRestLength,
                w_info.wheelRadius,
                m_tuning,
                w_info.isFrontWheel
            );

            wheel.m_suspensionStiffness = config.suspensionStiffness;
            wheel.m_wheelsDampingRelaxation = config.suspensionDamping;
            wheel.m_wheelsDampingCompression = config.suspensionCompression;
            wheel.m_frictionSlip = config.frictionSlip;
            wheel.m_maxSuspensionTravelCm = config.maxSuspensionTravel * 100.0f;
            ++m_wheelCount;
        }
    } else {
        for (const auto& w_info : config.wheels) {
            btWheelInfo& wheel = m_vehicle->addWheel(
                btVector3(w_info.connectionPoint.x, w_info.connectionPoint.y, w_info.connectionPoint.z),
                btVector3(w_info.wheelDirection.x, w_info.wheelDirection.y, w_info.wheelDirection.z),
                btVector3(w_info.wheelAxle.x, w_info.wheelAxle.y, w_info.wheelAxle.z),
                w_info.suspensionRestLength,
                w_info.wheelRadius,
                m_tuning,
                w_info.isFrontWheel
            );

            wheel.m_suspensionStiffness = config.suspensionStiffness;
            wheel.m_wheelsDampingRelaxation = config.suspensionDamping;
            wheel.m_wheelsDampingCompression = config.suspensionCompression;
            wheel.m_frictionSlip = config.frictionSlip;
            wheel.m_maxSuspensionTravelCm = config.maxSuspensionTravel * 100.0f;
            ++m_wheelCount;
        }
    }
}

void Vehicle::update(float dt) {
    if (!m_vehicle || !m_active) return;

    // Apply engine force to rear wheels (or all for AWD)
    float engineForce = m_throttle * m_config.maxEngineForce;
    float brakeForce = m_brake * m_config.maxBrakingForce;

    for (int i = 0; i < m_vehicle->getNumWheels(); ++i) {
        btWheelInfo& wheel = m_vehicle->getWheelInfo(i);
        bool isFront = wheel.m_bIsFrontWheel;

        // Apply steering to front wheels
        if (isFront) {
            m_vehicle->setSteeringValue(m_steering * m_config.maxSteerAngle, i);
        }

        // Apply engine force to rear wheels
        if (!isFront) {
            m_vehicle->applyEngineForce(engineForce, i);
        }

        // Apply braking
        m_vehicle->setBrake(brakeForce, i);

        // Handbrake locks rear wheels
        if (m_handbrake && !isFront) {
            m_vehicle->setBrake(m_config.maxBrakingForce * 3.0f, i);
        }
    }

    // Update model transform from chassis
    // Note: rendering uses chassisTransform() directly, no need to set model transform here
    // The model Y offset is applied in the rendering pass via modelYOffset()
}

void Vehicle::setThrottle(float amount) {
    m_throttle = glm::clamp(amount, -1.0f, 1.0f);
}

void Vehicle::setSteering(float amount) {
    m_steering = glm::clamp(amount, -1.0f, 1.0f);
}

void Vehicle::setBrake(float amount) {
    m_brake = glm::clamp(amount, 0.0f, 1.0f);
}

void Vehicle::setHandbrake(bool active) {
    m_handbrake = active;
}

glm::vec3 Vehicle::position() const {
    if (!m_chassisBody) return {0, 0, 0};
    btTransform trans;
    m_chassisBody->getMotionState()->getWorldTransform(trans);
    btVector3 pos = trans.getOrigin();
    return {pos.x(), pos.y(), pos.z()};
}

glm::mat4 Vehicle::chassisTransform() const {
    if (!m_chassisBody) return glm::mat4(1.0f);
    btTransform trans;
    m_chassisBody->getMotionState()->getWorldTransform(trans);
    btVector3 pos = trans.getOrigin();
    btQuaternion rot = trans.getRotation();

    glm::mat4 m = glm::mat4(1.0f);
    m = glm::translate(m, glm::vec3(pos.x(), pos.y(), pos.z()));
    glm::quat q(rot.w(), rot.x(), rot.y(), rot.z());
    m *= glm::mat4_cast(q);
    return m;
}

glm::mat4 Vehicle::wheelTransform(int index) const {
    if (!m_vehicle || index < 0 || index >= m_vehicle->getNumWheels())
        return glm::mat4(1.0f);

    m_vehicle->updateWheelTransform(index, true);
    const btTransform& trans = m_vehicle->getWheelInfo(index).m_worldTransform;
    btVector3 pos = trans.getOrigin();
    btQuaternion rot = trans.getRotation();

    glm::mat4 m = glm::mat4(1.0f);
    m = glm::translate(m, glm::vec3(pos.x(), pos.y(), pos.z()));
    glm::quat q(rot.w(), rot.x(), rot.y(), rot.z());
    m *= glm::mat4_cast(q);
    return m;
}

float Vehicle::speedKmh() const {
    if (!m_chassisBody) return 0.0f;
    btVector3 vel = m_chassisBody->getLinearVelocity();
    // Speed in km/h (dot product with forward direction for signed speed)
    btTransform trans;
    m_chassisBody->getMotionState()->getWorldTransform(trans);
    btVector3 forward = trans.getBasis().getColumn(2); // Z = forward
    float speedMs = vel.dot(forward);
    return speedMs * 3.6f;
}

void Vehicle::destroy(PhysicsWorld& world) {
    if (m_vehicle) {
        world.removeAction(m_vehicle);
        delete m_vehicle;
        m_vehicle = nullptr;
    }
    delete m_raycaster; m_raycaster = nullptr;

    if (m_chassisBody) {
        world.removeRigidBody(m_chassisBody);
        delete m_chassisBody->getMotionState();
        delete m_chassisBody;
        m_chassisBody = nullptr;
    }
    delete m_chassisShape; m_chassisShape = nullptr;
}

VehicleConfig Vehicle::defaultConfig() {
    VehicleConfig c;
    c.chassisHalfExtents = {0.9f, 0.4f, 2.0f};
    c.chassisMass = 1200.0f;
    c.suspensionStiffness = 15.0f;    // Softer = less bouncing
    c.suspensionDamping = 4.0f;       // Higher = absorbs oscillation
    c.suspensionCompression = 4.5f;
    c.maxSuspensionTravel = 0.4f;
    c.frictionSlip = 5.0f;            // Realistic tire grip (was 1000!)
    c.maxEngineForce = 2500.0f;
    c.maxBrakingForce = 100.0f;
    c.maxSteerAngle = 0.5f;
    return c;
}

VehicleConfig Vehicle::suvConfig() {
    VehicleConfig c;
    c.chassisHalfExtents = {1.1f, 0.6f, 2.5f};
    c.chassisMass = 2200.0f;
    c.suspensionStiffness = 20.0f;
    c.suspensionDamping = 5.0f;
    c.suspensionCompression = 6.0f;
    c.maxSuspensionTravel = 0.5f;
    c.frictionSlip = 5.0f;
    c.maxEngineForce = 3500.0f;
    c.maxBrakingForce = 120.0f;
    c.maxSteerAngle = 0.45f;
    return c;
}

void Vehicle::identifyWheelMeshes() {
    m_wheelMeshGroup.resize(4);  // FL=0, FR=1, RL=2, RR=3
    const auto& meshes = m_model.meshes();

    for (int i = 0; i < static_cast<int>(meshes.size()); ++i) {
        std::string name = meshes[i].name();
        std::string lower;
        lower.resize(name.size());
        std::transform(name.begin(), name.end(), lower.begin(),
                       [](unsigned char c) { return std::tolower(c); });

        // Skip steering wheel
        if (lower.find("steer") != std::string::npos) continue;

        bool isWheelPart = false;
        int group = -1;

        // Calipers have explicit position: BRAKE_CALIPER_FRONT_LEFT, etc.
        if (lower.find("caliper") != std::string::npos) {
            isWheelPart = true;
            bool isLeft = lower.find("left") != std::string::npos;
            bool isRight = lower.find("right") != std::string::npos;
            bool isFront = lower.find("front") != std::string::npos;
            bool isRear = lower.find("rear") != std::string::npos;
            if (isFront && isLeft) group = 0;       // FL
            else if (isFront && isRight) group = 1;  // FR
            else if (isRear && isLeft) group = 2;    // RL
            else if (isRear && isRight) group = 3;   // RR
        }
        // Rotor/Tyre/Wheel use numeric suffix: no suffix=0, 1=1, 2=2, 3=3
        else if (lower.find("rotor") != std::string::npos ||
                 lower.find("tyre") != std::string::npos ||
                 lower.find("tire") != std::string::npos ||
                 lower.find("wheel") != std::string::npos) {
            isWheelPart = true;
            // Extract suffix number from name (e.g., "rotor1" -> 1, "rotor" -> 0)
            // Look for a digit after the part keyword
            size_t digitPos = std::string::npos;
            for (size_t j = 0; j < lower.size(); ++j) {
                if (std::isdigit(lower[j]) && j > 0 && std::isalpha(lower[j-1])) {
                    digitPos = j;
                    break;
                }
            }
            group = (digitPos != std::string::npos) ? (lower[digitPos] - '0') : 0;
            if (group > 3) group = 3;
        }

        if (isWheelPart && group >= 0 && group < 4) {
            m_wheelMeshGroup[group].push_back(i);
            LOG_INFO("  -> Wheel part[%d] group %d: '%s'", i, group, name.c_str());
        }
    }

    for (int g = 0; g < 4; ++g) {
        if (m_wheelMeshGroup[g].empty()) {
            LOG_WARN("  Wheel group %d has no meshes", g);
        } else {
            LOG_INFO("  Wheel group %d: %zu meshes", g, m_wheelMeshGroup[g].size());
        }
    }
}

int Vehicle::findWheelGroup(int meshIndex) const {
    for (int g = 0; g < static_cast<int>(m_wheelMeshGroup.size()); ++g) {
        for (int mi : m_wheelMeshGroup[g]) {
            if (mi == meshIndex) return g;
        }
    }
    return -1;
}

void Vehicle::computeWheelMeshCenters() {
    m_wheelMeshCenters.resize(m_wheelMeshGroup.size(), glm::vec3(0.0f));
    const auto& meshes = m_model.meshes();

    for (int g = 0; g < static_cast<int>(m_wheelMeshGroup.size()); ++g) {
        glm::vec3 minBound(FLT_MAX);
        glm::vec3 maxBound(-FLT_MAX);
        int vertexCount = 0;

        for (int mi : m_wheelMeshGroup[g]) {
            if (mi < 0 || mi >= static_cast<int>(meshes.size())) continue;
            const auto& verts = meshes[mi].vertices();
            for (const auto& v : verts) {
                minBound = glm::min(minBound, v.position);
                maxBound = glm::max(maxBound, v.position);
                ++vertexCount;
            }
        }

        if (vertexCount > 0) {
            // Center of bounding box
            m_wheelMeshCenters[g] = (minBound + maxBound) * 0.5f;
            LOG_INFO("  Wheel group %d center: (%.3f, %.3f, %.3f) [%d verts]",
                     g, m_wheelMeshCenters[g].x, m_wheelMeshCenters[g].y,
                     m_wheelMeshCenters[g].z, vertexCount);
        }
    }
}

} // namespace ps
