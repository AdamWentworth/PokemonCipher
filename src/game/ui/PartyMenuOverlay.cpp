#include "game/ui/PartyMenuOverlay.h"

#include <algorithm>
#include <array>
#include <cmath>
#include <sstream>
#include <string>
#include <utility>

#include <SDL3/SDL_render.h>

#include "engine/TextureManager.h"

namespace {
constexpr int kCommandSummary = 0;
constexpr int kCommandSwitch = 1;
constexpr int kCommandCancel = 2;

constexpr const char* kAssetBg = "assets/ui/party_menu/bg_full.png";
constexpr const char* kAssetSlotMain = "assets/ui/party_menu/slot_main.png";
constexpr const char* kAssetSlotWide = "assets/ui/party_menu/slot_wide.png";
constexpr const char* kAssetSlotWideEmpty = "assets/ui/party_menu/slot_wide_empty.png";
constexpr const char* kAssetCancelButton = "assets/ui/party_menu/cancel_button.png";
constexpr const char* kAssetPokeballSmall = "assets/ui/party_menu/pokeball_small.png";

constexpr float kDesignWidth = 240.0f;
constexpr float kDesignHeight = 160.0f;

constexpr std::array<SDL_FRect, 6> kSlotRects = {
    SDL_FRect{8.0f, 24.0f, 80.0f, 56.0f},
    SDL_FRect{96.0f, 8.0f, 136.0f, 24.0f},
    SDL_FRect{96.0f, 32.0f, 136.0f, 24.0f},
    SDL_FRect{96.0f, 56.0f, 136.0f, 24.0f},
    SDL_FRect{96.0f, 80.0f, 136.0f, 24.0f},
    SDL_FRect{96.0f, 104.0f, 136.0f, 24.0f},
};

// FireRed single-party layout coordinates for the pokeball indicator sprite.
constexpr std::array<SDL_FPoint, 6> kPokeballPositions = {
    SDL_FPoint{16.0f, 34.0f},
    SDL_FPoint{102.0f, 25.0f},
    SDL_FPoint{102.0f, 49.0f},
    SDL_FPoint{102.0f, 73.0f},
    SDL_FPoint{102.0f, 97.0f},
    SDL_FPoint{102.0f, 121.0f},
};

constexpr SDL_FRect kMessageRect{8.0f, 128.0f, 160.0f, 24.0f};
constexpr SDL_FRect kCancelRect{176.0f, 128.0f, 56.0f, 24.0f};
constexpr SDL_FRect kCommandRect{176.0f, 82.0f, 56.0f, 42.0f};

const char* commandLabel(const int index) {
    switch (index) {
    case kCommandSummary:
        return "SUMMARY";
    case kCommandSwitch:
        return "SWITCH";
    case kCommandCancel:
        return "CANCEL";
    default:
        return "";
    }
}

SDL_FRect mapToViewport(
    const SDL_FRect& rect,
    const float scale,
    const float offsetX,
    const float offsetY
) {
    return SDL_FRect{
        offsetX + (rect.x * scale),
        offsetY + (rect.y * scale),
        rect.w * scale,
        rect.h * scale,
    };
}

float mapX(const float x, const float scale, const float offsetX) {
    return offsetX + (x * scale);
}

float mapY(const float y, const float scale, const float offsetY) {
    return offsetY + (y * scale);
}
} // namespace

void PartyMenuOverlay::open(const std::vector<PartyPokemon>& party) {
    open_ = true;
    commandMenuOpen_ = false;
    switchMode_ = false;
    commandMenuIndex_ = 0;
    setPrompt("Choose a POKEMON.");

    const int count = partyCount(party);
    if (count <= 0) {
        selectedIndex_ = 0;
        return;
    }

    selectedIndex_ = std::clamp(selectedIndex_, 0, count - 1);
    if (selectedIndex_ > 0) {
        lastRightColumnIndex_ = selectedIndex_;
    } else if (count > 1) {
        lastRightColumnIndex_ = std::clamp(lastRightColumnIndex_, 1, count - 1);
    }
}

