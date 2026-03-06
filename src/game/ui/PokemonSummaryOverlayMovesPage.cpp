#include "game/ui/PokemonSummaryOverlay.h"

#include <sstream>

#include <SDL3/SDL_render.h>

#include "engine/TextureManager.h"
#include "game/ui/PokemonSummaryLayout.h"
#include "game/ui/PokemonSummaryRenderUtils.h"
#include "game/ui/SummaryContent.h"

void PokemonSummaryOverlay::renderMovesPage(
    TextureManager& textureManager,
    SDL_Renderer* renderer,
    SDL_Texture* menuInfoTexture,
    const PartyPokemon& member,
    const DisplayStats& stats,
    const float scale,
    const float offsetX,
    const float offsetY,
    const PokemonSummaryLayout& layout
) const {
    (void)stats;
    const auto moves = summary_content::movesFor(member);
    for (int i = 0; i < 4; ++i) {
        const summary_content::MoveDisplay& move = moves[static_cast<std::size_t>(i)];
        const float nameY = (layout.movesRowStepY * static_cast<float>(i)) + layout.movesNameBaseY;
        const float ppY = nameY + layout.movesPpYOffset;
        const int ppColorIndex = summary_content::movePpColorIndex(move.pp, move.maxPp);
        const SDL_Color ppTextColor = summary_render::movePpColorFromIndex(ppColorIndex);
        const SDL_Color ppShadowColor = summary_render::movePpShadowColorFromIndex(ppColorIndex);

        summary_render::renderDebugTextWindowAligned(
            textureManager,
            move.name,
            layout.windowMovesRightPane,
            layout.movesNameX,
            nameY,
            scale,
            offsetX,
            offsetY,
            layout,
            FrlgTextRenderer::Font::Normal,
            summary_render::kMoveTextColorNormal,
            summary_render::kMoveTextColorNormalShadow
        );
        std::ostringstream ppText;
        ppText << "PP" << move.pp << "/" << move.maxPp;
        summary_render::renderDebugTextWindowRightAligned(
            textureManager,
            ppText.str(),
            layout.windowMovesRightPane,
            0.0f,
            79.0f,
            ppY,
            scale,
            offsetX,
            offsetY,
            layout,
            FrlgTextRenderer::Font::Normal,
            ppTextColor,
            ppShadowColor
        );

        summary_render::renderTypeIcon(
            renderer,
            menuInfoTexture,
            move.typeId,
            SDL_FRect{
                layout.windowMovesTypePane.x + layout.movesTypeIconXOffset,
                layout.windowMovesTypePane.y + nameY,
                layout.movesTypeIconWidth,
                layout.movesTypeIconHeight
            },
            scale,
            offsetX,
            offsetY
        );
    }
}

