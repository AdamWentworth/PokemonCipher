#pragma once

#include <string>
#include <vector>

#include <SDL3/SDL.h>

#include "utils/Collision.h"
#include "utils/Vector2D.h"

struct SpawnPoint {
    std::string id;
    Vector2D position;
};

struct WarpPoint {
    SDL_FRect rect{};
    std::string targetMap;
    std::string targetSpawnId;
    Vector2D targetPosition{};
    bool hasTargetPosition = false;
};

struct SceneSpawnRequest {
    std::string spawnId = "default";
    Vector2D position{};
    bool hasPosition = false;
};

struct SceneChangeRequest {
    std::string sceneName;
    SceneSpawnRequest spawn;
};

class WaypointSystem {
public:
    // This lets a map decide where the player should appear after a scene
    // change, instead of hard-coding every doorway in C++.
    Vector2D resolvePlayerStart(const std::vector<SpawnPoint>& spawnPoints, const SceneSpawnRequest& spawnRequest) const {
        const Vector2D fallbackStart(192.0f, 288.0f);
        if (spawnRequest.hasPosition) {
            return spawnRequest.position;
        }

        const Vector2D defaultSpawn = findSpawn(spawnPoints, "default", fallbackStart);
        return findSpawn(spawnPoints, spawnRequest.spawnId, defaultSpawn);
    }

    // This remembers whether the player started on a warp tile so entering a
    // building does not instantly send them back out again.
    void beginScene(const SDL_FRect& playerRect, const std::vector<WarpPoint>& warps) {
        playerWasTouchingWarp = findTouchedWarp(playerRect, warps) != nullptr;
    }

    // This is the simplest useful warp rule: once the player steps onto a new
    // warp tile, switch scenes and carry over the target spawn point.
    bool update(const SDL_FRect& playerRect, const std::vector<WarpPoint>& warps, SceneChangeRequest& request) {
        const WarpPoint* touchedWarp = findTouchedWarp(playerRect, warps);
        const bool isTouchingWarp = touchedWarp != nullptr;

        if (touchedWarp && !playerWasTouchingWarp) {
            request.sceneName = touchedWarp->targetMap;
            request.spawn.spawnId = touchedWarp->targetSpawnId.empty() ? "default" : touchedWarp->targetSpawnId;
            request.spawn.position = touchedWarp->targetPosition;
            request.spawn.hasPosition = touchedWarp->hasTargetPosition;
            playerWasTouchingWarp = true;
            return true;
        }

        playerWasTouchingWarp = isTouchingWarp;
        return false;
    }

private:
    bool playerWasTouchingWarp = false;

    static Vector2D findSpawn(const std::vector<SpawnPoint>& spawnPoints, const std::string& spawnId, const Vector2D& fallback) {
        for (const auto& spawn : spawnPoints) {
            if (spawn.id == spawnId) {
                return spawn.position;
            }
        }

        return fallback;
    }

    static const WarpPoint* findTouchedWarp(const SDL_FRect& playerRect, const std::vector<WarpPoint>& warps) {
        for (const auto& warp : warps) {
            if (Collision::AABB(playerRect, warp.rect)) {
                return &warp;
            }
        }

        return nullptr;
    }
};
