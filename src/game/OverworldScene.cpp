#include "game/OverworldScene.h"

#include <algorithm>
#include <cctype>
#include <filesystem>
#include <iostream>
#include <sstream>
#include <utility>
#include <vector>

#include <SDL3/SDL_keyboard.h>
#include <SDL3/SDL_keycode.h>

#include "engine/ecs/Component.h"
#include "engine/manager/AssetManager.h"
#include "engine/utils/Collision.h"
#include "game/scripting/LuaOverworldScriptLoader.h"

namespace {
std::vector<std::string> splitTokens(const std::string& input) {
    std::vector<std::string> tokens;
    std::istringstream stream(input);
    std::string token;

    while (stream >> token) {
        tokens.push_back(token);
    }

    return tokens;
}

std::vector<std::string> listLuaScriptIds() {
    std::vector<std::string> scriptIds;
    const std::filesystem::path scriptsDirectory = std::filesystem::path("assets") / "scripts";
    if (!std::filesystem::exists(scriptsDirectory) || !std::filesystem::is_directory(scriptsDirectory)) {
        return scriptIds;
    }

    for (const auto& entry : std::filesystem::directory_iterator(scriptsDirectory)) {
        if (!entry.is_regular_file()) {
            continue;
        }

        const std::filesystem::path path = entry.path();
        if (path.extension() != ".lua") {
            continue;
        }

        scriptIds.push_back(path.stem().string());
    }

    std::sort(scriptIds.begin(), scriptIds.end());
    scriptIds.erase(std::unique(scriptIds.begin(), scriptIds.end()), scriptIds.end());
    return scriptIds;
}
} // namespace

OverworldScene::OverworldScene(
    TextureManager& textureManager,
    GameState& gameState,
    const MapRegistry& mapRegistry,
    const std::string& initialMapId,
    const int viewportWidth,
    const int viewportHeight
) : textureManager_(textureManager),
    gameState_(gameState),
    mapRegistry_(mapRegistry),
    viewportWidth_(viewportWidth),
    viewportHeight_(viewportHeight),
    tilemapRenderer_(textureManager, nullptr, nullptr) {
    if (!loadMap(initialMapId, "default")) {
        std::cout << "Failed to load initial map id: " << initialMapId << '\n';
    }

    registerDefaultScripts();
    refreshInputState();
}

void OverworldScene::handleEvent(const SDL_Event& event) {
    if (event.type == SDL_EVENT_KEY_DOWN && !event.key.repeat && event.key.key == SDLK_GRAVE) {
        setDebugConsoleOpen(!debugConsoleOpen_);
        return;
    }

    if (!debugConsoleOpen_) {
        return;
    }

    if (event.type == SDL_EVENT_KEY_DOWN && !event.key.repeat) {
        if (event.key.key == SDLK_ESCAPE) {
            setDebugConsoleOpen(false);
            return;
        }

        if (event.key.key == SDLK_BACKSPACE) {
            if (!debugConsoleInput_.empty()) {
                debugConsoleInput_.pop_back();
            }
            return;
        }

        if (event.key.key == SDLK_RETURN || event.key.key == SDLK_KP_ENTER) {
            executeConsoleCommand(debugConsoleInput_);
            debugConsoleInput_.clear();
            return;
        }
    }

    if (event.type == SDL_EVENT_TEXT_INPUT && event.text.text && event.text.text[0] != '\0') {
        debugConsoleInput_ += event.text.text;
    }
}

void OverworldScene::update(const float dt) {
    OverworldScriptRunner::Runtime scriptRuntime{
        gameState_,
        [this](const std::string& message) {
            printConsole("[script] " + message);
        },
        [this](const bool isInputEnabled) {
            setScriptInputEnabled(isInputEnabled);
        },
        [this](const std::string& mapId, const std::string& spawnId) {
            return warpTo(mapId, spawnId);
        },
        [this](const std::string& mapId, const Vector2D& spawnPoint) {
            return warpTo(mapId, spawnPoint);
        },
        [this](const Vector2D& position) {
            return teleportPlayer(position);
        }
    };
    scriptRunner_.update(dt, scriptRuntime);

    gridMovementSystem_.update(world_.entities(), dt);
    world_.update(dt);
    resolveCollisions();
    checkMapWarps(dt);
    world_.updateCamera();
}

