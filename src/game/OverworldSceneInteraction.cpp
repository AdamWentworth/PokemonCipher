#include "game/OverworldScene.h"

#include <cmath>
#include <string>

#include "engine/ecs/Component.h"
#include "engine/utils/Collision.h"

bool OverworldScene::tryInteractWithNpc() {
    const Entity* player = world_.findFirstWith<PlayerTag>();
    if (!player || !player->hasComponent<Collider>()) {
        return false;
    }

    const Vector2D facing = playerFacingDirection();
    SDL_FRect interactionProbe = player->getComponent<Collider>().rect;
    interactionProbe.x += facing.x * static_cast<float>(map_.tileWidth);
    interactionProbe.y += facing.y * static_cast<float>(map_.tileHeight);

    for (const auto& entityPtr : world_.entities()) {
        if (!entityPtr ||
            !entityPtr->hasComponent<NpcTag>() ||
            !entityPtr->hasComponent<Collider>() ||
            !entityPtr->hasComponent<NpcInteraction>()) {
            continue;
        }

        const auto& npcCollider = entityPtr->getComponent<Collider>().rect;
        if (!Collision::AABB(interactionProbe, npcCollider)) {
            continue;
        }

        const auto& interaction = entityPtr->getComponent<NpcInteraction>();
        if (!interaction.scriptId.empty()) {
            const bool started = runScript(interaction.scriptId);
            if (!started) {
                printConsole("NPC script not found: " + interaction.scriptId);
            }
            return started;
        }

        if (!interaction.dialogue.empty()) {
            OverworldScript script{};
            script.id = interaction.id.empty() ? "__npc_interaction" : ("npc_" + interaction.id);
            script.commands.push_back(OverworldScriptCommand::LockInput());
            script.commands.push_back(OverworldScriptCommand::Dialogue(
                interaction.speaker.empty() ? "NPC" : interaction.speaker,
                interaction.dialogue
            ));
            script.commands.push_back(OverworldScriptCommand::UnlockInput());
            return startTransientScript(std::move(script));
        }

        return true;
    }

    return false;
}

Vector2D OverworldScene::playerFacingDirection() const {
    const Entity* player = world_.findFirstWith<PlayerTag>();
    if (!player) {
        return Vector2D(0.0f, 1.0f);
    }

    if (player->hasComponent<Animation>()) {
        const std::string& clip = player->getComponent<Animation>().currentClip;
        if (clip.find("up") != std::string::npos) {
            return Vector2D(0.0f, -1.0f);
        }
        if (clip.find("left") != std::string::npos) {
            return Vector2D(-1.0f, 0.0f);
        }
        if (clip.find("right") != std::string::npos) {
            return Vector2D(1.0f, 0.0f);
        }
        if (clip.find("down") != std::string::npos) {
            return Vector2D(0.0f, 1.0f);
        }
    }

    if (player->hasComponent<Velocity>()) {
        const Vector2D velocityDirection = player->getComponent<Velocity>().direction;
        constexpr float kDirectionEpsilon = 0.01f;
        const float absX = std::abs(velocityDirection.x);
        const float absY = std::abs(velocityDirection.y);
        if (absX >= absY && absX > kDirectionEpsilon) {
            return velocityDirection.x > 0.0f ? Vector2D(1.0f, 0.0f) : Vector2D(-1.0f, 0.0f);
        }
        if (absY > kDirectionEpsilon) {
            return velocityDirection.y > 0.0f ? Vector2D(0.0f, 1.0f) : Vector2D(0.0f, -1.0f);
        }
    }

    return Vector2D(0.0f, 1.0f);
}

bool OverworldScene::startTransientScript(OverworldScript script) {
    scriptRunner_.stop();
    dialogueOverlay_.hide();
    scriptAdvanceRequested_ = false;
    setScriptInputEnabled(true);

    transientScript_ = std::move(script);
    if (!scriptRunner_.start(&transientScript_)) {
        return false;
    }

    printConsole("Running script: " + transientScript_.id);
    return true;
}
