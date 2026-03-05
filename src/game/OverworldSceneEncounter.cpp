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

bool OverworldScene::triggerWildEncounter(const std::string& tableId) {
    const auto wildEncounter = wildEncounterService_.rollEncounter(tableId, encounterRng());
    if (!wildEncounter.has_value()) {
        printConsole("Encounter table has no entries: " + tableId);
        return false;
    }

    gameState_.setVar("pending_wild_species_id", wildEncounter->speciesId);
    gameState_.setVar("pending_wild_level", wildEncounter->level);
    gameState_.setVar("wild_encounter_count", gameState_.getVar("wild_encounter_count") + 1);

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
