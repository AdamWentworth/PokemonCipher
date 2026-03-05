#include "game/OverworldScene.h"

#include <algorithm>
#include <cctype>
#include <filesystem>
#include <iostream>
#include <utility>

#include "engine/ecs/Component.h"
#include "engine/utils/Collision.h"
#include "game/scripting/LuaOverworldScriptLoader.h"
#include "game/world/OverworldEntityFactory.h"

bool OverworldScene::runScript(const std::string& scriptId) {
    const std::string scriptKey = normalizeMapKey(scriptId);
    const std::filesystem::path luaScriptPath = std::filesystem::path("assets") / "scripts" / (scriptKey + ".lua");

    if (std::filesystem::exists(luaScriptPath)) {
        OverworldScript loadedScript{};
        std::string loadError;
        if (!loadOverworldScriptFromLuaFile(scriptKey, luaScriptPath.string(), loadedScript, loadError)) {
            printConsole("Lua load error for '" + scriptKey + "': " + loadError);
            return false;
        }

        scripts_[scriptKey] = std::move(loadedScript);
    } else if (scripts_.find(scriptKey) == scripts_.end()) {
        return false;
    }

    const auto it = scripts_.find(scriptKey);
    if (it == scripts_.end()) {
        return false;
    }

    scriptRunner_.stop();
    dialogueOverlay_.hide();
    startMenuOverlay_.close();
    startMenuOverlay_.clearStatusText();
    scriptAdvanceRequested_ = false;
    setScriptInputEnabled(true);

    if (!scriptRunner_.start(&it->second)) {
        return false;
    }

    printConsole("Running script: " + it->second.id);
    return true;
}

bool OverworldScene::loadMap(
    const std::string& mapId,
    const std::string& spawnId,
    const std::optional<Vector2D>& spawnOverride
) {
    const std::string normalizedMapId = normalizeMapKey(mapId);
    const MapDefinition* mapDefinition = mapRegistry_.find(normalizedMapId);
    if (!mapDefinition) {
        std::cout << "Map id '" << mapId << "' is not registered.\n";
        return false;
    }

    if (!map_.load(mapDefinition->mapPath.c_str())) {
        std::cout << "Failed to load map path: " << mapDefinition->mapPath << '\n';
        return false;
    }

    currentMapId_ = mapDefinition->id;
    tilemapRenderer_.setTilesets(
        mapDefinition->baseTilesetPath.c_str(),
        mapDefinition->coverTilesetPath.empty() ? nullptr : mapDefinition->coverTilesetPath.c_str()
    );

    world_.clear();

    gridMovementSystem_.setWorldBounds(
        static_cast<float>(map_.width * map_.tileWidth),
        static_cast<float>(map_.height * map_.tileHeight)
    );

    OverworldEntityFactory::createStaticCollisionEntities(world_, map_);
    OverworldEntityFactory::createCameraEntity(world_, map_, viewportWidth_, viewportHeight_);
    OverworldEntityFactory::createNpcEntities(world_, map_, textureManager_);

    const Vector2D spawnPoint = spawnOverride.has_value() ? *spawnOverride : map_.getSpawnPoint(spawnId);
    OverworldEntityFactory::createPlayerEntity(world_, map_, spawnPoint, textureManager_);
    refreshInputState();

    warpCooldownSeconds_ = 0.2f;
    encounterCooldownSeconds_ = 0.25f;
    syncEncounterTrackingToPlayer();
    return true;
}

void OverworldScene::checkMapWarps(const float dt) {
    if (warpCooldownSeconds_ > 0.0f) {
        warpCooldownSeconds_ = std::max(0.0f, warpCooldownSeconds_ - dt);
        return;
    }

    Entity* player = world_.findFirstWith<PlayerTag>();
    if (!player || !player->hasComponent<Collider>()) {
        return;
    }

    const auto& playerCollider = player->getComponent<Collider>().rect;
    for (const WarpPoint& warp : map_.warpPoints) {
        if (!Collision::AABB(playerCollider, warp.area)) {
            continue;
        }

        std::string targetMap = normalizeMapKey(warp.targetMap);
        if (targetMap.rfind("map_", 0) == 0) {
            targetMap = targetMap.substr(4);
        }

        std::optional<Vector2D> targetSpawnOverride;
        if (warp.targetSpawn.x != 0.0f || warp.targetSpawn.y != 0.0f) {
            targetSpawnOverride = warp.targetSpawn;
        }

        if (!loadMap(targetMap, "default", targetSpawnOverride)) {
            std::cout << "Warp target map not available: " << warp.targetMap << '\n';
        }

        return;
    }
}

std::string OverworldScene::normalizeMapKey(std::string key) {
    std::transform(key.begin(), key.end(), key.begin(), [](const unsigned char ch) {
        return static_cast<char>(std::tolower(ch));
    });
    return key;
}

void OverworldScene::registerDefaultScripts() {
    scripts_.clear();
}
