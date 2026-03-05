#include "game/dev/OverworldDevConsoleHandlers.h"

#include <algorithm>
#include <optional>
#include <sstream>
#include <vector>

#include "game/dev/OverworldDevConsoleParsing.h"

namespace OverworldDevConsoleHandlers {
bool handleInfoCommand(
    const std::string& command,
    const std::vector<std::string>& tokens,
    const OverworldDevConsole::Dependencies& dependencies
) {
    if (command == "help") {
        if (dependencies.print) {
            dependencies.print("Commands: help, maps, map, spawns, pos, scripts, runscript <id>, stopscript, startintro, resetintro, encounters, encounter [tableId], party, warp <mapId> [spawnId], warptile <mapId> <tx> <ty>, warpxy <mapId> <x> <y>, goto <tx> <ty>, teleport <x> <y>, setflag <k> <0|1>, getflag <k>, setvar <k> <n>, getvar <k>, settextspeed <normal|fast>, gettextspeed, setbattlestyle <shift|set>, getbattlestyle, setencounters <on|off>, getencounters, setencounterrate <0-100>, getencounterrate, reload");
        }
        return true;
    }

    if (command == "maps") {
        if (!dependencies.print) {
            return true;
        }

        if (!dependencies.listMapIds) {
            dependencies.print("Map registry unavailable.");
            return true;
        }

        const std::vector<std::string> ids = dependencies.listMapIds();
        if (ids.empty()) {
            dependencies.print("No maps registered.");
            return true;
        }

        std::string line = "Registered maps:";
        for (const std::string& id : ids) {
            line += " " + id;
        }
        dependencies.print(line);
        return true;
    }

    if (command == "map") {
        if (!dependencies.print) {
            return true;
        }

        if (!dependencies.currentMapId) {
            dependencies.print("Current map unavailable.");
            return true;
        }

        dependencies.print("Current map: " + dependencies.currentMapId());
        return true;
    }

    if (command == "scripts") {
        if (!dependencies.print) {
            return true;
        }

        std::vector<std::string> scriptIds = OverworldDevConsoleParsing::listLuaScriptIds();
        if (dependencies.listScriptIds) {
            std::vector<std::string> loadedScriptIds = dependencies.listScriptIds();
            scriptIds.insert(scriptIds.end(), loadedScriptIds.begin(), loadedScriptIds.end());
        }

        std::sort(scriptIds.begin(), scriptIds.end());
        scriptIds.erase(std::unique(scriptIds.begin(), scriptIds.end()), scriptIds.end());

        if (scriptIds.empty()) {
            dependencies.print("No scripts registered.");
            return true;
        }

        std::string line = "Scripts:";
        for (const std::string& scriptId : scriptIds) {
            line += " " + scriptId;
        }

        dependencies.print(line);
        return true;
    }

    if (command == "party") {
        if (!dependencies.print) {
            return true;
        }

        if (!dependencies.partySummary) {
            dependencies.print("Party data unavailable.");
            return true;
        }

        const std::string summary = dependencies.partySummary();
        if (summary.empty()) {
            dependencies.print("Party is empty.");
            return true;
        }

        dependencies.print(summary);
        return true;
    }

    if (command == "spawns") {
        if (!dependencies.print) {
            return true;
        }

        if (!dependencies.listSpawnIds) {
            dependencies.print("Spawn registry unavailable.");
            return true;
        }

        const std::vector<std::string> spawnIds = dependencies.listSpawnIds();
        if (spawnIds.empty()) {
            dependencies.print("No spawn points found on this map.");
            return true;
        }

        std::string line = "Spawn ids:";
        for (const std::string& id : spawnIds) {
            line += " " + id;
        }
        dependencies.print(line);
        return true;
    }

    if (command == "pos") {
        if (!dependencies.print) {
            return true;
        }

        if (!dependencies.playerPosition || !dependencies.currentMapId) {
            dependencies.print("Player entity not found.");
            return true;
        }

        const std::optional<OverworldDevConsole::PlayerPosition> position = dependencies.playerPosition();
        if (!position.has_value()) {
            dependencies.print("Player entity not found.");
            return true;
        }

        std::ostringstream stream;
        stream << "Map " << dependencies.currentMapId()
               << " | px(" << position->x << ", " << position->y << ")"
               << " tile(" << position->tileX << ", " << position->tileY << ")";
        dependencies.print(stream.str());
        return true;
    }

    (void)tokens;
    return false;
}
} // namespace OverworldDevConsoleHandlers
