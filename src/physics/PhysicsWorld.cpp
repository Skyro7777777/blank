#include "physics/PhysicsWorld.h"
#include "core/Logger.h"
#include <BulletCollision/CollisionDispatch/btGhostObject.h>
#include <glm/gtc/quaternion.hpp>
#include <vector>

namespace ps {

PhysicsWorld::PhysicsWorld() = default;

PhysicsWorld::~PhysicsWorld() {
    if (!m_world) return;

    // Remove all bodies before destruction
    for (int i = m_world->getNumCollisionObjects() - 1; i >= 0; --i) {
        btCollisionObject* obj = m_world->getCollisionObjectArray()[i];
        if (auto* body = btRigidBody::upcast(obj)) {
            m_world->removeRigidBody(body);
        } else {
            m_world->removeCollisionObject(obj);
        }
    }
    LOG_INFO("Physics world destroyed");
}

bool PhysicsWorld::init(const glm::vec3& gravity) {
    m_collisionConfig = std::make_unique<btDefaultCollisionConfiguration>();
    m_dispatcher = std::make_unique<btCollisionDispatcher>(m_collisionConfig.get());
    m_broadphase = std::make_unique<btDbvtBroadphase>();
    m_solver = std::make_unique<btSequentialImpulseConstraintSolver>();

    m_world = std::make_unique<btDiscreteDynamicsWorld>(
        m_dispatcher.get(), m_broadphase.get(),
        m_solver.get(), m_collisionConfig.get()
    );

    m_world->setGravity(btVector3(gravity.x, gravity.y, gravity.z));
    LOG_INFO("Physics world initialized (gravity: %.1f, %.1f, %.1f)",
             gravity.x, gravity.y, gravity.z);
    return true;
}

void PhysicsWorld::step(float dt) {
    if (!m_world) return;

    // Fixed timestep with accumulator for stable simulation
    m_accumulator += dt;
    while (m_accumulator >= FIXED_TIMESTEP) {
        m_world->stepSimulation(FIXED_TIMESTEP, 0);
        m_accumulator -= FIXED_TIMESTEP;
    }
    // Interpolate remaining time
    if (m_accumulator > 0.0f) {
        m_world->stepSimulation(m_accumulator, 0);
        m_accumulator = 0.0f;
    }

    // Process contact callbacks
    if (m_contactCallback && m_dispatcher) {
        int numManifolds = m_dispatcher->getNumManifolds();
        for (int i = 0; i < numManifolds; ++i) {
            btPersistentManifold* manifold = m_dispatcher->getManifoldByIndexInternal(i);
            if (manifold->getNumContacts() == 0) continue;

            const btCollisionObject* objA = manifold->getBody0();
            const btCollisionObject* objB = manifold->getBody1();

            ContactInfo info;
            info.bodyA = const_cast<btRigidBody*>(btRigidBody::upcast(objA));
            info.bodyB = const_cast<btRigidBody*>(btRigidBody::upcast(objB));

            btManifoldPoint& pt = manifold->getContactPoint(0);
            info.contactPoint = glm::vec3(pt.getPositionWorldOnB().x(),
                                           pt.getPositionWorldOnB().y(),
                                           pt.getPositionWorldOnB().z());
            info.contactNormal = glm::vec3(pt.m_normalWorldOnB.x(),
                                            pt.m_normalWorldOnB.y(),
                                            pt.m_normalWorldOnB.z());
            m_contactCallback(info);
        }
    }
}

void PhysicsWorld::addRigidBody(btRigidBody* body) {
    if (m_world && body) m_world->addRigidBody(body);
}

void PhysicsWorld::addRigidBody(btRigidBody* body, short group, short mask) {
    if (m_world && body) m_world->addRigidBody(body, group, mask);
}

void PhysicsWorld::removeRigidBody(btRigidBody* body) {
    if (m_world && body) m_world->removeRigidBody(body);
}

void PhysicsWorld::addConstraint(btTypedConstraint* constraint) {
    if (m_world && constraint) m_world->addConstraint(constraint);
}

void PhysicsWorld::removeConstraint(btTypedConstraint* constraint) {
    if (m_world && constraint) m_world->removeConstraint(constraint);
}

void PhysicsWorld::addAction(btActionInterface* action) {
    if (m_world && action) m_world->addAction(action);
}

void PhysicsWorld::removeAction(btActionInterface* action) {
    if (m_world && action) m_world->removeAction(action);
}

PhysicsWorld::RayResult PhysicsWorld::raycast(const glm::vec3& from, const glm::vec3& to,
                                                short collisionMask) const {
    RayResult result;
    if (!m_world) return result;

    btVector3 btFrom(from.x, from.y, from.z);
    btVector3 btTo(to.x, to.y, to.z);
    btCollisionWorld::ClosestRayResultCallback callback(btFrom, btTo);
    callback.m_collisionFilterMask = collisionMask;

    m_world->rayTest(btFrom, btTo, callback);

    if (callback.hasHit()) {
        result.hit = true;
        result.hitPoint = glm::vec3(callback.m_hitPointWorld.x(),
                                     callback.m_hitPointWorld.y(),
                                     callback.m_hitPointWorld.z());
        result.hitNormal = glm::vec3(callback.m_hitNormalWorld.x(),
                                      callback.m_hitNormalWorld.y(),
                                      callback.m_hitNormalWorld.z());
        result.hitFraction = callback.m_closestHitFraction;
        result.body = const_cast<btRigidBody*>(btRigidBody::upcast(callback.m_collisionObject));
        result.shape = callback.m_collisionObject->getCollisionShape();
    }
    return result;
}

std::vector<PhysicsWorld::RayResult> PhysicsWorld::raycastAll(const glm::vec3& from,
                                                                 const glm::vec3& to,
                                                                 short collisionMask) const {
    std::vector<RayResult> results;
    if (!m_world) return results;

    btVector3 btFrom(from.x, from.y, from.z);
    btVector3 btTo(to.x, to.y, to.z);
    btCollisionWorld::AllHitsRayResultCallback callback(btFrom, btTo);
    callback.m_collisionFilterMask = collisionMask;

    m_world->rayTest(btFrom, btTo, callback);

    for (int i = 0; i < callback.m_hitPointWorld.size(); ++i) {
        RayResult r;
        r.hit = true;
        r.hitPoint = glm::vec3(callback.m_hitPointWorld[i].x(),
                                callback.m_hitPointWorld[i].y(),
                                callback.m_hitPointWorld[i].z());
        r.hitNormal = glm::vec3(callback.m_hitNormalWorld[i].x(),
                                 callback.m_hitNormalWorld[i].y(),
                                 callback.m_hitNormalWorld[i].z());
        r.hitFraction = callback.m_hitFractions[i];
        r.body = const_cast<btRigidBody*>(btRigidBody::upcast(callback.m_collisionObjects[i]));
        r.shape = callback.m_collisionObjects[i]->getCollisionShape();
        results.push_back(r);
    }
    return results;
}

void PhysicsWorld::setDebugDraw(btIDebugDraw* drawer) {
    if (m_world) m_world->setDebugDrawer(drawer);
}

int PhysicsWorld::bodyCount() const {
    return m_world ? m_world->getNumCollisionObjects() : 0;
}

// ── Shape factories ──
namespace shapes {

btBoxShape* createBox(const glm::vec3& halfExtents) {
    return new btBoxShape(btVector3(halfExtents.x, halfExtents.y, halfExtents.z));
}

btSphereShape* createSphere(float radius) {
    return new btSphereShape(radius);
}

btCapsuleShape* createCapsule(float radius, float height) {
    return new btCapsuleShape(radius, height);
}

btCylinderShape* createCylinder(const glm::vec3& halfExtents) {
    return new btCylinderShape(btVector3(halfExtents.x, halfExtents.y, halfExtents.z));
}

btStaticPlaneShape* createPlane(const glm::vec3& normal, float constant) {
    return new btStaticPlaneShape(btVector3(normal.x, normal.y, normal.z), constant);
}

btConvexHullShape* createConvexHull(const float* vertices, int stride, int count) {
    auto* shape = new btConvexHullShape();
    for (int i = 0; i < count; ++i) {
        const float* v = vertices + i * stride;
        shape->addPoint(btVector3(v[0], v[1], v[2]));
    }
    return shape;
}

} // namespace shapes

// ── Body factories ──
namespace bodies {

static btVector3 toBt(const glm::vec3& v) { return {v.x, v.y, v.z}; }

btRigidBody* createStatic(btCollisionShape* shape, const glm::vec3& position,
                           const glm::vec3& rotation) {
    btTransform transform;
    transform.setIdentity();
    transform.setOrigin(toBt(position));
    if (rotation != glm::vec3(0)) {
        btQuaternion q;
        q.setEulerZYX(rotation.z, rotation.y, rotation.x);
        transform.setRotation(q);
    }

    btDefaultMotionState* motionState = new btDefaultMotionState(transform);
    btRigidBody::btRigidBodyConstructionInfo ci(0.0f, motionState, shape);
    ci.m_friction = 0.8f;
    ci.m_restitution = 0.2f;

    auto* body = new btRigidBody(ci);
    body->setCollisionFlags(body->getCollisionFlags() | btCollisionObject::CF_STATIC_OBJECT);
    return body;
}

btRigidBody* createDynamic(btCollisionShape* shape, float mass,
                            const glm::vec3& position,
                            const glm::vec3& rotation) {
    btTransform transform;
    transform.setIdentity();
    transform.setOrigin(toBt(position));
    if (rotation != glm::vec3(0)) {
        btQuaternion q;
        q.setEulerZYX(rotation.z, rotation.y, rotation.x);
        transform.setRotation(q);
    }

    btDefaultMotionState* motionState = new btDefaultMotionState(transform);
    btVector3 localInertia(0, 0, 0);
    if (mass > 0.0f) shape->calculateLocalInertia(mass, localInertia);

    btRigidBody::btRigidBodyConstructionInfo ci(mass, motionState, shape, localInertia);
    ci.m_friction = 0.5f;
    ci.m_restitution = 0.3f;

    return new btRigidBody(ci);
}

void setFriction(btRigidBody* body, float friction) {
    if (body) body->setFriction(friction);
}

void setRestitution(btRigidBody* body, float restitution) {
    if (body) body->setRestitution(restitution);
}

void setDamping(btRigidBody* body, float linear, float angular) {
    if (body) body->setDamping(linear, angular);
}

void setCollisionGroup(btRigidBody* body, short group, short mask) {
    // Must re-add to world with new group/mask (caller handles this)
    (void)body; (void)group; (void)mask;
}

} // namespace bodies

} // namespace ps
