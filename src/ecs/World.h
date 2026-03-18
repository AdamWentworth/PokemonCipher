#pragma once

#include <algorithm>
#include <memory>
#include <vector>

#include <SDL3/SDL_events.h>

#include "Entity.h"
#include "systems/KeyboardInputSystem.h"
#include "systems/MovementSystem.h"
#include "systems/RenderSystem.h"
#include "systems/CollisionSystem.h"
#include "systems/AnimationSystem.h"
#include "systems/CameraSystem.h"
#include "systems/SpawnTimerSystem.h"
#include "systems/DestructionSystem.h"
#include "EventManager.h"
#include "Map.h"

class World {
    Map map;
    std::vector<std::unique_ptr<Entity>> entities;
    std::vector<std::unique_ptr<Entity>> deferredEntities;
    MovementSystem movementSystem;
    RenderSystem renderSystem;
    KeyboardInputSystem keyboardInputSystem;
    CollisionSystem collisionSystem;
    AnimationSystem animationSystem;
    CameraSystem cameraSystem;
    EventManager eventManager;
    SpawnTimerSystem spawnTimerSystem;
    DestructionSystem destructionSystem;

public:
    World();
    void update(float dt, SDL_Event& event) {
        keyboardInputSystem.update(entities, event);
        movementSystem.update(entities, dt);
        collisionSystem.update(*this);
        animationSystem.update(entities, dt);
        cameraSystem.update(entities);
        spawnTimerSystem.update(entities, dt);
        destructionSystem.update(entities);
        synchronizeEntities();
        cleanup();
    }

    void render() {

        for (auto& entity: entities) {
            if (entity->hasComponent<Camera>()){
                map.draw(entity->getComponent<Camera>());
                break;
            }
        }
        renderSystem.render(entities);
    }

    Entity& createEntity() {
        // we use emplace instead of push so we don't create copy
        entities.emplace_back(std::make_unique<Entity>());
        return *entities.back();
    }

    Entity& createDeferredEntity() {
        deferredEntities.emplace_back(std::make_unique<Entity>());
        return *deferredEntities.back();
    }

    std::vector<std::unique_ptr<Entity>>& getEntities() {
        return entities;
    }

    void cleanup() {
         // use lambda predicate to remove 
        std::erase_if(entities, [](const std::unique_ptr<Entity>& e) {
            return !e || !e->isActive();
        });
    }

    void synchronizeEntities() {
        if (!deferredEntities.empty()){
            std::move(
                deferredEntities.begin(),
                deferredEntities.end(),
                std::back_inserter(entities)
            );
            deferredEntities.clear();
        };
    }

    EventManager& getEventManager() {return eventManager;}
    Map& getMap(){return map;}
};
