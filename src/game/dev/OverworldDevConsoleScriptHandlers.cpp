#include "game/dev/OverworldDevConsoleHandlers.h"

#include <string>
#include <vector>

#include "game/state/GameState.h"

namespace {
constexpr const char* kIntroScriptId = "intro_prologue";
}

namespace OverworldDevConsoleHandlers {
bool handleScriptCommand(
    const std::string& command,
    const std::vector<std::string>& tokens,
    const OverworldDevConsole::Dependencies& dependencies
) {
    if (command == "runscript") {
        if (!dependencies.print) {
            return true;
        }

        if (tokens.size() != 2) {
            dependencies.print("Usage: runscript <scriptId>");
            return true;
        }

        if (!dependencies.runScript || !dependencies.runScript(tokens[1])) {
            dependencies.print("Script not found: " + tokens[1]);
        }
        return true;
    }

    if (command == "stopscript") {
        if (!dependencies.print) {
            return true;
        }

        if (!dependencies.stopScript || !dependencies.stopScript()) {
            dependencies.print("No script is running.");
            return true;
        }

        dependencies.print("Script stopped.");
        return true;
    }

    if (command == "startintro") {
        if (!dependencies.print) {
            return true;
        }

        if (!dependencies.runScript || !dependencies.runScript(kIntroScriptId)) {
            dependencies.print("Intro script not found.");
            return true;
        }

        dependencies.print("Intro script started.");
        return true;
    }

    if (command == "resetintro") {
        if (!dependencies.print) {
            return true;
        }

        if (!dependencies.gameState) {
            dependencies.print("Game state unavailable.");
            return true;
        }

        dependencies.gameState->setFlag("intro_complete", false);
        dependencies.gameState->setFlag("starter_eevee_obtained", false);
        dependencies.gameState->setVar("story_checkpoint", 0);
        dependencies.gameState->clearParty();

        if (!dependencies.runScript || !dependencies.runScript(kIntroScriptId)) {
            dependencies.print("Intro state reset. intro_prologue.lua not found.");
            return true;
        }

        dependencies.print("Intro state reset and intro script started.");
        return true;
    }

    return false;
}
} // namespace OverworldDevConsoleHandlers
