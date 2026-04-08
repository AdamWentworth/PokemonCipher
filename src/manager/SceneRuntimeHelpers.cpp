#include "SceneRuntimeHelpers.h"

#include <SDL3/SDL_keycode.h>

namespace SceneRuntimeHelpers {
Entity* findPlayer(World& world) {
    for (auto& entity : world.getEntities()) {
        if (entity && entity->hasComponent<PlayerTag>() && entity->hasComponent<Collider>()) {
            return entity.get();
        }
    }

    return nullptr;
}

// The current clip name already tells us which way the player is facing, so we
// can reuse that instead of storing a second copy of the same idea in movement.
Vector2D facingForClip(const std::string& clipName) {
    if (clipName.find("right") != std::string::npos) return Vector2D(1.0f, 0.0f);
    if (clipName.find("left") != std::string::npos) return Vector2D(-1.0f, 0.0f);
    if (clipName.find("up") != std::string::npos) return Vector2D(0.0f, -1.0f);
    return Vector2D(0.0f, 1.0f);
}

// These keys match the usual Pokemon-style confirm button so the player can
// reuse the same habit later for signs, NPCs, and dialogue boxes.
bool isInteractionKey(const SDL_Event& event) {
    if (event.type != SDL_EVENT_KEY_DOWN || event.key.repeat) {
        return false;
    }

    return event.key.key == SDLK_RETURN ||
        event.key.key == SDLK_KP_ENTER ||
        event.key.key == SDLK_SPACE ||
        event.key.key == SDLK_Z;
}

// These keys match the usual menu button idea, and checking only key-down
// events keeps the menu from flickering open and shut while a key is held.
bool isStartMenuKey(const SDL_Event& event) {
    if (event.type != SDL_EVENT_KEY_DOWN || event.key.repeat) {
        return false;
    }

    return event.key.key == SDLK_ESCAPE ||
        event.key.key == SDLK_TAB ||
        event.key.key == SDLK_RETURN;
}
}
