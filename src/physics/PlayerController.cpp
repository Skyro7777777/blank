#include "physics/PlayerController.h"
#include "physics/CollisionShapes.h"
#include "core/Logger.h"
#include <glm/gtc/quaternion.hpp>

namespace ps {

PlayerController::PlayerController() = default;

PlayerController::~PlayerController() {
    // Note: caller must call destroy() before physics world is destroyed
}

bool PlayerController::init(PhysicsWorld& world, const glm::vec3& spawnPos) {
    // Create standing capsule
    m_shape = shapes::createCapsule(STAND_RADIUS, STAND_HEIGHT);
    // Create crouching capsule (shorter)
    m_crouchShape = shapes::createCapsule(STAND_RADIUS, CROUCH_HEIGHT);

    // Create dynamic body
    float mass = 80.0f; // ~80kg player
    btTransform transform;
    transform.setIdentity();
    transform.setOrigin(btVector3(spawnPos.x, spawnPos.y, spawnPos.z));

    m_motionState = new btDefaultMotionState(transform);

    btVector3 localInertia(0, 0, 0);
    m_shape->calculateLocalInertia(mass, localInertia);

    btRigidBody::btRigidBodyConstructionInfo ci(mass, m_motionState, m_shape, localInertia);
    ci.m_friction = 0.0f;  // No friction - movement is velocity-driven
    ci.m_restitution = 0.0f;
    ci.m_linearDamping = 0.9f;   // Heavy damping for responsive feel
    ci.m_angularDamping = 1.0f;  // Lock rotation

    m_body = new btRigidBody(ci);

    // Prevent capsule from tipping over
    m_body->setAngularFactor(btVector3(0, 0, 0));
    m_body->setActivationState(DISABLE_DEACTIVATION);

    // Store world reference for ground check raycasts
    m_world = world.world();

    // Add to world with player collision group
    world.addRigidBody(m_body, GROUP_PLAYER, Mask::PLAYER);

    LOG_INFO("Player controller initialized at (%.1f, %.1f, %.1f)",
             spawnPos.x, spawnPos.y, spawnPos.z);
    return true;
}

void PlayerController::update(float dt) {
    if (!m_body) return;

    // Check if grounded via downward raycast
    checkGrounded();

    // Handle crouch shape change
    if (m_crouching != m_wasCrouching) {
        btCollisionShape* newShape = m_crouching ? m_crouchShape : m_shape;
        m_body->setCollisionShape(newShape);
        m_wasCrouching = m_crouching;
    }

    // Apply horizontal movement as velocity (not force, for responsive control)
    btVector3 currentVel = m_body->getLinearVelocity();

    // Convert pending move to world-space velocity
    float targetVelX = m_pendingMove.x * m_moveSpeed;
    float targetVelZ = m_pendingMove.z * m_moveSpeed;

    // Only apply horizontal movement; preserve vertical velocity (gravity/jump)
    btVector3 newVel(targetVelX, currentVel.y(), targetVelZ);
    m_body->setLinearVelocity(newVel);

    // Reset pending movement
    m_pendingMove = glm::vec3(0.0f);
}

void PlayerController::moveForward(float amount) {
    m_pendingMove.z -= amount; // -Z is forward in OpenGL
}

void PlayerController::moveRight(float amount) {
    m_pendingMove.x += amount;
}

void PlayerController::setMoveInput(const glm::vec3& worldDir) {
    m_pendingMove += worldDir;
}

void PlayerController::jump() {
    if (!m_body || !m_grounded) return;

    btVector3 vel = m_body->getLinearVelocity();
    vel.setY(m_jumpForce);
    m_body->setLinearVelocity(vel);
    m_grounded = false;
}

void PlayerController::setCrouching(bool crouching) {
    m_crouching = crouching;
}

glm::vec3 PlayerController::position() const {
    if (!m_body) return {0, 0, 0};
    btTransform trans;
    m_body->getMotionState()->getWorldTransform(trans);
    btVector3 pos = trans.getOrigin();
    return {pos.x(), pos.y(), pos.z()};
}

glm::vec3 PlayerController::velocity() const {
    if (!m_body) return {0, 0, 0};
    btVector3 vel = m_body->getLinearVelocity();
    return {vel.x(), vel.y(), vel.z()};
}

float PlayerController::eyeHeight() const {
    return m_crouching ? EYE_HEIGHT_CROUCH : EYE_HEIGHT_STAND;
}

bool PlayerController::isGrounded() const {
    return m_grounded;
}

void PlayerController::checkGrounded() {
    if (!m_body || !m_world) return;

    btTransform trans;
    m_body->getMotionState()->getWorldTransform(trans);
    btVector3 origin = trans.getOrigin();

    // Small downward raycast from below the capsule center
    float capsuleHalfHeight = (m_crouching ? CROUCH_HEIGHT : STAND_HEIGHT) * 0.5f + STAND_RADIUS;
    btVector3 from = origin - btVector3(0, capsuleHalfHeight - 0.05f, 0);
    btVector3 to = from - btVector3(0, 0.15f, 0); // 15cm below feet

    btCollisionWorld::ClosestRayResultCallback callback(from, to);
    // Only check against static objects
    callback.m_collisionFilterGroup = GROUP_PLAYER;
    callback.m_collisionFilterMask = GROUP_STATIC;

    m_world->rayTest(from, to, callback);
    m_grounded = callback.hasHit();
}

void PlayerController::destroy(PhysicsWorld& world) {
    if (m_body) {
        world.removeRigidBody(m_body);
        delete m_body->getMotionState();
        delete m_body;
        m_body = nullptr;
    }
    delete m_shape; m_shape = nullptr;
    delete m_crouchShape; m_crouchShape = nullptr;
}

} // namespace ps
