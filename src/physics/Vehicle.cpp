#include "physics/Vehicle.h"
#include "physics/CollisionShapes.h"
#include "core/Logger.h"
#include <BulletDynamics/Vehicle/btRaycastVehicle.h>
#include <BulletCollision/CollisionDispatch/btGhostObject.h>
#include <BulletCollision/CollisionShapes/btCompoundShape.h>
#include <glm/gtc/quaternion.hpp>
#include <algorithm>
#include <cctype>
#include <cmath>

namespace ps {

Vehicle::Vehicle() = default;

Vehicle::~Vehicle() {
    // Caller must call destroy() first
}

bool Vehicle::init(PhysicsWorld& world, const std::string& modelPath,
                    const glm::vec3& spawnPos, const VehicleConfig& config) {
    m_config = config;
    m_modelScale = config.modelScale;

    // Load car model
    if (!modelPath.empty()) {
        if (!m_model.load(modelPath)) {
            LOG_WARN("Vehicle model not loaded: %s", modelPath.c_str());
        } else {
            LOG_INFO("Vehicle model loaded: %s (%zu meshes)",
                     modelPath.c_str(), m_model.meshes().size());
            // Identify wheel meshes by name and compute their local centers +
            // the model's actual wheel radius. NOTE: m_modelYOffset is finalized
            // AFTER setupWheels(), because it must account for the suspension rest
            // length -- the wheels hang below the chassis connection point by the
            // rest length, so the body has to be shifted down by that amount to
            // line the model's wheel-arches up with the physics wheel centers.
            identifyWheelMeshes();
            computeWheelMeshCenters();
        }
    }

    // Apply model scale to the measured wheel radius so the physics wheels grow
    // with the rendered car, then apply the user wheelScale (diameter multiplier).
    float baseWheelRadius = (m_modelWheelRadius > 0.0f ? m_modelWheelRadius : 0.35f)
                            * m_modelScale;
    float physWheelRadius = baseWheelRadius * config.wheelScale;
    LOG_INFO("Wheel radius: model=%.3f scaled=%.3f physics(with wheelScale %.2f)=%.3f",
             m_modelWheelRadius, baseWheelRadius, config.wheelScale, physWheelRadius);

    // Build the collision shape. The chassis box must NOT touch the ground at
    // rest -- if it does, Bullet generates a huge upward impulse that launches
    // the car skyward (the classic "rocket car" bug). We wrap the box in a
    // compound shape and raise it by chassisYShift so its bottom sits above the
    // wheel contact plane, leaving the suspension to support the car.
    m_chassisShape = shapes::createBox(config.chassisHalfExtents);
    m_compoundShape = new btCompoundShape();
    btTransform boxLocalTrans;
    boxLocalTrans.setIdentity();
    boxLocalTrans.setOrigin(btVector3(0, config.chassisYShift, 0));
    m_compoundShape->addChildShape(boxLocalTrans, m_chassisShape);

    // Spawn exactly at rest height so the car starts settled on its suspension:
    //   chassisCenterY = groundY + wheelRadius + suspensionRestLength + chassisHalfHeight
    float wheelRestLength = 0.5f;  // matches the default wheel layout in setupWheels()
    float spawnY = spawnPos.y + config.chassisHalfExtents.y + physWheelRadius + wheelRestLength;

    btTransform transform;
    transform.setIdentity();
    transform.setOrigin(btVector3(spawnPos.x, spawnY, spawnPos.z));

    auto* motionState = new btDefaultMotionState(transform);
    btVector3 localInertia(0, 0, 0);
    m_compoundShape->calculateLocalInertia(config.chassisMass, localInertia);

    btRigidBody::btRigidBodyConstructionInfo ci(
        config.chassisMass, motionState, m_compoundShape, localInertia);
    ci.m_friction = 0.3f;
    ci.m_linearDamping = 0.15f;
    ci.m_angularDamping = 0.95f;  // High to resist tipping / spinning out
    ci.m_restitution = 0.0f;      // No bounce on collisions

    m_chassisBody = new btRigidBody(ci);
    m_chassisBody->setActivationState(DISABLE_DEACTIVATION);
    // Keep the car from rolling over its up axis; allow yaw only.
    m_chassisBody->setAngularFactor(btVector3(0, 1, 0));

    world.addRigidBody(m_chassisBody, GROUP_VEHICLE, Mask::VEHICLE);

    // Create raycast vehicle
    m_raycaster = new btDefaultVehicleRaycaster(world.world());
    m_vehicle = new btRaycastVehicle(m_tuning, m_chassisBody, m_raycaster);
    m_vehicle->setCoordinateSystem(0, 1, 2); // right, up, forward

    world.addAction(m_vehicle);

    // Setup wheels
    setupWheels(world, config);

    // Finalize the model Y offset now that the wheels (and their rest length)
    // exist. The body must be shifted down by the suspension rest length so that
    // the model's wheel-arches (at the model's axle height, y=0) line up with the
    // physics wheel centers, which sit one rest-length BELOW the connection points.
    if (m_vehicle->getNumWheels() > 0) {
        m_modelYOffset = -config.chassisHalfExtents.y
                       - m_vehicle->getWheelInfo(0).getSuspensionRestLength();
    } else {
        m_modelYOffset = -config.chassisHalfExtents.y;
    }
    LOG_INFO("Vehicle modelYOffset = %.3f (halfHeight=%.3f, restLen=%.3f, wheelRadius=%.3f, scale=%.2f)",
             m_modelYOffset, config.chassisHalfExtents.y,
             m_vehicle->getNumWheels() > 0 ? m_vehicle->getWheelInfo(0).getSuspensionRestLength() : 0.0f,
             physWheelRadius, m_modelScale);

    // Reset suspension and run a short pre-simulation so the car settles cleanly.
    // IMPORTANT: only resetSuspension ONCE -- calling it every step fights the
    // raycast and is itself a major cause of the car "bouncing / flying off".
    m_vehicle->resetSuspension();
    for (int i = 0; i < 20; ++i) {
        world.world()->stepSimulation(1.0f / 60.0f, 1, 1.0f / 60.0f);
        // Clamp any vertical velocity that the settling might produce so a bad
        // first contact cannot accumulate into a launch.
        btVector3 v = m_chassisBody->getLinearVelocity();
        if (std::abs(v.y()) > 1.0f) v.setY(0.0f);
        m_chassisBody->setLinearVelocity(v);
        m_chassisBody->setAngularVelocity(btVector3(0, 0, 0));
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
    // Physics wheel radius = measured model radius * modelScale * wheelScale
    float baseR = (m_modelWheelRadius > 0.0f ? m_modelWheelRadius : 0.35f) * m_modelScale;
    float r = baseR * config.wheelScale;

    if (config.wheels.empty()) {
        // Default 4-wheel layout if none specified
        float w = config.chassisHalfExtents.x * 0.85f;
        float h = -config.chassisHalfExtents.y;  // Bottom of chassis
        float l = config.chassisHalfExtents.z * 0.7f;

        // Front-left, Front-right, Rear-left, Rear-right
        std::vector<WheelInfo> defaultWheels = {
            {{-w, h,  l}, {0,-1,0}, {-1,0,0}, 0.5f, r, true},   // FL
            {{ w, h,  l}, {0,-1,0}, {-1,0,0}, 0.5f, r, true},   // FR
            {{-w, h, -l}, {0,-1,0}, {-1,0,0}, 0.5f, r, false},  // RL
            {{ w, h, -l}, {0,-1,0}, {-1,0,0}, 0.5f, r, false},  // RR
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
            wheel.m_rollInfluence = config.rollInfluence;
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
            wheel.m_rollInfluence = config.rollInfluence;
            ++m_wheelCount;
        }
    }
}

void Vehicle::update(float dt) {
    if (!m_vehicle || !m_active) return;

    // ── Stability clamp: cap linear speed and zero out any runaway vertical /
    // angular velocity that would otherwise make the car "bounce and fly to sky".
    if (m_chassisBody) {
        btVector3 v = m_chassisBody->getLinearVelocity();
        float horizSpeed = std::sqrt(v.x()*v.x() + v.z()*v.z());
        if (horizSpeed > m_config.maxSpeedMs) {
            float scl = m_config.maxSpeedMs / horizSpeed;
            v.setX(v.x() * scl);
            v.setZ(v.z() * scl);
        }
        // Kill vertical spikes (launches) beyond a sane bound.
        if (v.y() > 8.0f) v.setY(8.0f);
        if (v.y() < -15.0f) v.setY(-15.0f);
        m_chassisBody->setLinearVelocity(v);

        // Clamp angular velocity to prevent tumbling.
        btVector3 av = m_chassisBody->getAngularVelocity();
        if (av.length2() > 9.0f) {  // 3 rad/s cap
            av *= 3.0f / std::sqrt(av.length2());
            m_chassisBody->setAngularVelocity(av);
        }
    }

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
    // m_chassisShape is owned by m_compoundShape as a child; delete compound first.
    delete m_compoundShape; m_compoundShape = nullptr;
    delete m_chassisShape; m_chassisShape = nullptr;
}

VehicleConfig Vehicle::defaultConfig() {
    VehicleConfig c;
    // Sized to match the Lexus IS-F GLB model (body ~2.0 W, ~1.7 H, ~4.66 L).
    // The collision box is intentionally a bit smaller than the visual body so
    // wheel collision (raycast) handles most contact; the box prevents the car
    // from driving through walls.
    c.chassisHalfExtents = {0.95f, 0.45f, 2.15f};
    c.chassisYShift = 0.15f;          // Raise box so it clears the ground at rest
    c.chassisMass = 1200.0f;
    // Bullet3-correct suspension tuning: stiffness must be balanced with damping
    // or the car accumulates energy and launches. These values are stable.
    c.suspensionStiffness = 28.0f;
    c.suspensionDamping = 4.4f;       // relaxation
    c.suspensionCompression = 2.3f;
    c.maxSuspensionTravel = 0.3f;
    c.frictionSlip = 10.5f;           // Good grip without being grippy-glitchy
    c.rollInfluence = 0.04f;          // Very low = stable, resists tipping
    c.maxEngineForce = 1800.0f;       // Tame acceleration (was 2500 -> launched)
    c.maxBrakingForce = 120.0f;
    c.maxSteerAngle = 0.5f;
    c.maxSpeedMs = 50.0f;             // ~180 km/h limiter
    c.modelScale = 1.15f;             // Car + wheels ~15% bigger (FPV feel)
    c.wheelScale = 1.25f;             // Wheels 25% larger diameter (user request)
    return c;
}

VehicleConfig Vehicle::suvConfig() {
    VehicleConfig c;
    c.chassisHalfExtents = {1.1f, 0.6f, 2.5f};
    c.chassisYShift = 0.2f;
    c.chassisMass = 2200.0f;
    c.suspensionStiffness = 32.0f;
    c.suspensionDamping = 5.5f;
    c.suspensionCompression = 3.0f;
    c.maxSuspensionTravel = 0.4f;
    c.frictionSlip = 11.0f;
    c.rollInfluence = 0.05f;
    c.maxEngineForce = 2600.0f;
    c.maxBrakingForce = 140.0f;
    c.maxSteerAngle = 0.45f;
    c.maxSpeedMs = 50.0f;
    c.modelScale = 1.2f;
    c.wheelScale = 1.25f;
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
            // Measure the wheel radius from the Y/Z half-extents. The wheel is a
            // disc in the Y-Z plane (axle along X), so the radius is the larger of
            // the Y / Z half-extents. Keep the largest across all wheel groups --
            // the tyre group dominates and gives the true outer radius.
            float extY = 0.5f * (maxBound.y - minBound.y);
            float extZ = 0.5f * (maxBound.z - minBound.z);
            float groupRadius = std::max(extY, extZ);
            if (groupRadius > m_modelWheelRadius) m_modelWheelRadius = groupRadius;
            LOG_INFO("  Wheel group %d center: (%.3f, %.3f, %.3f) radius=%.3f [%d verts]",
                     g, m_wheelMeshCenters[g].x, m_wheelMeshCenters[g].y,
                     m_wheelMeshCenters[g].z, groupRadius, vertexCount);
        }
    }
    LOG_INFO("  Model wheel radius (max of groups): %.3f", m_modelWheelRadius);
}

} // namespace ps
