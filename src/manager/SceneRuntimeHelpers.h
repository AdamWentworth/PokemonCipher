#pragma once

#include <SDL3/SDL_events.h>
#include <string>

#include "../ecs/World.h"

namespace SceneRuntimeHelpers {
// These small scene-only rules had started to crowd Scene.cpp, so they live
// here now to keep the scene file focused on coordinating the frame flow.
Entity* findPlayer(World& world);
Vector2D facingForClip(const std::string& clipName);
bool isInteractionKey(const SDL_Event& event);
bool isStartMenuKey(const SDL_Event& event);
}