void OverworldScene::render() {
    const Entity* cameraEntity = world_.findFirstWith<Camera>();
    if (cameraEntity) {
        const auto& camera = cameraEntity->getComponent<Camera>();
        tilemapRenderer_.renderGround(map_, camera);
    }

    world_.render(textureManager_);

    if (cameraEntity) {
        const auto& camera = cameraEntity->getComponent<Camera>();
        tilemapRenderer_.renderCover(map_, camera);
    }
}

bool OverworldScene::warpTo(const std::string& mapId, const std::string& spawnId) {
    return loadMap(mapId, spawnId);
}

bool OverworldScene::warpTo(const std::string& mapId, const Vector2D& spawnPoint) {
    return loadMap(mapId, "default", spawnPoint);
}

bool OverworldScene::teleportPlayer(const Vector2D& pixelPosition) {
    Entity* player = world_.findFirstWith<PlayerTag>();
    if (!player || !player->hasComponent<Transform>()) {
        return false;
    }

    auto& transform = player->getComponent<Transform>();

    float clampedX = pixelPosition.x;
    float clampedY = pixelPosition.y;

    if (player->hasComponent<Collider>()) {
        const auto& collider = player->getComponent<Collider>().rect;
        const float maxX = std::max(0.0f, static_cast<float>(map_.width * map_.tileWidth) - collider.w);
        const float maxY = std::max(0.0f, static_cast<float>(map_.height * map_.tileHeight) - collider.h);
        clampedX = std::clamp(clampedX, 0.0f, maxX);
        clampedY = std::clamp(clampedY, 0.0f, maxY);
    }

    transform.position = Vector2D(clampedX, clampedY);
    transform.oldPosition = transform.position;

    if (player->hasComponent<Velocity>()) {
        auto& velocity = player->getComponent<Velocity>();
        velocity.direction = Vector2D(0.0f, 0.0f);
    }

    if (player->hasComponent<GridMovement>()) {
        auto& gridMovement = player->getComponent<GridMovement>();
        gridMovement.isMoving = false;
        gridMovement.targetPosition = transform.position;
        gridMovement.stepDirection = Vector2D(0.0f, 0.0f);
        gridMovement.bumpTimeRemaining = 0.0f;
        gridMovement.bumpDirection = Vector2D(0.0f, 0.0f);
        gridMovement.applyWalkStartFrame = false;
    }

    return true;
}

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

    createStaticCollisionEntities();
    createCameraEntity(viewportWidth_, viewportHeight_);

    const Vector2D spawnPoint = spawnOverride.has_value() ? *spawnOverride : map_.getSpawnPoint(spawnId);
    createPlayerEntity(spawnPoint);
    refreshInputState();

    warpCooldownSeconds_ = 0.2f;
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

void OverworldScene::setScriptInputEnabled(const bool isEnabled) {
    scriptInputLocked_ = !isEnabled;
    refreshInputState();
}

void OverworldScene::refreshInputState() {
    gridMovementSystem_.setInputEnabled(!debugConsoleOpen_ && !scriptInputLocked_);
}

void OverworldScene::setDebugConsoleOpen(const bool isOpen) {
    if (debugConsoleOpen_ == isOpen) {
        return;
    }

    debugConsoleOpen_ = isOpen;
    debugConsoleInput_.clear();

    SDL_Window* const keyboardWindow = SDL_GetKeyboardFocus();
    if (keyboardWindow) {
        if (debugConsoleOpen_) {
            SDL_StartTextInput(keyboardWindow);
        } else {
            SDL_StopTextInput(keyboardWindow);
        }
    }

    refreshInputState();

    if (debugConsoleOpen_) {
        printConsole("Dev console open. Use `help` then Enter.");
    } else {
        printConsole("Dev console closed.");
    }
}

