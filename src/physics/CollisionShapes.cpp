#include "physics/CollisionShapes.h"

namespace ps {

btRigidBody* createGroundPlane(PhysicsWorld& world, float y) {
    auto* shape = shapes::createPlane({0, 1, 0}, y);
    auto* body = bodies::createStatic(shape, {0, 0, 0});
    body->setFriction(0.9f);
    body->setRestitution(0.1f);
    world.addRigidBody(body, GROUP_STATIC, Mask::STATIC);
    return body;
}

btRigidBody* createStaticBox(PhysicsWorld& world, const glm::vec3& halfExtents,
                              const glm::vec3& position, const glm::vec3& rotation) {
    auto* shape = shapes::createBox(halfExtents);
    auto* body = bodies::createStatic(shape, position, rotation);
    body->setFriction(0.7f);
    world.addRigidBody(body, GROUP_STATIC, Mask::STATIC);
    return body;
}

} // namespace ps
