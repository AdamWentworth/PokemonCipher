#pragma once

#include <algorithm>
#include <functional>
#include <string>
#include <utility>
#include <vector>

#include "engine/utils/Vector2D.h"
#include "game/state/GameState.h"

enum class OverworldScriptCommandType {
    Log,
    Dialogue,
    WaitSeconds,
    LockInput,
    UnlockInput,
    SetFlag,
    SetVar,
    ClearParty,
    AddPartyPokemon,
    WarpToSpawn,
    WarpToPosition,
    TeleportPosition
};

struct OverworldScriptCommand {
    OverworldScriptCommandType type = OverworldScriptCommandType::Log;
    std::string text;
    std::string key;
    std::string secondary;
    float floatValue = 0.0f;
    int intValue = 0;
    int intValueSecondary = 0;
    bool boolValue = false;
    Vector2D position{};

    static OverworldScriptCommand Log(std::string message) {
        OverworldScriptCommand command;
        command.type = OverworldScriptCommandType::Log;
        command.text = std::move(message);
        return command;
    }

    static OverworldScriptCommand Dialogue(std::string speaker, std::string message) {
        OverworldScriptCommand command;
        command.type = OverworldScriptCommandType::Dialogue;
        command.key = std::move(speaker);
        command.text = std::move(message);
        return command;
    }

    static OverworldScriptCommand Wait(const float seconds) {
        OverworldScriptCommand command;
        command.type = OverworldScriptCommandType::WaitSeconds;
        command.floatValue = seconds;
        return command;
    }

    static OverworldScriptCommand LockInput() {
        OverworldScriptCommand command;
        command.type = OverworldScriptCommandType::LockInput;
        return command;
    }

    static OverworldScriptCommand UnlockInput() {
        OverworldScriptCommand command;
        command.type = OverworldScriptCommandType::UnlockInput;
        return command;
    }

    static OverworldScriptCommand SetFlag(std::string flagKey, const bool value) {
        OverworldScriptCommand command;
        command.type = OverworldScriptCommandType::SetFlag;
        command.key = std::move(flagKey);
        command.boolValue = value;
        return command;
    }

    static OverworldScriptCommand SetVar(std::string varKey, const int value) {
        OverworldScriptCommand command;
        command.type = OverworldScriptCommandType::SetVar;
        command.key = std::move(varKey);
        command.intValue = value;
        return command;
    }

    static OverworldScriptCommand ClearParty() {
        OverworldScriptCommand command;
        command.type = OverworldScriptCommandType::ClearParty;
        return command;
    }

    static OverworldScriptCommand AddPartyPokemon(const int speciesId, const int level, const bool isPartner = false) {
        OverworldScriptCommand command;
        command.type = OverworldScriptCommandType::AddPartyPokemon;
        command.intValue = speciesId;
        command.intValueSecondary = level;
        command.boolValue = isPartner;
        return command;
    }

    static OverworldScriptCommand WarpToSpawn(std::string mapId, std::string spawnId) {
        OverworldScriptCommand command;
        command.type = OverworldScriptCommandType::WarpToSpawn;
        command.key = std::move(mapId);
        command.secondary = std::move(spawnId);
        return command;
    }

    static OverworldScriptCommand WarpToPosition(std::string mapId, const Vector2D position) {
        OverworldScriptCommand command;
        command.type = OverworldScriptCommandType::WarpToPosition;
        command.key = std::move(mapId);
        command.position = position;
        return command;
    }

    static OverworldScriptCommand TeleportToPosition(const Vector2D position) {
        OverworldScriptCommand command;
        command.type = OverworldScriptCommandType::TeleportPosition;
        command.position = position;
        return command;
    }
};

struct OverworldScript {
    std::string id;
    std::vector<OverworldScriptCommand> commands;
};