void OverworldScene::executeConsoleCommand(const std::string& commandLine) {
    const std::vector<std::string> tokens = splitTokens(commandLine);
    if (tokens.empty()) {
        return;
    }

    const std::string command = normalizeMapKey(tokens[0]);

    if (command == "help") {
        printConsole("Commands: help, maps, map, spawns, pos, scripts, runscript <id>, stopscript, warp <mapId> [spawnId], warptile <mapId> <tx> <ty>, warpxy <mapId> <x> <y>, goto <tx> <ty>, teleport <x> <y>, setflag <k> <0|1>, getflag <k>, setvar <k> <n>, getvar <k>, reload");
        return;
    }

    if (command == "maps") {
        const std::vector<std::string> ids = mapRegistry_.mapIds();
        if (ids.empty()) {
            printConsole("No maps registered.");
            return;
        }

        std::string line = "Registered maps:";
        for (const std::string& id : ids) {
            line += " " + id;
        }
        printConsole(line);
        return;
    }

    if (command == "map") {
        printConsole("Current map: " + currentMapId_);
        return;
    }

    if (command == "scripts") {
        std::vector<std::string> scriptIds = listLuaScriptIds();
        for (const auto& [_, script] : scripts_) {
            scriptIds.push_back(script.id);
        }

        std::sort(scriptIds.begin(), scriptIds.end());
        scriptIds.erase(std::unique(scriptIds.begin(), scriptIds.end()), scriptIds.end());

        if (scriptIds.empty()) {
            printConsole("No scripts registered.");
            return;
        }

        std::string line = "Scripts:";
        for (const std::string& scriptId : scriptIds) {
            line += " " + scriptId;
        }

        printConsole(line);
        return;
    }

    if (command == "runscript") {
        if (tokens.size() != 2) {
            printConsole("Usage: runscript <scriptId>");
            return;
        }

        if (!runScript(tokens[1])) {
            printConsole("Script not found: " + tokens[1]);
            return;
        }

        return;
    }

    if (command == "stopscript") {
        if (!scriptRunner_.isRunning()) {
            printConsole("No script is running.");
            return;
        }

        scriptRunner_.stop();
        setScriptInputEnabled(true);
        printConsole("Script stopped.");
        return;
    }

    if (command == "spawns") {
        if (map_.spawnPoints.empty()) {
            printConsole("No spawn points found on this map.");
            return;
        }

        std::vector<std::string> spawnIds;
        spawnIds.reserve(map_.spawnPoints.size());
        for (const auto& [id, _] : map_.spawnPoints) {
            spawnIds.push_back(id);
        }
        std::sort(spawnIds.begin(), spawnIds.end());

        std::string line = "Spawn ids:";
        for (const std::string& id : spawnIds) {
            line += " " + id;
        }
        printConsole(line);
        return;
    }

    if (command == "pos") {
        const Entity* player = world_.findFirstWith<PlayerTag>();
        if (!player || !player->hasComponent<Transform>()) {
            printConsole("Player entity not found.");
            return;
        }

        const auto& position = player->getComponent<Transform>().position;
        const int tileX = map_.tileWidth > 0 ? static_cast<int>(position.x / static_cast<float>(map_.tileWidth)) : 0;
        const int tileY = map_.tileHeight > 0 ? static_cast<int>(position.y / static_cast<float>(map_.tileHeight)) : 0;
        std::ostringstream stream;
        stream << "Map " << currentMapId_
               << " | px(" << position.x << ", " << position.y << ")"
               << " tile(" << tileX << ", " << tileY << ")";
        printConsole(stream.str());
        return;
    }

    if (command == "reload") {
        if (!loadMap(currentMapId_, "default")) {
            printConsole("Reload failed.");
            return;
        }

        printConsole("Reloaded map: " + currentMapId_);
        return;
    }

    if (command == "warp") {
        if (tokens.size() < 2 || tokens.size() > 3) {
            printConsole("Usage: warp <mapId> [spawnId]");
            return;
        }

        const std::string spawnId = tokens.size() >= 3 ? tokens[2] : "default";
        if (!warpTo(tokens[1], spawnId)) {
            printConsole("Warp failed.");
            return;
        }

        printConsole("Warped to " + currentMapId_ + " spawn " + spawnId);
        return;
    }

    if (command == "warptile") {
        if (tokens.size() != 4) {
            printConsole("Usage: warptile <mapId> <tileX> <tileY>");
            return;
        }

        int tileX = 0;
        int tileY = 0;
        if (!parseIntToken(tokens[2], tileX) || !parseIntToken(tokens[3], tileY)) {
            printConsole("warptile expects integer tile coordinates.");
            return;
        }

        const Vector2D spawnPoint(
            static_cast<float>(tileX * map_.tileWidth),
            static_cast<float>(tileY * map_.tileHeight)
        );
        if (!warpTo(tokens[1], spawnPoint)) {
            printConsole("Warp failed.");
            return;
        }

        std::ostringstream stream;
        stream << "Warped to " << currentMapId_ << " tile(" << tileX << ", " << tileY << ")";
        printConsole(stream.str());
        return;
    }

    if (command == "warpxy") {
        if (tokens.size() != 4) {
            printConsole("Usage: warpxy <mapId> <x> <y>");
            return;
        }

        float x = 0.0f;
        float y = 0.0f;
        if (!parseFloatToken(tokens[2], x) || !parseFloatToken(tokens[3], y)) {
            printConsole("warpxy expects numeric x/y.");
            return;
        }

        if (!warpTo(tokens[1], Vector2D(x, y))) {
            printConsole("Warp failed.");
            return;
        }

        std::ostringstream stream;
        stream << "Warped to " << currentMapId_ << " px(" << x << ", " << y << ")";
        printConsole(stream.str());
        return;
    }

    if (command == "goto") {
        if (tokens.size() != 3) {
            printConsole("Usage: goto <tileX> <tileY>");
            return;
        }

        int tileX = 0;
        int tileY = 0;
        if (!parseIntToken(tokens[1], tileX) || !parseIntToken(tokens[2], tileY)) {
            printConsole("goto expects integer tile coordinates.");
            return;
        }

        const Vector2D destination(
            static_cast<float>(tileX * map_.tileWidth),
            static_cast<float>(tileY * map_.tileHeight)
        );
        if (!teleportPlayer(destination)) {
            printConsole("Teleport failed.");
            return;
        }

        std::ostringstream stream;
        stream << "Teleported to tile(" << tileX << ", " << tileY << ")";
        printConsole(stream.str());
        return;
    }

    if (command == "teleport") {
        if (tokens.size() != 3) {
            printConsole("Usage: teleport <x> <y>");
            return;
        }

        float x = 0.0f;
        float y = 0.0f;
        if (!parseFloatToken(tokens[1], x) || !parseFloatToken(tokens[2], y)) {
            printConsole("teleport expects numeric x/y.");
            return;
        }

        if (!teleportPlayer(Vector2D(x, y))) {
            printConsole("Teleport failed.");
            return;
        }

        std::ostringstream stream;
        stream << "Teleported to px(" << x << ", " << y << ")";
        printConsole(stream.str());
        return;
    }

    if (command == "setflag") {
        if (tokens.size() != 3) {
            printConsole("Usage: setflag <key> <0|1|true|false>");
            return;
        }

        bool flagValue = false;
        if (!parseBoolToken(tokens[2], flagValue)) {
            printConsole("setflag expects 0/1/true/false/on/off.");
            return;
        }

        gameState_.setFlag(tokens[1], flagValue);
        printConsole("Flag set: " + tokens[1]);
        return;
    }

    if (command == "getflag") {
        if (tokens.size() != 2) {
            printConsole("Usage: getflag <key>");
            return;
        }

        printConsole("Flag " + tokens[1] + " = " + (gameState_.getFlag(tokens[1]) ? "true" : "false"));
        return;
    }

    if (command == "setvar") {
        if (tokens.size() != 3) {
            printConsole("Usage: setvar <key> <int>");
            return;
        }

        int value = 0;
        if (!parseIntToken(tokens[2], value)) {
            printConsole("setvar expects an integer value.");
            return;
        }

        gameState_.setVar(tokens[1], value);
        printConsole("Var set: " + tokens[1]);
        return;
    }

    if (command == "getvar") {
        if (tokens.size() != 2) {
            printConsole("Usage: getvar <key>");
            return;
        }

        const int value = gameState_.getVar(tokens[1]);
        std::ostringstream stream;
        stream << "Var " << tokens[1] << " = " << value;
        printConsole(stream.str());
        return;
    }

    printConsole("Unknown command: " + tokens[0]);
}

