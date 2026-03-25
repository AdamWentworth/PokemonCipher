#pragma once
#include <memory>
#include <vector>
#include <SDL3/SDL_events.h>

#include "../Component.h"
#include "../Entity.h"

class KeyboardInputSystem {
    public:
    void update(const std::vector<std::unique_ptr<Entity>>& entities, const SDL_Event& event) {
        for (auto& e : entities) {
            if (e->hasComponent<PlayerTag>() && e->hasComponent<Velocity>()) {
                auto& v = e->getComponent<Velocity>();
                if (event.type == SDL_EVENT_KEY_DOWN) {
                    switch (event.key.key) {
                        case SDLK_W:
                        case SDLK_UP:
                            v.direction.y = -1;
                            break;
                        case SDLK_S:
                        case SDLK_DOWN:
                            v.direction.y = 1;
                            break;
                        case SDLK_A:
                        case SDLK_LEFT:
                            v.direction.x = -1;
                            break;
                        case SDLK_D:
                        case SDLK_RIGHT:
                            v.direction.x = 1;
                            break;
                        default:
                            break;
                    }
                }
                if (event.type == SDL_EVENT_KEY_UP) {
                    switch (event.key.key) {
                        case SDLK_W:
                        case SDLK_UP:
                            v.direction.y = 0;
                            break;
                        case SDLK_S:
                        case SDLK_DOWN:
                            v.direction.y = 0;
                            break;
                        case SDLK_A:
                        case SDLK_LEFT:
                            v.direction.x = 0;
                            break;
                        case SDLK_D:
                        case SDLK_RIGHT:
                            v.direction.x = 0;
                            break;
                        default:
                            break;
                    }
                }
            }
        }
    }
};