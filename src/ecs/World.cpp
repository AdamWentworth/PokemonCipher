#include "World.h"
#include "Entity.h"
#include "systems/CollisionSystem.h"
#include "Component.h"

//second observer for collision events
//this one is a free function
void CollisionObserver(const CollisionEvent& collision) {
    if (collision.entityA == nullptr || collision.entityB == nullptr) return;

    // std::cout << "A collision occurred between entity A and entity B" << std::endl;
}

World::World() {

    eventManager.subscribe([this](const CollisionEvent& collision) {
        if (collision.entityA == nullptr || collision.entityB == nullptr) return;

        if (!(collision.entityA->hasComponent<Collider>() && collision.entityB->hasComponent<Collider>())) return;

        auto& colliderA = collision.entityA->getComponent<Collider>();
        auto& colliderB = collision.entityB->getComponent<Collider>();

        Entity* player = nullptr;
        Entity* wall = nullptr;

        if (colliderA.tag == "player" && colliderB.tag == "wall") {
            player = collision.entityA;
            wall = collision.entityB;
        } else if (colliderA.tag == "wall" && colliderB.tag == "player") {
            player = collision.entityB;
            wall = collision.entityA;
        }

        if (player && wall) {
           auto& t = player->getComponent<Transform>();
           t.position = t.oldPosition;
        }
    });
}
