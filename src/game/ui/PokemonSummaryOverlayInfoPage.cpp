#include "game/ui/PokemonSummaryOverlay.h"

#include <sstream>

#include <SDL3/SDL_render.h>

#include "engine/TextureManager.h"
#include "game/ui/PokemonSummaryLayout.h"
#include "game/ui/PokemonSummaryRenderUtils.h"
#include "game/ui/SummaryContent.h"

void PokemonSummaryOverlay::renderInfoPage(
    TextureManager& textureManager,
    SDL_Renderer* renderer,
    SDL_Texture* menuInfoTexture,
    const PartyPokemon& member,
    const char* speciesName,
    const SpeciesDefinition* speciesData,
    const float scale,
    const float offsetX,
    const float offsetY,
    const PokemonSummaryLayout& layout
) const {
    (void)renderer;
    (void)speciesData;

    std::ostringstream dexText;
    dexText << member.speciesId;
    summary_render::renderDebugTextWindowAligned(
        textureManager,
        dexText.str(),
        layout.windowInfoRightPane,
        layout.infoDexX,
        layout.infoDexY,
        scale,
        offsetX,
        offsetY,
        layout
    );
    summary_render::renderDebugTextWindowAligned(
        textureManager,
        speciesName,
        layout.windowInfoRightPane,
        layout.infoNameX,
        layout.infoNameY,
        scale,
        offsetX,
        offsetY,
        layout
    );

    const summary_content::SpeciesTypeDisplay speciesTypes = summary_content::typeIdsForSpecies(member);
    summary_render::renderTypeIcon(
        renderer,
        menuInfoTexture,
        speciesTypes.primary,
        layout.infoTypeIconRect,
        scale,
        offsetX,
        offsetY
    );
    if (speciesTypes.secondary != speciesTypes.primary) {
        SDL_FRect secondaryTypeRect = layout.infoTypeIconRect;
        secondaryTypeRect.x += 36.0f;
        summary_render::renderTypeIcon(
            renderer,
            menuInfoTexture,
            speciesTypes.secondary,
            secondaryTypeRect,
            scale,
            offsetX,
            offsetY
        );
    }

    summary_render::renderDebugTextWindowAligned(
        textureManager,
        "ADAM",
        layout.windowInfoRightPane,
        layout.infoOtX,
        layout.infoOtY,
        scale,
        offsetX,
        offsetY,
        layout
    );
    summary_render::renderDebugTextWindowAligned(
        textureManager,
        "00001",
        layout.windowInfoRightPane,
        layout.infoIdX,
        layout.infoIdY,
        scale,
        offsetX,
        offsetY,
        layout
    );
    summary_render::renderDebugTextWindowAligned(
        textureManager,
        summary_content::itemTextFor(member),
        layout.windowInfoRightPane,
        layout.infoItemX,
        layout.infoItemY,
        scale,
        offsetX,
        offsetY,
        layout
    );

    summary_render::renderDebugTextWindowAligned(
        textureManager,
        "Relaxed nature.",
        layout.windowInfoTrainerMemo,
        layout.infoMemoLine1X,
        layout.infoMemoLine1Y,
        scale,
        offsetX,
        offsetY,
        layout
    );
    summary_render::renderDebugTextWindowAligned(
        textureManager,
        "Met in PALLET TOWN at Lv5.",
        layout.windowInfoTrainerMemo,
        layout.infoMemoLine2X,
        layout.infoMemoLine2Y,
        scale,
        offsetX,
        offsetY,
        layout
    );
}

