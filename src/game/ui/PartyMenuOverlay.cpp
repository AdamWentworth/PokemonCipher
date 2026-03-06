#include "game/ui/PartyMenuOverlay.h"

#include <algorithm>
#include <cmath>
#include <iostream>
#include <sstream>
#include <string>
#include <utility>

#include <SDL3/SDL_render.h>
#include <SDL3/SDL_timer.h>

#include "engine/TextureManager.h"
#include "game/data/SpeciesRegistry.h"
#include "game/ui/PartyMenuLayout.h"

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

const PartyMenuLayout& partyMenuLayout();

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
    return offsetY + ((y + partyMenuLayout().textBaselineYOffset) * scale);
}

const PartyMenuLayout& partyMenuLayout() {
    static const PartyMenuLayout layout = []() {
        PartyMenuLayout loaded = makeDefaultPartyMenuLayout();
        std::string error;
        if (!loadPartyMenuLayoutFromLuaFile("assets/config/ui/party_menu_layout.lua", loaded, error) &&
            !error.empty()) {
            std::cout << "Party menu layout config load failed; using defaults: " << error << '\n';
        }
        return loaded;
    }();

    return layout;
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

void PartyMenuOverlay::setSelectedIndex(const int selectedIndex, const std::vector<PartyPokemon>& party) {
    const int count = partyCount(party);
    if (count <= 0) {
        selectedIndex_ = 0;
        lastRightColumnIndex_ = 1;
        return;
    }

    selectedIndex_ = std::clamp(selectedIndex, 0, count - 1);
    if (selectedIndex_ > 0) {
        lastRightColumnIndex_ = selectedIndex_;
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
            commandMenuOpen_ = false;
            setPrompt("Choose a POKEMON.");
            return PartyMenuAction::OpenSummary;
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

    const PartyMenuLayout& layout = partyMenuLayout();
    const float scaleX = static_cast<float>(viewportWidth) / layout.designWidth;
    const float scaleY = static_cast<float>(viewportHeight) / layout.designHeight;
    const float scale = std::max(0.01f, std::min(scaleX, scaleY));
    const float contentWidth = layout.designWidth * scale;
    const float contentHeight = layout.designHeight * scale;
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
        const SDL_FRect& designRect = layout.slotRects[static_cast<std::size_t>(i)];

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
        const float textY = (i == 0) ? (baseY + layout.nameTextMainYOffset) : (baseY + layout.nameTextOtherYOffset);

        const char* iconAssetPath = speciesIconAssetPathForId(member.speciesId);
        SDL_Texture* iconTexture = iconAssetPath ? textureManager.load(iconAssetPath) : nullptr;
        const bool hasSpeciesIcon = iconTexture != nullptr;

        if (pokeballTexture && !hasSpeciesIcon) {
            SDL_FRect src{0.0f, selected ? 16.0f : 0.0f, 16.0f, 16.0f};
            SDL_FRect dst = mapToViewport(
                SDL_FRect{
                    layout.pokeballPositions[static_cast<std::size_t>(i)].x - 8.0f,
                    layout.pokeballPositions[static_cast<std::size_t>(i)].y - 8.0f,
                    16.0f,
                    16.0f
                },
                scale,
                offsetX,
                offsetY
            );
            SDL_RenderTexture(renderer, pokeballTexture, &src, &dst);
        }

        if (iconTexture) {
            const int iconFrame = static_cast<int>((SDL_GetTicks() / 300) % 2);
            const SDL_FRect src{0.0f, static_cast<float>(iconFrame * 32), 32.0f, 32.0f};
            SDL_FRect dst = mapToViewport(
                SDL_FRect{
                    layout.pokemonIconPositions[static_cast<std::size_t>(i)].x - 16.0f,
                    layout.pokemonIconPositions[static_cast<std::size_t>(i)].y - 16.0f,
                    32.0f,
                    32.0f
                },
                scale,
                offsetX,
                offsetY
            );
            SDL_RenderTexture(renderer, iconTexture, &src, &dst);
        }

        SDL_SetRenderDrawColor(renderer, 34, 37, 44, 255);
        SDL_RenderDebugText(
            renderer,
            mapX(baseX + layout.nameTextOffsetX, scale, offsetX),
            mapY(textY, scale, offsetY),
            speciesNameForId(member.speciesId)
        );

        std::ostringstream levelText;
        levelText << "Lv" << member.level;
        SDL_RenderDebugText(
            renderer,
            mapX(baseX + (i == 0 ? layout.levelTextMainX : layout.levelTextOtherX), scale, offsetX),
            mapY(textY + (i == 0 ? layout.levelTextMainYOffset : layout.levelTextOtherYOffset), scale, offsetY),
            levelText.str().c_str()
        );

        const float barX = baseX + (i == 0 ? layout.hpBarMainX : layout.hpBarOtherX);
        const float barY = baseY + (i == 0 ? layout.hpBarMainY : layout.hpBarOtherY);
        const float barW = layout.hpBarWidth;
        const float hpRatio = std::clamp(static_cast<float>(stats.hp) / static_cast<float>(stats.maxHp), 0.0f, 1.0f);
        const float fillW = std::max(1.0f, std::round(barW * hpRatio));

        SDL_FRect hpBack = mapToViewport(SDL_FRect{barX, barY, barW, layout.hpBarHeight}, scale, offsetX, offsetY);
        SDL_SetRenderDrawColor(renderer, 48, 56, 70, 255);
        SDL_RenderFillRect(renderer, &hpBack);

        SDL_FRect hpFill = mapToViewport(SDL_FRect{barX, barY, fillW, layout.hpBarHeight}, scale, offsetX, offsetY);
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
            mapX(baseX + (i == 0 ? layout.hpTextMainX : layout.hpTextOtherX), scale, offsetX),
            mapY(baseY + (i == 0 ? layout.hpTextMainY : layout.hpTextOtherY), scale, offsetY),
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

    const SDL_FRect messageRect = mapToViewport(layout.messageRect, scale, offsetX, offsetY);
    SDL_SetRenderDrawColor(renderer, 20, 24, 32, 228);
    SDL_RenderFillRect(renderer, &messageRect);
    SDL_SetRenderDrawColor(renderer, 232, 232, 232, 255);
    SDL_RenderRect(renderer, &messageRect);

    if (!drawDesignTexture(cancelButtonTexture, layout.cancelRect)) {
        const SDL_FRect cancelRect = mapToViewport(layout.cancelRect, scale, offsetX, offsetY);
        SDL_SetRenderDrawColor(renderer, 20, 24, 32, 228);
        SDL_RenderFillRect(renderer, &cancelRect);
        SDL_SetRenderDrawColor(renderer, 232, 232, 232, 255);
        SDL_RenderRect(renderer, &cancelRect);
    }

    SDL_SetRenderDrawColor(renderer, 245, 245, 245, 255);
    if (count <= 0) {
        SDL_RenderDebugText(
            renderer,
            mapX(layout.messageRect.x + layout.promptTextXOffset, scale, offsetX),
            mapY(layout.messageRect.y + layout.promptTextYOffset, scale, offsetY),
            "No POKEMON. Press X to close."
        );
    } else {
        SDL_RenderDebugText(
            renderer,
            mapX(layout.messageRect.x + layout.promptTextXOffset, scale, offsetX),
            mapY(layout.messageRect.y + layout.promptTextYOffset, scale, offsetY),
            promptText_.c_str()
        );
    }

    if (!commandMenuOpen_) {
        return;
    }

    const SDL_FRect commandRect = mapToViewport(layout.commandRect, scale, offsetX, offsetY);
    SDL_SetRenderDrawColor(renderer, 20, 24, 32, 236);
    SDL_RenderFillRect(renderer, &commandRect);
    SDL_SetRenderDrawColor(renderer, 232, 232, 232, 255);
    SDL_RenderRect(renderer, &commandRect);

    for (int i = 0; i < 3; ++i) {
        const float y = layout.commandRect.y + layout.commandLineStartYOffset + (layout.commandLineStep * static_cast<float>(i));
        if (i == commandMenuIndex_) {
            SDL_SetRenderDrawColor(renderer, 255, 231, 128, 255);
            SDL_RenderDebugText(renderer, mapX(layout.commandRect.x + layout.commandArrowXOffset, scale, offsetX), mapY(y, scale, offsetY), ">");
            SDL_RenderDebugText(renderer, mapX(layout.commandRect.x + layout.commandLabelXOffset, scale, offsetX), mapY(y, scale, offsetY), commandLabel(i));
        } else {
            SDL_SetRenderDrawColor(renderer, 245, 245, 245, 255);
            SDL_RenderDebugText(renderer, mapX(layout.commandRect.x + layout.commandLabelXOffset, scale, offsetX), mapY(y, scale, offsetY), commandLabel(i));
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
    if (const SpeciesDefinition* species = getSpeciesRegistry().find(speciesId)) {
        if (!species->name.empty()) {
            return species->name.c_str();
        }
    }

    return "POKEMON";
}

const char* PartyMenuOverlay::speciesIconAssetPathForId(const int speciesId) {
    if (const SpeciesDefinition* species = getSpeciesRegistry().find(speciesId)) {
        if (!species->iconPath.empty()) {
            return species->iconPath.c_str();
        }
    }

    return nullptr;
}
