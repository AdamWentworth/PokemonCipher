#pragma once

#include <string>

enum class BootMode {
    Auto,
    Intro,
    Overworld
};

struct LaunchOptions {
    BootMode mode = BootMode::Auto;
    std::string mapId = "pallet_town";
    std::string spawnId = "default";
    bool showHelp = false;
    bool valid = true;
};

void printLaunchUsage();
LaunchOptions parseLaunchOptions(int argc, char** argv);
