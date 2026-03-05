#include "game/dev/OverworldDevConsoleHandlers.h"

#include <sstream>
#include <string>
#include <vector>

#include "engine/utils/Vector2D.h"
#include "game/dev/OverworldDevConsoleParsing.h"

namespace OverworldDevConsoleHandlers {
bool handleTravelCommand(
    const std::string& command,
    const std::vector<std::string>& tokens,
    const OverworldDevConsole::Dependencies& dependencies
) {
    if (command == "reload") {
        if (!dependencies.print) {
            return true;
        }

        if (!dependencies.reloadCurrentMap || !dependencies.reloadCurrentMap()) {
            dependencies.print("Reload failed.");
            return true;
        }

        if (dependencies.currentMapId) {
            dependencies.print("Reloaded map: " + dependencies.currentMapId());
        } else {
            dependencies.print("Reloaded map.");
        }
        return true;
    }

    if (command == "warp") {
        if (!dependencies.print) {
            return true;
        }

        if (tokens.size() < 2 || tokens.size() > 3) {
            dependencies.print("Usage: warp <mapId> [spawnId]");
            return true;
        }

        if (!dependencies.warpToSpawn) {
            dependencies.print("Warp unavailable.");
            return true;
        }

        const std::string spawnId = tokens.size() >= 3 ? tokens[2] : "default";
        if (!dependencies.warpToSpawn(tokens[1], spawnId)) {
            dependencies.print("Warp failed.");
            return true;
        }

        const std::string currentMap = dependencies.currentMapId ? dependencies.currentMapId() : tokens[1];
        dependencies.print("Warped to " + currentMap + " spawn " + spawnId);
        return true;
    }

    if (command == "warptile") {
        if (!dependencies.print) {
            return true;
        }

        if (tokens.size() != 4) {
            dependencies.print("Usage: warptile <mapId> <tileX> <tileY>");
            return true;
        }

        if (!dependencies.warpToPoint || !dependencies.tileToPixel) {
            dependencies.print("Warp unavailable.");
            return true;
        }

        int tileX = 0;
        int tileY = 0;
        if (!OverworldDevConsoleParsing::parseIntToken(tokens[2], tileX)
            || !OverworldDevConsoleParsing::parseIntToken(tokens[3], tileY)) {
            dependencies.print("warptile expects integer tile coordinates.");
            return true;
        }

        const Vector2D spawnPoint = dependencies.tileToPixel(tileX, tileY);
        if (!dependencies.warpToPoint(tokens[1], spawnPoint)) {
            dependencies.print("Warp failed.");
            return true;
        }

        const std::string currentMap = dependencies.currentMapId ? dependencies.currentMapId() : tokens[1];
        std::ostringstream stream;
        stream << "Warped to " << currentMap << " tile(" << tileX << ", " << tileY << ")";
        dependencies.print(stream.str());
        return true;
    }

    if (command == "warpxy") {
        if (!dependencies.print) {
            return true;
        }

        if (tokens.size() != 4) {
            dependencies.print("Usage: warpxy <mapId> <x> <y>");
            return true;
        }

        if (!dependencies.warpToPoint) {
            dependencies.print("Warp unavailable.");
            return true;
        }

        float x = 0.0f;
        float y = 0.0f;
        if (!OverworldDevConsoleParsing::parseFloatToken(tokens[2], x)
            || !OverworldDevConsoleParsing::parseFloatToken(tokens[3], y)) {
            dependencies.print("warpxy expects numeric x/y.");
            return true;
        }

        if (!dependencies.warpToPoint(tokens[1], Vector2D(x, y))) {
            dependencies.print("Warp failed.");
            return true;
        }

        const std::string currentMap = dependencies.currentMapId ? dependencies.currentMapId() : tokens[1];
        std::ostringstream stream;
        stream << "Warped to " << currentMap << " px(" << x << ", " << y << ")";
        dependencies.print(stream.str());
        return true;
    }

    if (command == "goto") {
        if (!dependencies.print) {
            return true;
        }

        if (tokens.size() != 3) {
            dependencies.print("Usage: goto <tileX> <tileY>");
            return true;
        }

        if (!dependencies.teleportToPoint || !dependencies.tileToPixel) {
            dependencies.print("Teleport unavailable.");
            return true;
        }

        int tileX = 0;
        int tileY = 0;
        if (!OverworldDevConsoleParsing::parseIntToken(tokens[1], tileX)
            || !OverworldDevConsoleParsing::parseIntToken(tokens[2], tileY)) {
            dependencies.print("goto expects integer tile coordinates.");
            return true;
        }

        if (!dependencies.teleportToPoint(dependencies.tileToPixel(tileX, tileY))) {
            dependencies.print("Teleport failed.");
            return true;
        }

        std::ostringstream stream;
        stream << "Teleported to tile(" << tileX << ", " << tileY << ")";
        dependencies.print(stream.str());
        return true;
    }

    if (command == "teleport") {
        if (!dependencies.print) {
            return true;
        }

        if (tokens.size() != 3) {
            dependencies.print("Usage: teleport <x> <y>");
            return true;
        }

        if (!dependencies.teleportToPoint) {
            dependencies.print("Teleport unavailable.");
            return true;
        }

        float x = 0.0f;
        float y = 0.0f;
        if (!OverworldDevConsoleParsing::parseFloatToken(tokens[1], x)
            || !OverworldDevConsoleParsing::parseFloatToken(tokens[2], y)) {
            dependencies.print("teleport expects numeric x/y.");
            return true;
        }

        if (!dependencies.teleportToPoint(Vector2D(x, y))) {
            dependencies.print("Teleport failed.");
            return true;
        }

        std::ostringstream stream;
        stream << "Teleported to px(" << x << ", " << y << ")";
        dependencies.print(stream.str());
        return true;
    }

    return false;
}
} // namespace OverworldDevConsoleHandlers
