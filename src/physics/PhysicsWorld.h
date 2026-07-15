#pragma once

#include <btBulletDynamicsCommon.h>
#include <glm/glm.hpp>
#include <memory>
#include <functional>

namespace ps {

// Contact callback info
struct ContactInfo {
    btRigidBody* bodyA = nullptr;
    btRigidBody* bodyB = nullptr;
    glm::vec3 contactPoint{0.0f};
    glm::vec3 contactNormal{0.0f};
};

using ContactCallback = std::function<void(const ContactInfo&)>;

class PhysicsWorld {
public:
    PhysicsWorld();
    ~PhysicsWorld();

    PhysicsWorld(const PhysicsWorld&) = delete;
    PhysicsWorld& operator=(const PhysicsWorld&) = delete;

    // Initialize the physics world
    bool init(const glm::vec3& gravity = {0.0f, -9.81f, 0.0f});

    // Step simulation
    void step(float dt);

    // Add/remove rigid bodies
    void addRigidBody(btRigidBody* body);
    void addRigidBody(btRigidBody* body, short group, short mask);
    void removeRigidBody(btRigidBody* body);

    // Add/remove constraints
    void addConstraint(btTypedConstraint* constraint);
    void removeConstraint(btTypedConstraint* constraint);

    // Add/remove vehicles
    void addAction(btActionInterface* action);
    void removeAction(btActionInterface* action);

    // Raycasting
    struct RayResult {
        bool hit = false;
        glm::vec3 hitPoint{0.0f};
        glm::vec3 hitNormal{0.0f};
        float hitFraction = 1.0f;
        btRigidBody* body = nullptr;
        const btCollisionShape* shape = nullptr;
    };

    RayResult raycast(const glm::vec3& from, const glm::vec3& to,
                      short collisionMask = -1) const;

    // Multi-hit raycast (returns all hits along ray)
    std::vector<RayResult> raycastAll(const glm::vec3& from, const glm::vec3& to,
                                       short collisionMask = -1) const;

    // Contact callbacks
    void setContactCallback(ContactCallback cb) { m_contactCallback = cb; }

    // Access
    btDiscreteDynamicsWorld* world() { return m_world.get(); }
    const btDiscreteDynamicsWorld* world() const { return m_world.get(); }

    // Debug draw (optional, can be enabled in ImGui)
    void setDebugDraw(btIDebugDraw* drawer);

    // Stats
    int bodyCount() const;

private:
    // Bullet components (owned)
    std::unique_ptr<btDefaultCollisionConfiguration> m_collisionConfig;
    std::unique_ptr<btCollisionDispatcher> m_dispatcher;
    std::unique_ptr<btBroadphaseInterface> m_broadphase;
    std::unique_ptr<btSequentialImpulseConstraintSolver> m_solver;
    std::unique_ptr<btDiscreteDynamicsWorld> m_world;

    ContactCallback m_contactCallback;
    float m_accumulator = 0.0f;
    static constexpr float FIXED_TIMESTEP = 1.0f / 60.0f;
    static constexpr int MAX_SUBSTEPS = 3;
};

// ── Collision shape factory helpers ──
namespace shapes {

btBoxShape* createBox(const glm::vec3& halfExtents);
btSphereShape* createSphere(float radius);
btCapsuleShape* createCapsule(float radius, float height);
btCylinderShape* createCylinder(const glm::vec3& halfExtents);
btStaticPlaneShape* createPlane(const glm::vec3& normal, float constant);
btConvexHullShape* createConvexHull(const float* vertices, int stride, int count);

} // namespace shapes

// ── RigidBody factory ──
namespace bodies {

btRigidBody* createStatic(btCollisionShape* shape, const glm::vec3& position,
                           const glm::vec3& rotation = {0, 0, 0});
btRigidBody* createDynamic(btCollisionShape* shape, float mass,
                            const glm::vec3& position,
                            const glm::vec3& rotation = {0, 0, 0});

void setFriction(btRigidBody* body, float friction);
void setRestitution(btRigidBody* body, float restitution);
void setDamping(btRigidBody* body, float linear, float angular);
void setCollisionGroup(btRigidBody* body, short group, short mask);

} // namespace bodies

} // namespace ps
