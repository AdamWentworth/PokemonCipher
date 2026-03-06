#include "game/OverworldScene.h"

#include <algorithm>
#include <cmath>
#include <random>
#include <sstream>
#include <string>

#include "engine/ecs/Component.h"
#include "engine/utils/Collision.h"
#include "game/events/OverworldScript.h"

namespace {
constexpr float kEncounterCooldownOnTriggerSeconds = 0.25f;
constexpr float kEncounterCooldownOnBlockedSeconds = 0.06f;
constexpr float kTallGrassRustleFrameSeconds = 0.05f;
constexpr int kTallGrassRustleFrameCount = 5;
constexpr std::size_t kMaxTallGrassRustles = 12;

std::mt19937& encounterRng() {
    static std::mt19937 rng{std::random_device{}()};
    return rng;
}

bool isInEncounterZone(const SDL_FRect& playerCollider, const Map& map, std::string& outTableId) {
    for (const EncounterZone& zone : map.encounterZones) {
        if (!Collision::AABB(playerCollider, zone.area)) {
            continue;
        }

        outTableId = zone.tableId.empty() ? "default_grass" : zone.tableId;
        return true;
    }

    return false;
}
} // namespace

void OverworldScene::spawnTallGrassRustle(const int tileX, const int tileY) {
    if (!tallGrassRustleTexture_ || map_.tileWidth <= 0 || map_.tileHeight <= 0) {
        return;
    }

    TallGrassRustle rustle{};
    rustle.tileX = tileX;
    rustle.tileY = tileY;
    rustle.elapsedSeconds = 0.0f;
    rustle.durationSeconds = static_cast<float>(kTallGrassRustleFrameCount) * kTallGrassRustleFrameSeconds;
    tallGrassRustles_.push_back(rustle);

    if (tallGrassRustles_.size() > kMaxTallGrassRustles) {
        const std::size_t overflow = tallGrassRustles_.size() - kMaxTallGrassRustles;
        tallGrassRustles_.erase(tallGrassRustles_.begin(), tallGrassRustles_.begin() + overflow);
    }
}

void OverworldScene::updateTallGrassRustles(const float dt) {
    if (tallGrassRustles_.empty()) {
        return;
    }

    const float clampedDt = std::max(0.0f, dt);
    for (TallGrassRustle& rustle : tallGrassRustles_) {
        rustle.elapsedSeconds += clampedDt;
    }

    tallGrassRustles_.erase(
        std::remove_if(
            tallGrassRustles_.begin(),
            tallGrassRustles_.end(),
            [](const TallGrassRustle& rustle) {
                return rustle.elapsedSeconds >= rustle.durationSeconds;
            }
        ),
        tallGrassRustles_.end()
    );
}

void OverworldScene::renderTallGrassRustles(const Camera& camera) const {
    if (!tallGrassRustleTexture_ || map_.tileWidth <= 0 || map_.tileHeight <= 0) {
        return;
    }

    constexpr float kSourceFrameSize = 16.0f;
    for (const TallGrassRustle& rustle : tallGrassRustles_) {
        const int frame = std::clamp(
            static_cast<int>(rustle.elapsedSeconds / kTallGrassRustleFrameSeconds),
            0,
            kTallGrassRustleFrameCount - 1
        );

        SDL_FRect src{};
        src.x = 0.0f;
        src.y = static_cast<float>(frame) * kSourceFrameSize;
        src.w = kSourceFrameSize;
        src.h = kSourceFrameSize;

        SDL_FRect dst{};
        dst.x = std::round(static_cast<float>(rustle.tileX * map_.tileWidth) - camera.view.x);
        dst.y = std::round(static_cast<float>(rustle.tileY * map_.tileHeight) - camera.view.y);
        dst.w = static_cast<float>(map_.tileWidth);
        dst.h = static_cast<float>(map_.tileHeight);

        textureManager_.draw(tallGrassRustleTexture_, src, dst);
    }
}