void PartyMenuOverlay::close() {
    open_ = false;
    commandMenuOpen_ = false;
    switchMode_ = false;
}

PartyMenuAction PartyMenuOverlay::handleKey(const SDL_Keycode key, std::vector<PartyPokemon>& party) {
    if (!open_) {
        return PartyMenuAction::None;
    }

    const int count = partyCount(party);
    if (key == SDLK_ESCAPE || key == SDLK_X || key == SDLK_BACKSPACE) {
        if (commandMenuOpen_) {
            commandMenuOpen_ = false;
            setPrompt("Choose a POKEMON.");
            return PartyMenuAction::None;
        }

        if (switchMode_) {
            switchMode_ = false;
            setPrompt("Choose a POKEMON.");
            return PartyMenuAction::None;
        }

        close();
        return PartyMenuAction::Closed;
    }

    if (count <= 0) {
        if (key == SDLK_RETURN || key == SDLK_KP_ENTER || key == SDLK_Z || key == SDLK_SPACE) {
            close();
            return PartyMenuAction::Closed;
        }
        return PartyMenuAction::None;
    }

    if (commandMenuOpen_) {
        if (key == SDLK_UP || key == SDLK_W) {
            commandMenuIndex_ = (commandMenuIndex_ + 2) % 3;
            return PartyMenuAction::None;
        }

        if (key == SDLK_DOWN || key == SDLK_S) {
            commandMenuIndex_ = (commandMenuIndex_ + 1) % 3;
            return PartyMenuAction::None;
        }

        if (key != SDLK_RETURN && key != SDLK_KP_ENTER && key != SDLK_Z && key != SDLK_SPACE) {
            return PartyMenuAction::None;
        }

        if (commandMenuIndex_ == kCommandSummary) {
            std::ostringstream message;
            message << speciesNameForId(party[static_cast<std::size_t>(selectedIndex_)].speciesId)
                    << " summary not implemented yet.";
            setPrompt(message.str());
            commandMenuOpen_ = false;
            return PartyMenuAction::None;
        }

        if (commandMenuIndex_ == kCommandSwitch) {
            if (count < 2) {
                setPrompt("No other POKEMON to switch.");
                commandMenuOpen_ = false;
                return PartyMenuAction::None;
            }

            switchSourceIndex_ = selectedIndex_;
            switchMode_ = true;
            commandMenuOpen_ = false;
            setPrompt("Move to where?");
            return PartyMenuAction::None;
        }

        commandMenuOpen_ = false;
        setPrompt("Choose a POKEMON.");
        return PartyMenuAction::None;
    }

    if (key == SDLK_UP || key == SDLK_W || key == SDLK_DOWN || key == SDLK_S || key == SDLK_LEFT || key == SDLK_A ||
        key == SDLK_RIGHT || key == SDLK_D) {
        moveSelection(key, count);
        return PartyMenuAction::None;
    }

    if (key != SDLK_RETURN && key != SDLK_KP_ENTER && key != SDLK_Z && key != SDLK_SPACE) {
        return PartyMenuAction::None;
    }

    if (switchMode_) {
        if (selectedIndex_ == switchSourceIndex_) {
            setPrompt("Move to where?");
            return PartyMenuAction::None;
        }

        std::swap(
            party[static_cast<std::size_t>(switchSourceIndex_)],
            party[static_cast<std::size_t>(selectedIndex_)]
        );
        switchMode_ = false;
        if (selectedIndex_ > 0) {
            lastRightColumnIndex_ = selectedIndex_;
        }
        setPrompt("POKEMON switched.");
        return PartyMenuAction::None;
    }

    commandMenuOpen_ = true;
    commandMenuIndex_ = 0;
    setPrompt("Do what with this POKEMON?");
    return PartyMenuAction::None;
}

