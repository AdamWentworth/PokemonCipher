#pragma once

#include <filesystem>
#include <vector>

#include "game/state/GameState.h"

struct SaveSlotData {
    std::string mapId = "pallet_town";
    float playerX = 0.0f;
    float playerY = 0.0f;
    bool introComplete = false;
    bool starterEeveeObtained = false;
    int storyCheckpoint = 0;
    bool textSpeedFast = false;
    bool battleStyleSet = false;
    bool wildEncountersEnabled = true;
    int wildEncounterRatePercent = 16;
    std::vector<PartyPokemon> party;
};

class SaveGameStorage {
public:
    explicit SaveGameStorage(std::filesystem::path rootDirectory = "saves");

    bool hasSlot(int slotIndex) const;
    bool loadSlot(int slotIndex, SaveSlotData& dataOut) const;
    bool writeSlot(int slotIndex, const SaveSlotData& data) const;

private:
    std::filesystem::path slotPath(int slotIndex) const;
    std::filesystem::path rootDirectory_;
};
