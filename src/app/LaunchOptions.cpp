#include "app/LaunchOptions.h"

#include <algorithm>
#include <cctype>
#include <cstdlib>
#include <iostream>
#include <string>

namespace {
std::string toLower(std::string value) {
    std::transform(value.begin(), value.end(), value.begin(), [](const unsigned char ch) {
        return static_cast<char>(std::tolower(ch));
    });
    return value;
}

bool parseBootMode(const std::string& value, BootMode& modeOut) {
    const std::string normalized = toLower(value);
    if (normalized == "auto") {
        modeOut = BootMode::Auto;
        return true;
    }

    if (normalized == "intro" || normalized == "tutorial") {
        modeOut = BootMode::Intro;
        return true;
    }

    if (normalized == "overworld" || normalized == "skip_intro") {
        modeOut = BootMode::Overworld;
        return true;
    }

    return false;
}
} // namespace

void printLaunchUsage() {
    std::cout << "PokemonCipher launch options:\n"
              << "  --boot <auto|intro|overworld>\n"
              << "  --map <mapId>\n"
              << "  --spawn <spawnId>\n"
              << "  --help\n"
              << "\n"
              << "Env vars (optional):\n"
              << "  POKEMONCIPHER_BOOT_MODE\n"
              << "  POKEMONCIPHER_MAP\n"
              << "  POKEMONCIPHER_SPAWN\n";
}

LaunchOptions parseLaunchOptions(const int argc, char** argv) {
    LaunchOptions options{};

    if (const char* modeEnv = std::getenv("POKEMONCIPHER_BOOT_MODE"); modeEnv && modeEnv[0] != '\0') {
        if (!parseBootMode(modeEnv, options.mode)) {
            std::cout << "Invalid POKEMONCIPHER_BOOT_MODE value: " << modeEnv << '\n';
            options.valid = false;
            return options;
        }
    }

    if (const char* mapEnv = std::getenv("POKEMONCIPHER_MAP"); mapEnv && mapEnv[0] != '\0') {
        options.mapId = mapEnv;
    }

    if (const char* spawnEnv = std::getenv("POKEMONCIPHER_SPAWN"); spawnEnv && spawnEnv[0] != '\0') {
        options.spawnId = spawnEnv;
    }

    for (int i = 1; i < argc; ++i) {
        const std::string argument = argv[i] ? argv[i] : "";
        if (argument == "--help" || argument == "-h") {
            options.showHelp = true;
            continue;
        }

        auto readValue = [argc, argv, &i](const std::string& inlineValue) -> std::string {
            if (!inlineValue.empty()) {
                return inlineValue;
            }

            if (i + 1 >= argc) {
                return "";
            }

            ++i;
            return argv[i] ? argv[i] : "";
        };

        const std::size_t equalsPos = argument.find('=');
        const std::string key = equalsPos == std::string::npos ? argument : argument.substr(0, equalsPos);
        const std::string inlineValue = equalsPos == std::string::npos ? "" : argument.substr(equalsPos + 1);

        if (key == "--boot") {
            const std::string value = readValue(inlineValue);
            if (value.empty() || !parseBootMode(value, options.mode)) {
                std::cout << "Invalid --boot value. Expected auto|intro|overworld.\n";
                options.valid = false;
                return options;
            }
            continue;
        }

        if (key == "--map") {
            const std::string value = readValue(inlineValue);
            if (value.empty()) {
                std::cout << "Missing value for --map.\n";
                options.valid = false;
                return options;
            }
            options.mapId = value;
            continue;
        }

        if (key == "--spawn") {
            const std::string value = readValue(inlineValue);
            if (value.empty()) {
                std::cout << "Missing value for --spawn.\n";
                options.valid = false;
                return options;
            }
            options.spawnId = value;
            continue;
        }

        std::cout << "Unknown argument: " << argument << '\n';
        options.valid = false;
        return options;
    }

    return options;
}
