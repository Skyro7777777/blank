#pragma once

#include "physics/PhysicsWorld.h"
#include <glm/glm.hpp>
#include <memory>

namespace ps {

class PlayerController {
public:
    PlayerController();
    ~PlayerController();

    // Initialize player with capsule collider in the physics world
    bool init(PhysicsWorld& world, const glm::vec3& spawnPos = {0.0f, 1.0f, 3.0f});

    // Update - sync physics to camera, apply movement
    void update(float dt);

    // Movement commands (accumulated, applied in update)
    void moveForward(float amount);
    void moveRight(float amount);
    void setMoveInput(const glm::vec3& worldDirection); // Camera-relative world-space direction
    void jump();
    void setCrouching(bool crouching);

    // Getters
    glm::vec3 position() const;
    glm::vec3 velocity() const;
    float eyeHeight() const;
    bool isGrounded() const;
    bool isCrouching() const { return m_crouching; }
    float speed() const { return m_moveSpeed; }

    // Settings
    void setMoveSpeed(float speed) { m_moveSpeed = speed; }
    void setJumpForce(float force) { m_jumpForce = force; }
    void setSprintMultiplier(float mult) { m_sprintMult = mult; }

    // Access the rigid body
    btRigidBody* body() { return m_body; }
    const btRigidBody* body() const { return m_body; }

    // Cleanup
    void destroy(PhysicsWorld& world);

private:
    void checkGrounded();

    btRigidBody* m_body = nullptr;
    btDiscreteDynamicsWorld* m_world = nullptr;  // Non-owning ref for raycasts
    btCollisionShape* m_shape = nullptr;       // Standing capsule
    btCollisionShape* m_crouchShape = nullptr;  // Crouching capsule
    btDefaultMotionState* m_motionState = nullptr;

    // Movement
    float m_moveSpeed = 7.0f;
    float m_jumpForce = 6.0f;
    float m_sprintMult = 2.0f;
    glm::vec3 m_pendingMove{0.0f};

    // State
    bool m_grounded = false;
    bool m_crouching = false;
    bool m_wasCrouching = false;

    // Capsule dimensions
    static constexpr float STAND_RADIUS = 0.35f;
    static constexpr float STAND_HEIGHT = 1.2f;   // Capsule cylinder height (total = 1.2 + 2*0.35 = 1.9)
    static constexpr float CROUCH_HEIGHT = 0.6f;
    static constexpr float EYE_HEIGHT_STAND = 1.6f;
    static constexpr float EYE_HEIGHT_CROUCH = 1.0f;
};

} // namespace ps
