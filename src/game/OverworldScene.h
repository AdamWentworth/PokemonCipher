#pragma once

#include <SDL3/SDL_events.h>

#include <functional>
#include <deque>
#include <memory>
#include <optional>
#include <string>
#include <unordered_map>

#include "game/dev/OverworldDevConsole.h"
#include "game/encounters/WildEncounterService.h"
#include "engine/Map.h"
#include "engine/TextureManager.h"
#include "engine/TilemapRenderer.h"
#include "engine/ecs/World.h"
#include "engine/manager/Scene.h"
#include "game/events/OverworldScript.h"
#include "game/state/GameState.h"
#include "game/systems/GridMovementSystem.h"
#include "game/ui/DialogueOverlay.h"
#include "game/ui/PartyMenuOverlay.h"
#include "game/ui/PokemonSummaryOverlay.h"
#include "game/ui/StartMenuOverlay.h"
#include "game/world/MapRegistry.h"

class OverworldScene : public Scene {
public:
    OverworldScene(
        TextureManager& textureManager,
        GameState& gameState,
        const MapRegistry& mapRegistry,
        const std::string& initialMapId,
        std::function<bool(const std::string&, const Vector2D&)> saveGameCallback,
        std::function<void(const WildEncounter&, const std::string&, const Vector2D&)> encounterCallback,
        int viewportWidth,
        int viewportHeight
    );

    void handleEvent(const SDL_Event& event) override;
    void update(float dt) override;
    void render() override;

    bool warpTo(const std::string& mapId, const std::string& spawnId = "default");
    bool warpTo(const std::string& mapId, const Vector2D& spawnPoint);
    bool teleportPlayer(const Vector2D& pixelPosition);
    bool runScript(const std::string& scriptId);
    const std::string& currentMapId() const { return currentMapId_; }

private:
    struct PendingMapWarp {
        std::string targetMap;
        std::string targetSpawnId = "default";
        std::optional<Vector2D> targetSpawnOverride;
        Vector2D entryDirection{};
    };

    enum class WarpTransitionPhase {
        None,
        FadeOut,
        FadeIn,
    };

    void createDevConsole();
    bool tryInteractWithNpc();
    void renderDevConsoleOverlay() const;
    bool triggerWildEncounter(const std::string& tableId);
    void checkEncounterZones(float dt);
    void syncEncounterTrackingToPlayer();
    void registerDefaultScripts();
    void setScriptInputEnabled(bool isEnabled);
    void refreshInputState();
    void setDebugConsoleOpen(bool isOpen);
    void printConsole(const std::string& message);
    Vector2D playerFacingDirection() const;
    bool startTransientScript(OverworldScript script);
    bool consumeScriptAdvanceRequested();

    bool loadMap(
        const std::string& mapId,
        const std::string& spawnId,
        const std::optional<Vector2D>& spawnOverride = std::nullopt
    );
    void queueMapWarp(
        std::string targetMap,
        std::string targetSpawnId,
        std::optional<Vector2D> targetSpawnOverride,
        Vector2D entryDirection
    );
    void updateWarpTransition(float dt);
    void renderWarpTransitionOverlay() const;
    bool isWarpTransitionActive() const { return warpTransitionPhase_ != WarpTransitionPhase::None; }
    void applyPlayerFacing(Vector2D facingDirection);
    void checkMapWarps(float dt);
    static std::string normalizeMapKey(std::string key);

    TextureManager& textureManager_;
    GameState& gameState_;
    const MapRegistry& mapRegistry_;
    std::string currentMapId_;
    int viewportWidth_ = 0;
    int viewportHeight_ = 0;
    float warpCooldownSeconds_ = 0.0f;
    bool requireWarpTileExit_ = false;
    Vector2D warpEntryDirection_{};
    std::optional<PendingMapWarp> pendingMapWarp_;
    WarpTransitionPhase warpTransitionPhase_ = WarpTransitionPhase::None;
    float warpTransitionTimerSeconds_ = 0.0f;
    float warpTransitionDurationSeconds_ = 0.12f;
    bool debugConsoleOpen_ = false;
    std::string debugConsoleInput_;
    std::deque<std::string> debugConsoleHistory_;
    bool scriptInputLocked_ = false;
    bool introScriptChecked_ = false;
    bool scriptAdvanceRequested_ = false;
    float encounterCooldownSeconds_ = 0.0f;
    bool hasEncounterTrackingTile_ = false;
    int encounterTrackingTileX_ = 0;
    int encounterTrackingTileY_ = 0;
    std::function<bool(const std::string&, const Vector2D&)> saveGameCallback_;
    std::function<void(const WildEncounter&, const std::string&, const Vector2D&)> encounterCallback_;
    StartMenuOverlay startMenuOverlay_;
    PartyMenuOverlay partyMenuOverlay_;
    PokemonSummaryOverlay pokemonSummaryOverlay_;

    Map map_;
    TilemapRenderer tilemapRenderer_;
    DialogueOverlay dialogueOverlay_;
    World world_;
    GridMovementSystem gridMovementSystem_;
    WildEncounterService wildEncounterService_;
    std::unique_ptr<OverworldDevConsole> devConsole_;
    OverworldScript transientScript_;
    std::unordered_map<std::string, OverworldScript> scripts_;
    OverworldScriptRunner scriptRunner_;
};