bool OverworldScene::triggerWildEncounter(const std::string& tableId) {
    const auto wildEncounter = wildEncounterService_.rollEncounter(tableId, encounterRng());
    if (!wildEncounter.has_value()) {
        printConsole("Encounter table has no entries: " + tableId);
        return false;
    }

    gameState_.setVar("pending_wild_species_id", wildEncounter->speciesId);
    gameState_.setVar("pending_wild_level", wildEncounter->level);
    gameState_.setVar("wild_encounter_count", gameState_.getVar("wild_encounter_count") + 1);

    Vector2D playerPosition{};
    if (const Entity* player = world_.findFirstWith<PlayerTag>();
        player && player->hasComponent<Transform>()) {
        playerPosition = player->getComponent<Transform>().position;
    }

    if (encounterCallback_) {
        encounterCallback_(*wildEncounter, currentMapId_, playerPosition);
        return true;
    }

    std::ostringstream encounterText;
    encounterText << "A wild " << wildEncounter->speciesName << " Lv" << wildEncounter->level << " appeared!";

    OverworldScript script{};
    script.id = "__wild_encounter";
    script.commands.push_back(OverworldScriptCommand::LockInput());
    script.commands.push_back(OverworldScriptCommand::Log(
        "Wild encounter: table=" + wildEncounter->tableId +
        " species=" + std::to_string(wildEncounter->speciesId) +
        " level=" + std::to_string(wildEncounter->level)));
    script.commands.push_back(OverworldScriptCommand::Dialogue("System", encounterText.str()));
    script.commands.push_back(OverworldScriptCommand::UnlockInput());

    return startTransientScript(std::move(script));
}

void OverworldScene::syncEncounterTrackingToPlayer() {
    const Entity* player = world_.findFirstWith<PlayerTag>();
    if (!player || !player->hasComponent<Transform>() || map_.tileWidth <= 0 || map_.tileHeight <= 0) {
        hasEncounterTrackingTile_ = false;
        return;
    }

    const auto& position = player->getComponent<Transform>().position;
    encounterTrackingTileX_ = static_cast<int>(std::floor(position.x / static_cast<float>(map_.tileWidth)));
    encounterTrackingTileY_ = static_cast<int>(std::floor(position.y / static_cast<float>(map_.tileHeight)));
    hasEncounterTrackingTile_ = true;
}

void OverworldScene::checkEncounterZones(const float dt) {
    if (encounterCooldownSeconds_ > 0.0f) {
        encounterCooldownSeconds_ = std::max(0.0f, encounterCooldownSeconds_ - dt);
        return;
    }

    if (debugConsoleOpen_ || startMenuOverlay_.isOpen() || scriptRunner_.isRunning() || map_.encounterZones.empty()) {
        return;
    }

    if (!gameState_.areWildEncountersEnabled()) {
        return;
    }

    const Entity* player = world_.findFirstWith<PlayerTag>();
    if (!player ||
        !player->hasComponent<Transform>() ||
        !player->hasComponent<Collider>() ||
        map_.tileWidth <= 0 ||
        map_.tileHeight <= 0) {
        return;
    }

    const auto& position = player->getComponent<Transform>().position;
    const int tileX = static_cast<int>(std::floor(position.x / static_cast<float>(map_.tileWidth)));
    const int tileY = static_cast<int>(std::floor(position.y / static_cast<float>(map_.tileHeight)));

    if (!hasEncounterTrackingTile_) {
        encounterTrackingTileX_ = tileX;
        encounterTrackingTileY_ = tileY;
        hasEncounterTrackingTile_ = true;
        return;
    }

    if (tileX == encounterTrackingTileX_ && tileY == encounterTrackingTileY_) {
        return;
    }

    encounterTrackingTileX_ = tileX;
    encounterTrackingTileY_ = tileY;

    std::string tableId;
    if (!isInEncounterZone(player->getComponent<Collider>().rect, map_, tableId)) {
        encounterCooldownSeconds_ = kEncounterCooldownOnBlockedSeconds;
        return;
    }
    spawnTallGrassRustle(tileX, tileY);

    const float encounterChance = static_cast<float>(gameState_.wildEncounterRatePercent()) / 100.0f;
    if (encounterChance <= 0.0f) {
        encounterCooldownSeconds_ = kEncounterCooldownOnBlockedSeconds;
        return;
    }

    std::uniform_real_distribution<float> rollDistribution(0.0f, 1.0f);
    if (rollDistribution(encounterRng()) > encounterChance) {
        encounterCooldownSeconds_ = kEncounterCooldownOnBlockedSeconds;
        return;
    }

    if (triggerWildEncounter(tableId)) {
        encounterCooldownSeconds_ = kEncounterCooldownOnTriggerSeconds;
    }
}
