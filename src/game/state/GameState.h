#pragma once

#include <algorithm>
#include <cctype>
#include <string>
#include <unordered_map>
#include <vector>

struct PartyPokemon {
    int speciesId = 0;
    int level = 1;
    bool isPartner = false;
};

class GameState {
public:
    void setFlag(const std::string& key, const bool value) {
        flags_[normalize(key)] = value;
    }

    bool getFlag(const std::string& key, const bool defaultValue = false) const {
        const auto it = flags_.find(normalize(key));
        if (it == flags_.end()) {
            return defaultValue;
        }

        return it->second;
    }

    void setVar(const std::string& key, const int value) {
        vars_[normalize(key)] = value;
    }

    int getVar(const std::string& key, const int defaultValue = 0) const {
        const auto it = vars_.find(normalize(key));
        if (it == vars_.end()) {
            return defaultValue;
        }

        return it->second;
    }

    void clearParty() {
        party_.clear();
    }

    void addPartyPokemon(const int speciesId, const int level, const bool isPartner = false) {
        PartyPokemon pokemon{};
        pokemon.speciesId = speciesId;
        pokemon.level = level;
        pokemon.isPartner = isPartner;
        party_.push_back(pokemon);
    }

    const std::vector<PartyPokemon>& party() const {
        return party_;
    }

    void setTextSpeedFast(const bool value) {
        textSpeedFast_ = value;
    }

    bool isTextSpeedFast() const {
        return textSpeedFast_;
    }

    void setBattleStyleSet(const bool value) {
        battleStyleSet_ = value;
    }

    bool isBattleStyleSet() const {
        return battleStyleSet_;
    }

    void setWildEncountersEnabled(const bool value) {
        wildEncountersEnabled_ = value;
    }

    bool areWildEncountersEnabled() const {
        return wildEncountersEnabled_;
    }

    void setWildEncounterRatePercent(const int value) {
        wildEncounterRatePercent_ = std::clamp(value, 0, 100);
    }

    int wildEncounterRatePercent() const {
        return wildEncounterRatePercent_;
    }

private:
    static std::string normalize(const std::string& key) {
        std::string normalized;
        normalized.reserve(key.size());
        for (const char ch : key) {
            normalized.push_back(static_cast<char>(std::tolower(static_cast<unsigned char>(ch))));
        }
        return normalized;
    }

    std::unordered_map<std::string, bool> flags_;
    std::unordered_map<std::string, int> vars_;
    std::vector<PartyPokemon> party_;
    bool textSpeedFast_ = false;
    bool battleStyleSet_ = false;
    bool wildEncountersEnabled_ = true;
    int wildEncounterRatePercent_ = 16;
};
