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
    // Some warps should only work when the player steps into them from one
    // side, like walking up into a door or across a stair tile.
    std::string requiredDirection;
    Vector2D targetPosition{};
    bool hasTargetPosition = false;
};

struct SceneSpawnRequest {
    std::string spawnId = "default";
    Vector2D position{};
    bool hasPosition = false;
    // This carries the player's facing into the next map so they keep the same
    // sense of direction after a doorway or route change.
    Vector2D facingDirection{0.0f, 1.0f};
};

struct SceneChangeRequest {
    std::string sceneName;
    SceneSpawnRequest spawn;
};

class WaypointSystem {
public:
    // This picks the player's starting spot from the map's named spawn list so
    // doors and route exits can decide the landing tile without hard-coded C++.
    Vector2D resolvePlayerStart(const std::vector<SpawnPoint>& spawnPoints, const SceneSpawnRequest& spawnRequest) const {
        const Vector2D fallbackStart(192.0f, 288.0f);
        if (spawnRequest.hasPosition) {
            return spawnRequest.position;
        }

        const Vector2D defaultSpawn = findSpawn(spawnPoints, "default", fallbackStart);
        return findSpawn(spawnPoints, spawnRequest.spawnId, defaultSpawn);
    }

    // This remembers whether the player began on top of a warp so entering a
    // room does not immediately send them back through the same doorway.
    void beginScene(const SDL_FRect& playerRect, const std::vector<WarpPoint>& warps, const Vector2D& entryDirection) {
        const bool spawnedOnWarp = findTouchedWarp(playerRect, warps) != nullptr;
        // Spawning on a warp tile should not instantly fire it again. We keep
        // the direction that led into this map so only walking back through
        // that doorway will re-arm the warp while the player is still on it.
        requireWarpTileExit = spawnedOnWarp;
        warpEntryDirection = spawnedOnWarp ? entryDirection : Vector2D(0.0f, 0.0f);
        if (warpEntryDirection == Vector2D(0.0f, 0.0f)) {
            warpEntryDirection = Vector2D(0.0f, 1.0f);
        }
    }

    // This either fires an instant warp on first touch or, for door-style
    // warps, waits until the player is fully standing on that tile.
    bool update(
        const SDL_FRect& playerRect,
        Vector2D inputDirection,
        const Vector2D& facingDirection,
        const std::vector<WarpPoint>& warps,
        SceneChangeRequest& request
    ) {
        const WarpPoint* touchedWarp = findTouchedWarp(playerRect, warps);
        // Movement already ignores diagonal steps, so we collapse held input
        // the same way here before checking a door or stair direction.
        if (inputDirection.x != 0.0f) inputDirection.y = 0.0f;
        // If the player is not pressing a direction right now, keep the last
        // facing so the next map opens with the same orientation.
        const Vector2D warpFacing = inputDirection == Vector2D(0.0f, 0.0f) ? facingDirection : inputDirection;

        if (requireWarpTileExit) {
            if (touchedWarp) {
                // While the player is still standing on the arrival warp tile,
                // only the opposite direction should let them go back through it.
                if (inputDirection == Vector2D(0.0f, 0.0f)) {
                    return false;
                }

                const float dot = (inputDirection.x * warpEntryDirection.x) + (inputDirection.y * warpEntryDirection.y);
                if (dot > -0.5f) {
                    return false;
                }

                if (!matchesRequiredDirection(*touchedWarp, inputDirection)) {
                    return false;
                }

                fillSceneChange(*touchedWarp, warpFacing, request);
                return true;
            }

            requireWarpTileExit = false;
            warpEntryDirection = Vector2D(0.0f, 0.0f);
        }

        if (!touchedWarp) return false;

        if (shouldWaitForFullStep(playerRect, *touchedWarp)) {
            // Single-tile doors and stairs should wait until the player has
            // actually arrived on the tile, not the first frame they overlap it.
            return false;
        }

        if (!touchedWarp->requiredDirection.empty()) {
            // Door and stair warps should only fire when the player is on
            // the tile and actively trying to move the required way.
            if (!matchesRequiredDirection(*touchedWarp, inputDirection)) {
                return false;
            }
        }

        fillSceneChange(*touchedWarp, warpFacing, request);
        return true;
    }

private:
    bool requireWarpTileExit = false;
    Vector2D warpEntryDirection{};

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

    static void fillSceneChange(const WarpPoint& warp, const Vector2D& warpFacing, SceneChangeRequest& request) {
        request.sceneName = warp.targetMap;
        request.spawn.spawnId = warp.targetSpawnId.empty() ? "default" : warp.targetSpawnId;
        request.spawn.position = warp.targetPosition;
        request.spawn.hasPosition = warp.hasTargetPosition;
        // Carry the direction that actually activated the warp so the next map
        // opens with the player still facing through that doorway or route.
        request.spawn.facingDirection = warpFacing;
    }

    static bool shouldWaitForFullStep(const SDL_FRect& playerRect, const WarpPoint& warp) {
        // Doorways are a single tile, so we wait until the player is fully on
        // that tile before the map change can happen.
        const bool isSingleTileWarp = warp.rect.w <= playerRect.w + 0.01f && warp.rect.h <= playerRect.h + 0.01f;
        return isSingleTileWarp && (playerRect.x != warp.rect.x || playerRect.y != warp.rect.y);
    }

    static bool matchesRequiredDirection(const WarpPoint& warp, const Vector2D& requestedDirection) {
        if (warp.requiredDirection.empty()) {
            return true;
        }

        if (warp.requiredDirection == "left") return requestedDirection.x < 0.0f;
        if (warp.requiredDirection == "right") return requestedDirection.x > 0.0f;
        if (warp.requiredDirection == "up") return requestedDirection.y < 0.0f;
        if (warp.requiredDirection == "down") return requestedDirection.y > 0.0f;

        return true;
    }
};