class OverworldScriptRunner {
public:
    struct Runtime {
        GameState& gameState;
        std::function<void(const std::string&)> log;
        std::function<void(const std::string&, const std::string&)> showDialogue;
        std::function<void()> hideDialogue;
        std::function<bool()> consumeAdvanceRequested;
        std::function<void(bool)> setInputEnabled;
        std::function<bool(const std::string&, const std::string&)> warpToSpawn;
        std::function<bool(const std::string&, const Vector2D&)> warpToPosition;
        std::function<bool(const Vector2D&)> teleportToPosition;
    };

    bool start(const OverworldScript* script) {
        if (!script) {
            return false;
        }

        activeScript_ = script;
        currentCommandIndex_ = 0;
        waitRemainingSeconds_ = 0.0f;
        waitingForAdvance_ = false;
        return true;
    }

    void stop() {
        activeScript_ = nullptr;
        currentCommandIndex_ = 0;
        waitRemainingSeconds_ = 0.0f;
        waitingForAdvance_ = false;
    }

    bool isRunning() const {
        return activeScript_ != nullptr;
    }

    const OverworldScript* activeScript() const {
        return activeScript_;
    }

    void update(const float dt, Runtime& runtime) {
        if (!activeScript_) {
            return;
        }

        if (waitingForAdvance_) {
            if (!runtime.consumeAdvanceRequested()) {
                return;
            }

            runtime.hideDialogue();
            waitingForAdvance_ = false;
        }

        if (waitRemainingSeconds_ > 0.0f) {
            waitRemainingSeconds_ = std::max(0.0f, waitRemainingSeconds_ - dt);
            if (waitRemainingSeconds_ > 0.0f) {
                return;
            }
        }

        while (activeScript_) {
            if (currentCommandIndex_ >= activeScript_->commands.size()) {
                stop();
                return;
            }

            const OverworldScriptCommand& command = activeScript_->commands[currentCommandIndex_];
            ++currentCommandIndex_;

            switch (command.type) {
            case OverworldScriptCommandType::Log:
                runtime.log(command.text);
                break;

            case OverworldScriptCommandType::Dialogue:
                runtime.showDialogue(command.key, command.text);
                waitingForAdvance_ = true;
                return;

            case OverworldScriptCommandType::WaitSeconds:
                waitRemainingSeconds_ = std::max(0.0f, command.floatValue);
                if (waitRemainingSeconds_ > 0.0f) {
                    return;
                }
                break;

            case OverworldScriptCommandType::LockInput:
                runtime.setInputEnabled(false);
                break;

            case OverworldScriptCommandType::UnlockInput:
                runtime.setInputEnabled(true);
                break;

            case OverworldScriptCommandType::SetFlag:
                runtime.gameState.setFlag(command.key, command.boolValue);
                break;

            case OverworldScriptCommandType::SetVar:
                runtime.gameState.setVar(command.key, command.intValue);
                break;

            case OverworldScriptCommandType::ClearParty:
                runtime.gameState.clearParty();
                break;

            case OverworldScriptCommandType::AddPartyPokemon:
                runtime.gameState.addPartyPokemon(command.intValue, command.intValueSecondary, command.boolValue);
                break;

            case OverworldScriptCommandType::WarpToSpawn: {
                if (!runtime.warpToSpawn(command.key, command.secondary)) {
                    runtime.log("Script warp failed for map '" + command.key + "'.");
                }
                break;
            }

            case OverworldScriptCommandType::WarpToPosition: {
                if (!runtime.warpToPosition(command.key, command.position)) {
                    runtime.log("Script warp failed for map '" + command.key + "'.");
                }
                break;
            }

            case OverworldScriptCommandType::TeleportPosition:
                if (!runtime.teleportToPosition(command.position)) {
                    runtime.log("Script teleport failed.");
                }
                break;
            }
        }
    }

private:
    const OverworldScript* activeScript_ = nullptr;
    std::size_t currentCommandIndex_ = 0;
    float waitRemainingSeconds_ = 0.0f;
    bool waitingForAdvance_ = false;
};