void OverworldScene::printConsole(const std::string& message) const {
    std::cout << "[dev] " << message << '\n';
}

void OverworldScene::createStaticCollisionEntities() {
    for (const SDL_FRect& rect : map_.blockingRects) {
        Entity& wall = world_.createEntity();

        wall.addComponent<Transform>(Vector2D(rect.x, rect.y), 0.0f, 1.0f);

        auto& collider = wall.addComponent<Collider>();
        collider.tag = "wall";
        collider.rect = rect;
    }
}

void OverworldScene::createCameraEntity(const int viewportWidth, const int viewportHeight) {
    Entity& cameraEntity = world_.createEntity();

    SDL_FRect cameraView{};
    cameraView.w = static_cast<float>(viewportWidth);
    cameraView.h = static_cast<float>(viewportHeight);

    cameraEntity.addComponent<Camera>(
        cameraView,
        static_cast<float>(map_.width * map_.tileWidth),
        static_cast<float>(map_.height * map_.tileHeight)
    );
}

void OverworldScene::createPlayerEntity(const Vector2D& spawnPoint) {
    if (!AssetManager::hasAnimation("player")) {
        AssetManager::loadAnimation("player", "assets/animations/wes_overworld.xml");
    }

    Entity& player = world_.createEntity();

    auto& transform = player.addComponent<Transform>(spawnPoint, 0.0f, 1.0f);
    player.addComponent<Velocity>(Vector2D(0.0f, 0.0f), 80.0f);
    player.addComponent<GridMovement>(static_cast<float>(map_.tileWidth), false, spawnPoint);

    const Animation animation = AssetManager::getAnimation("player");
    auto& anim = player.addComponent<Animation>(animation);
    anim.currentClip = "idle_down";
    anim.baseSpeed = 8.0f / 60.0f;
    anim.speed = anim.baseSpeed;

    SDL_FRect src{0.0f, 0.0f, 16.0f, 32.0f};
    const auto clipIt = anim.clips.find(anim.currentClip);
    if (clipIt != anim.clips.end() && !clipIt->second.frameIndices.empty()) {
        src = clipIt->second.frameIndices[0];
    }

    SDL_FRect dst{transform.position.x, transform.position.y, 16.0f, 32.0f};
    auto& sprite = player.addComponent<Sprite>(textureManager_.load("assets/characters/wes/overworld/wes_overworld.png"), src, dst);
    sprite.offset = Vector2D(0.0f, -16.0f);
    sprite.mirrorOnRightClip = true;

    auto& collider = player.addComponent<Collider>();
    collider.tag = "player";
    collider.rect.w = static_cast<float>(map_.tileWidth);
    collider.rect.h = static_cast<float>(map_.tileHeight);
    collider.rect.x = transform.position.x;
    collider.rect.y = transform.position.y;

    player.addComponent<PlayerTag>();
}

