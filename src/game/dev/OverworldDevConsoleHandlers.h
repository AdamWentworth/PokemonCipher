#pragma once

#include <string>
#include <vector>

#include "game/dev/OverworldDevConsole.h"

namespace OverworldDevConsoleHandlers {
bool handleInfoCommand(
    const std::string& command,
    const std::vector<std::string>& tokens,
    const OverworldDevConsole::Dependencies& dependencies
);

bool handleScriptCommand(
    const std::string& command,
    const std::vector<std::string>& tokens,
    const OverworldDevConsole::Dependencies& dependencies
);

bool handleEncounterCommand(
    const std::string& command,
    const std::vector<std::string>& tokens,
    const OverworldDevConsole::Dependencies& dependencies
);

bool handleTravelCommand(
    const std::string& command,
    const std::vector<std::string>& tokens,
    const OverworldDevConsole::Dependencies& dependencies
);

bool handleStateCommand(
    const std::string& command,
    const std::vector<std::string>& tokens,
    const OverworldDevConsole::Dependencies& dependencies
);
} // namespace OverworldDevConsoleHandlers
