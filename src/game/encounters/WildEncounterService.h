#pragma once

#include <optional>
#include <random>
#include <string>
#include <vector>

struct WildEncounter {
    std::string tableId;
    int speciesId = 0;
    std::string speciesName;
    int level = 1;
};

class WildEncounterService {
public:
    WildEncounterService();

    std::optional<WildEncounter> rollEncounter(const std::string& tableId, std::mt19937& rng) const;
    std::vector<std::string> tableIds() const;

private:
    struct EncounterSlot {
        int speciesId = 0;
        int minLevel = 1;
        int maxLevel = 1;
        int weight = 1;
    };

    struct EncounterTable {
        std::string id;
        std::vector<EncounterSlot> slots;
    };

    static std::string normalizeKey(std::string key);
    static std::string speciesNameForId(int speciesId);
    const EncounterTable* findTable(const std::string& tableId) const;

    std::vector<EncounterTable> tables_;
};
