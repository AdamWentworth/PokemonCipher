#include "game/save/SaveGameStorage.h"

#include <fstream>
#include <sstream>
#include <string>
#include <utility>

namespace {
bool parseBool(const std::string& value, bool& out) {
    if (value == "1" || value == "true") {
        out = true;
        return true;
    }
    if (value == "0" || value == "false") {
        out = false;
        return true;
    }
    return false;
}

bool parseInt(const std::string& value, int& out) {
    try {
        std::size_t parsed = 0;
        const int parsedValue = std::stoi(value, &parsed);
        if (parsed != value.size()) {
            return false;
        }

        out = parsedValue;
        return true;
    } catch (...) {
        return false;
    }
}

bool parseFloat(const std::string& value, float& out) {
    try {
        std::size_t parsed = 0;
        const float parsedValue = std::stof(value, &parsed);
        if (parsed != value.size()) {
            return false;
        }

        out = parsedValue;
        return true;
    } catch (...) {
        return false;
    }
}
} // namespace

SaveGameStorage::SaveGameStorage(std::filesystem::path rootDirectory) : rootDirectory_(std::move(rootDirectory)) {}

bool SaveGameStorage::hasSlot(const int slotIndex) const {
    return std::filesystem::exists(slotPath(slotIndex));
}

bool SaveGameStorage::loadSlot(const int slotIndex, SaveSlotData& dataOut) const {
    std::ifstream file(slotPath(slotIndex));
    if (!file.is_open()) {
        return false;
    }

    SaveSlotData loaded{};
    std::string line;
    while (std::getline(file, line)) {
        const std::size_t equalsPos = line.find('=');
        if (equalsPos == std::string::npos) {
            continue;
        }

        const std::string key = line.substr(0, equalsPos);
        const std::string value = line.substr(equalsPos + 1);

        if (key == "map_id") {
            loaded.mapId = value;
            continue;
        }

        if (key == "player_x") {
            if (!parseFloat(value, loaded.playerX)) {
                return false;
            }
            continue;
        }

        if (key == "player_y") {
            if (!parseFloat(value, loaded.playerY)) {
                return false;
            }
            continue;
        }

        if (key == "intro_complete") {
            if (!parseBool(value, loaded.introComplete)) {
                return false;
            }
            continue;
        }

        if (key == "starter_eevee_obtained") {
            if (!parseBool(value, loaded.starterEeveeObtained)) {
                return false;
            }
            continue;
        }

        if (key == "story_checkpoint") {
            if (!parseInt(value, loaded.storyCheckpoint)) {
                return false;
            }
            continue;
        }

        if (key == "text_speed_fast") {
            if (!parseBool(value, loaded.textSpeedFast)) {
                return false;
            }
            continue;
        }

        if (key == "battle_style_set") {
            if (!parseBool(value, loaded.battleStyleSet)) {
                return false;
            }
            continue;
        }

        if (key == "wild_encounters_enabled") {
            if (!parseBool(value, loaded.wildEncountersEnabled)) {
                return false;
            }
            continue;
        }

        if (key == "wild_encounter_rate_percent") {
            if (!parseInt(value, loaded.wildEncounterRatePercent)) {
                return false;
            }
            continue;
        }

        if (key.rfind("party_", 0) == 0) {
            std::istringstream parser(value);
            std::string speciesToken;
            std::string levelToken;
            std::string partnerToken;
            if (!std::getline(parser, speciesToken, ',')) {
                return false;
            }
            if (!std::getline(parser, levelToken, ',')) {
                return false;
            }
            if (!std::getline(parser, partnerToken, ',')) {
                return false;
            }

            PartyPokemon member{};
            if (!parseInt(speciesToken, member.speciesId)) {
                return false;
            }
            if (!parseInt(levelToken, member.level)) {
                return false;
            }
            if (!parseBool(partnerToken, member.isPartner)) {
                return false;
            }

            loaded.party.push_back(member);
        }
    }

    if (loaded.mapId.empty()) {
        return false;
    }

    dataOut = loaded;
    return true;
}

bool SaveGameStorage::writeSlot(const int slotIndex, const SaveSlotData& data) const {
    std::error_code ec;
    std::filesystem::create_directories(rootDirectory_, ec);

    std::ofstream file(slotPath(slotIndex), std::ios::trunc);
    if (!file.is_open()) {
        return false;
    }

    file << "map_id=" << data.mapId << "\n";
    file << "player_x=" << data.playerX << "\n";
    file << "player_y=" << data.playerY << "\n";
    file << "intro_complete=" << (data.introComplete ? 1 : 0) << "\n";
    file << "starter_eevee_obtained=" << (data.starterEeveeObtained ? 1 : 0) << "\n";
    file << "story_checkpoint=" << data.storyCheckpoint << "\n";
    file << "text_speed_fast=" << (data.textSpeedFast ? 1 : 0) << "\n";
    file << "battle_style_set=" << (data.battleStyleSet ? 1 : 0) << "\n";
    file << "wild_encounters_enabled=" << (data.wildEncountersEnabled ? 1 : 0) << "\n";
    file << "wild_encounter_rate_percent=" << data.wildEncounterRatePercent << "\n";
    for (std::size_t i = 0; i < data.party.size(); ++i) {
        const PartyPokemon& member = data.party[i];
        file << "party_" << i << "="
             << member.speciesId << ","
             << member.level << ","
             << (member.isPartner ? 1 : 0) << "\n";
    }

    return file.good();
}

std::filesystem::path SaveGameStorage::slotPath(const int slotIndex) const {
    return rootDirectory_ / ("slot" + std::to_string(slotIndex) + ".sav");
}