void OverworldScene::resolveCollisions() {
    for (const CollisionPair& pair : world_.collisions()) {
        if (!pair.entityA || !pair.entityB) {
            continue;
        }

        if (!pair.entityA->hasComponent<Collider>() || !pair.entityB->hasComponent<Collider>()) {
            continue;
        }

        auto& colliderA = pair.entityA->getComponent<Collider>();
        auto& colliderB = pair.entityB->getComponent<Collider>();

        Entity* player = nullptr;
        if (colliderA.tag == "player" && colliderB.tag == "wall") {
            player = pair.entityA;
        } else if (colliderA.tag == "wall" && colliderB.tag == "player") {
            player = pair.entityB;
        }

        if (!player || !player->hasComponent<Transform>()) {
            continue;
        }

        auto& transform = player->getComponent<Transform>();
        transform.position = transform.oldPosition;
    }
}

bool OverworldScene::parseBoolToken(const std::string& token, bool& valueOut) {
    std::string normalized = token;
    std::transform(normalized.begin(), normalized.end(), normalized.begin(), [](const unsigned char ch) {
        return static_cast<char>(std::tolower(ch));
    });

    if (normalized == "1" || normalized == "true" || normalized == "on" || normalized == "yes") {
        valueOut = true;
        return true;
    }

    if (normalized == "0" || normalized == "false" || normalized == "off" || normalized == "no") {
        valueOut = false;
        return true;
    }

    return false;
}

bool OverworldScene::parseIntToken(const std::string& token, int& valueOut) {
    try {
        std::size_t parsedChars = 0;
        const int value = std::stoi(token, &parsedChars);
        if (parsedChars != token.size()) {
            return false;
        }

        valueOut = value;
        return true;
    } catch (...) {
        return false;
    }
}

bool OverworldScene::parseFloatToken(const std::string& token, float& valueOut) {
    try {
        std::size_t parsedChars = 0;
        const float value = std::stof(token, &parsedChars);
        if (parsedChars != token.size()) {
            return false;
        }

        valueOut = value;
        return true;
    } catch (...) {
        return false;
    }
}