void PartyMenuOverlay::render(
    TextureManager& textureManager,
    const int viewportWidth,
    const int viewportHeight,
    const std::vector<PartyPokemon>& party
) const {
    SDL_Renderer* renderer = textureManager.renderer();
    if (!open_ || !renderer || viewportWidth <= 0 || viewportHeight <= 0) {
        return;
    }

    const float scaleX = static_cast<float>(viewportWidth) / kDesignWidth;
    const float scaleY = static_cast<float>(viewportHeight) / kDesignHeight;
    const float scale = std::max(0.01f, std::min(scaleX, scaleY));
    const float contentWidth = kDesignWidth * scale;
    const float contentHeight = kDesignHeight * scale;
    const float offsetX = (static_cast<float>(viewportWidth) - contentWidth) * 0.5f;
    const float offsetY = (static_cast<float>(viewportHeight) - contentHeight) * 0.5f;

    const SDL_FRect fullScreenRect{0.0f, 0.0f, static_cast<float>(viewportWidth), static_cast<float>(viewportHeight)};
    const SDL_FRect contentRect{offsetX, offsetY, contentWidth, contentHeight};

    SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_BLEND);
    SDL_SetRenderDrawColor(renderer, 8, 8, 12, 255);
    SDL_RenderFillRect(renderer, &fullScreenRect);

    SDL_Texture* bgTexture = textureManager.load(kAssetBg);
    if (bgTexture) {
        SDL_RenderTexture(renderer, bgTexture, nullptr, &contentRect);
    } else {
        SDL_SetRenderDrawColor(renderer, 168, 200, 232, 255);
        SDL_RenderFillRect(renderer, &contentRect);
    }

    SDL_Texture* mainSlotTexture = textureManager.load(kAssetSlotMain);
    SDL_Texture* wideSlotTexture = textureManager.load(kAssetSlotWide);
    SDL_Texture* emptySlotTexture = textureManager.load(kAssetSlotWideEmpty);
    SDL_Texture* cancelButtonTexture = textureManager.load(kAssetCancelButton);
    SDL_Texture* pokeballTexture = textureManager.load(kAssetPokeballSmall);

    const auto drawDesignTexture = [&](SDL_Texture* texture, const SDL_FRect& designRect) {
        if (!texture) {
            return false;
        }

        const SDL_FRect dst = mapToViewport(designRect, scale, offsetX, offsetY);
        SDL_RenderTexture(renderer, texture, nullptr, &dst);
        return true;
    };

    const int count = partyCount(party);
    for (int i = 0; i < kMaxPartySlots; ++i) {
        const bool filled = i < count;
        const bool selected = i == selectedIndex_;
        const bool switchSource = switchMode_ && i == switchSourceIndex_;
        const SDL_FRect& designRect = kSlotRects[static_cast<std::size_t>(i)];

        if (i == 0) {
            if (!drawDesignTexture(mainSlotTexture, designRect)) {
                SDL_FRect fallbackRect = mapToViewport(designRect, scale, offsetX, offsetY);
                SDL_SetRenderDrawColor(renderer, 248, 248, 248, 255);
                SDL_RenderFillRect(renderer, &fallbackRect);
            }
        } else {
            SDL_Texture* slotTexture = filled ? wideSlotTexture : emptySlotTexture;
            if (!drawDesignTexture(slotTexture, designRect)) {
                SDL_FRect fallbackRect = mapToViewport(designRect, scale, offsetX, offsetY);
                SDL_SetRenderDrawColor(renderer, filled ? 248 : 208, filled ? 248 : 216, filled ? 248 : 224, 255);
                SDL_RenderFillRect(renderer, &fallbackRect);
            }
        }

        const SDL_FRect slotRect = mapToViewport(designRect, scale, offsetX, offsetY);
        if (switchSource) {
            SDL_SetRenderDrawColor(renderer, 255, 144, 80, 255);
            SDL_RenderRect(renderer, &slotRect);
        } else if (selected) {
            SDL_SetRenderDrawColor(renderer, 255, 224, 96, 255);
            SDL_RenderRect(renderer, &slotRect);
        }

        if (!filled) {
            if (i > 0) {
                SDL_SetRenderDrawColor(renderer, 72, 84, 104, 255);
                SDL_RenderDebugText(
                    renderer,
                    mapX(designRect.x + 6.0f, scale, offsetX),
                    mapY(designRect.y + 8.0f, scale, offsetY),
                    "----------"
                );
            }
            continue;
        }

        const PartyPokemon& member = party[static_cast<std::size_t>(i)];
        const DisplayStats stats = makeDisplayStats(member);
        const float baseX = designRect.x;
        const float baseY = designRect.y;
        const float textY = (i == 0) ? (baseY + 8.0f) : (baseY + 3.0f);

        if (pokeballTexture) {
            SDL_FRect src{0.0f, selected ? 16.0f : 0.0f, 16.0f, 16.0f};
            SDL_FRect dst = mapToViewport(
                SDL_FRect{
                    kPokeballPositions[static_cast<std::size_t>(i)].x,
                    kPokeballPositions[static_cast<std::size_t>(i)].y,
                    16.0f,
                    16.0f
                },
                scale,
                offsetX,
                offsetY
            );
            SDL_RenderTexture(renderer, pokeballTexture, &src, &dst);
        }

        SDL_SetRenderDrawColor(renderer, 34, 37, 44, 255);
        SDL_RenderDebugText(
            renderer,
            mapX(baseX + 22.0f, scale, offsetX),
            mapY(textY, scale, offsetY),
            speciesNameForId(member.speciesId)
        );

        std::ostringstream levelText;
        levelText << "Lv" << member.level;
        SDL_RenderDebugText(
            renderer,
            mapX(baseX + (i == 0 ? 32.0f : 52.0f), scale, offsetX),
            mapY(textY + (i == 0 ? 10.0f : 0.0f), scale, offsetY),
            levelText.str().c_str()
        );

        const float barX = baseX + (i == 0 ? 24.0f : 88.0f);
        const float barY = baseY + (i == 0 ? 36.0f : 11.0f);
        const float barW = 48.0f;
        const float hpRatio = std::clamp(static_cast<float>(stats.hp) / static_cast<float>(stats.maxHp), 0.0f, 1.0f);
        const float fillW = std::max(1.0f, std::round(barW * hpRatio));

        SDL_FRect hpBack = mapToViewport(SDL_FRect{barX, barY, barW, 3.0f}, scale, offsetX, offsetY);
        SDL_SetRenderDrawColor(renderer, 48, 56, 70, 255);
        SDL_RenderFillRect(renderer, &hpBack);

        SDL_FRect hpFill = mapToViewport(SDL_FRect{barX, barY, fillW, 3.0f}, scale, offsetX, offsetY);
        if (hpRatio > 0.5f) {
            SDL_SetRenderDrawColor(renderer, 88, 200, 104, 255);
        } else if (hpRatio > 0.2f) {
            SDL_SetRenderDrawColor(renderer, 240, 192, 72, 255);
        } else {
            SDL_SetRenderDrawColor(renderer, 224, 88, 72, 255);
        }
        SDL_RenderFillRect(renderer, &hpFill);

        std::ostringstream hpText;
        hpText << stats.hp << "/" << stats.maxHp;
        SDL_SetRenderDrawColor(renderer, 34, 37, 44, 255);
        SDL_RenderDebugText(
            renderer,
            mapX(baseX + (i == 0 ? 38.0f : 100.0f), scale, offsetX),
            mapY(baseY + (i == 0 ? 42.0f : 13.0f), scale, offsetY),
            hpText.str().c_str()
        );

        if (member.isPartner) {
            SDL_SetRenderDrawColor(renderer, 255, 160, 72, 255);
            SDL_RenderDebugText(
                renderer,
                mapX(baseX + designRect.w - 12.0f, scale, offsetX),
                mapY(textY, scale, offsetY),
                "*"
            );
        }
    }

    const SDL_FRect messageRect = mapToViewport(kMessageRect, scale, offsetX, offsetY);
    SDL_SetRenderDrawColor(renderer, 20, 24, 32, 228);
    SDL_RenderFillRect(renderer, &messageRect);
    SDL_SetRenderDrawColor(renderer, 232, 232, 232, 255);
    SDL_RenderRect(renderer, &messageRect);

    if (!drawDesignTexture(cancelButtonTexture, kCancelRect)) {
        const SDL_FRect cancelRect = mapToViewport(kCancelRect, scale, offsetX, offsetY);
        SDL_SetRenderDrawColor(renderer, 20, 24, 32, 228);
        SDL_RenderFillRect(renderer, &cancelRect);
        SDL_SetRenderDrawColor(renderer, 232, 232, 232, 255);
        SDL_RenderRect(renderer, &cancelRect);
    }

    SDL_SetRenderDrawColor(renderer, 245, 245, 245, 255);
    if (count <= 0) {
        SDL_RenderDebugText(
            renderer,
            mapX(kMessageRect.x + 6.0f, scale, offsetX),
            mapY(kMessageRect.y + 8.0f, scale, offsetY),
            "No POKEMON. Press X to close."
        );
    } else {
        SDL_RenderDebugText(
            renderer,
            mapX(kMessageRect.x + 6.0f, scale, offsetX),
            mapY(kMessageRect.y + 8.0f, scale, offsetY),
            promptText_.c_str()
        );
    }

    if (!commandMenuOpen_) {
        return;
    }

    const SDL_FRect commandRect = mapToViewport(kCommandRect, scale, offsetX, offsetY);
    SDL_SetRenderDrawColor(renderer, 20, 24, 32, 236);
    SDL_RenderFillRect(renderer, &commandRect);
    SDL_SetRenderDrawColor(renderer, 232, 232, 232, 255);
    SDL_RenderRect(renderer, &commandRect);

    for (int i = 0; i < 3; ++i) {
        const float y = kCommandRect.y + 6.0f + (10.0f * static_cast<float>(i));
        if (i == commandMenuIndex_) {
            SDL_SetRenderDrawColor(renderer, 255, 231, 128, 255);
            SDL_RenderDebugText(renderer, mapX(kCommandRect.x + 4.0f, scale, offsetX), mapY(y, scale, offsetY), ">");
            SDL_RenderDebugText(renderer, mapX(kCommandRect.x + 12.0f, scale, offsetX), mapY(y, scale, offsetY), commandLabel(i));
        } else {
            SDL_SetRenderDrawColor(renderer, 245, 245, 245, 255);
            SDL_RenderDebugText(renderer, mapX(kCommandRect.x + 12.0f, scale, offsetX), mapY(y, scale, offsetY), commandLabel(i));
        }
    }
}

