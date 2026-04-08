#include "Game.h"

#include "manager/AssetManager.h"

void Game::loadGameContent(const int gameViewWidth, const int gameViewHeight) {
    // Game content registration changes when maps and startup assets change,
    // so it is kept separate from low-level SDL startup.
    AssetManager::loadAnimation("player", "assets/animations/wes_overworld.xml");

    // These scene names match the map names already written into the TMX warp
    // data, so doors and route exits can switch maps without extra remapping code.
    sceneManager.load("MAP_PALLET_TOWN", "assets/maps/pallet_town/pallet_town_map.tmx", "assets/tilesets/pallet_town/pallet_town_tileset.png", gameViewWidth, gameViewHeight);
    sceneManager.load("MAP_PALLET_TOWN_PLAYERS_HOUSE_1F", "assets/maps/pallet_town_players_house_1f/pallet_town_players_house_1f_map.tmx", "assets/tilesets/pallet_town_players_house_1f/pallet_town_players_house_1f_tileset.png", gameViewWidth, gameViewHeight);
    sceneManager.load("MAP_PALLET_TOWN_PLAYERS_HOUSE_2F", "assets/maps/pallet_town_players_house_2f/pallet_town_players_house_2f_map.tmx", "assets/tilesets/pallet_town_players_house_2f/pallet_town_players_house_2f_tileset.png", gameViewWidth, gameViewHeight);
    sceneManager.load("MAP_PALLET_TOWN_RIVALS_HOUSE", "assets/maps/pallet_town_rivals_house/pallet_town_rivals_house_map.tmx", "assets/tilesets/pallet_town_rivals_house/pallet_town_rivals_house_tileset.png", gameViewWidth, gameViewHeight);
    sceneManager.load("MAP_PALLET_TOWN_PROFESSOR_OAKS_LAB", "assets/maps/pallet_town_professor_oaks_lab/pallet_town_professor_oaks_lab_map.tmx", "assets/tilesets/pallet_town_professor_oaks_lab/pallet_town_professor_oaks_lab_tileset.png", gameViewWidth, gameViewHeight);
    sceneManager.load("MAP_ROUTE1", "assets/maps/route_1/route_1_map.tmx", "assets/tilesets/route_1/route_1_tileset.png", gameViewWidth, gameViewHeight);
    sceneManager.changeSceneDeferred("MAP_PALLET_TOWN");
}
