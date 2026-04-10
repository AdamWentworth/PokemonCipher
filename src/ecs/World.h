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

public:
    World();
    void update(float dt, SDL_Event& event) {
        keyboardInputSystem.update(entities, event);
        // Movement now also checks the map bounds, so rooms do not need wall
        // objects on every single outer edge just to stop the player.
        movementSystem.update(entities, dt, map);
        collisionSystem.update(*this);
        animationSystem.update(entities, dt);
        cameraSystem.update(entities);
        synchronizeEntities();
        cleanup();
    }

    void render() {
        // We now render the map twice, so we keep the same camera for both
        // passes to avoid the cover layer drifting away from terrain/sprites.
        Camera* activeCamera = nullptr;

        for (auto& entity: entities) {
            if (entity->hasComponent<Camera>()){
                activeCamera = &entity->getComponent<Camera>();
                map.draw(*activeCamera);
                break;
            }
        }
        renderSystem.render(entities);
        if (activeCamera) {
            // This second map pass is the whole reason cover layers work:
            // drawing them last lets roofs/tree tops overlap the player and NPCs.
            map.drawCover(*activeCamera);
        }
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