int PartyMenuOverlay::partyCount(const std::vector<PartyPokemon>& party) const {
    return std::min<int>(kMaxPartySlots, static_cast<int>(party.size()));
}

void PartyMenuOverlay::moveSelection(const SDL_Keycode key, const int count) {
    if (count <= 1) {
        selectedIndex_ = 0;
        return;
    }

    if (selectedIndex_ == 0) {
        if (key == SDLK_RIGHT || key == SDLK_D) {
            selectedIndex_ = std::clamp(lastRightColumnIndex_, 1, count - 1);
        }
        return;
    }

    if (key == SDLK_UP || key == SDLK_W) {
        selectedIndex_ = (selectedIndex_ == 1) ? (count - 1) : (selectedIndex_ - 1);
    } else if (key == SDLK_DOWN || key == SDLK_S) {
        selectedIndex_ = (selectedIndex_ == count - 1) ? 1 : (selectedIndex_ + 1);
    } else if (key == SDLK_LEFT || key == SDLK_A) {
        lastRightColumnIndex_ = selectedIndex_;
        selectedIndex_ = 0;
    }
}

void PartyMenuOverlay::setPrompt(std::string text) {
    promptText_ = std::move(text);
}

PartyMenuOverlay::DisplayStats PartyMenuOverlay::makeDisplayStats(const PartyPokemon& member) {
    DisplayStats stats{};
    stats.maxHp = std::max(1, 12 + (member.level * 3));
    stats.hp = stats.maxHp;
    return stats;
}

const char* PartyMenuOverlay::speciesNameForId(const int speciesId) {
    switch (speciesId) {
    case 16:
        return "PIDGEY";
    case 19:
        return "RATTATA";
    case 133:
        return "EEVEE";
    default:
        return "POKEMON";
    }
}
