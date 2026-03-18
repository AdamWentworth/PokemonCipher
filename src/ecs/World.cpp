#include "World.h"
#include "Entity.h"
#include "systems/CollisionSystem.h"
#include "Component.h"
#include "../Game.h"
#include <iostream>

//second observer for collision events
//this one is a free function
void CollisionObserver(const CollisionEvent& collision) {
    if (collision.entityA == nullptr || collision.entityB == nullptr) return;

    // std::cout << "A collision occurred between entity A and entity B" << std::endl;
}

World::World() {

    eventManager.subscribe([this](const CollisionEvent& collision) {

        Entity* sceneStateEntity = nullptr;

        for (auto& e: entities) {
            if (e->hasComponent<SceneState>()) {
                sceneStateEntity = e.get();
                break;
            }
        }

        if (!sceneStateEntity) return;

        if (collision.entityA == nullptr || collision.entityB == nullptr) return;

        if (!(collision.entityA->hasComponent<Collider>() && collision.entityB->hasComponent<Collider>())) return;

        auto& colliderA = collision.entityA->getComponent<Collider>();
        auto& colliderB = collision.entityB->getComponent<Collider>();

        Entity* player = nullptr;
        Entity* item = nullptr;
        Entity* wall = nullptr;
        Entity* projectile = nullptr;

        if (colliderA.tag == "player" && colliderB.tag == "item") {
            player = collision.entityA;
            item = collision.entityB;
        } else if (colliderA.tag == "item" && colliderB.tag == "player") {
            player = collision.entityB;
            item = collision.entityA;
        }

        if (player && item) {
            std::cout << "Player picked up a coin" << std::endl;
            item->destroy();
            auto& sceneState = sceneStateEntity->getComponent<SceneState>();
            sceneState.coinsCollected++;

            bool hasActiveCoins = false;
            for (const auto& e : entities) {
                if (!e || !e->isActive()) continue;
                if (e->hasComponent<Collider>() && e->getComponent<Collider>().tag == "item") {
                    hasActiveCoins = true;
                    break;
                }
            }

            if (!hasActiveCoins) {
                Game::onSceneChangeRequest("level2");
            }
        }

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

        if (colliderA.tag == "player" && colliderB.tag == "projectile") {
            player = collision.entityA;
            projectile = collision.entityB;
        } else if (colliderA.tag == "projectile" && colliderB.tag == "player") {
            player = collision.entityB;
            projectile = collision.entityA;
        }

        if (player && projectile) {
            player->destroy();
            Game::onSceneChangeRequest("gameover");
        }
    });
}
