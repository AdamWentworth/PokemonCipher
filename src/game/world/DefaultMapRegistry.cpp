#include "game/world/DefaultMapRegistry.h"

#include <iostream>

#include "game/world/MapRegistryLoader.h"

namespace {
MapRegistry buildFallbackMapRegistry() {
    MapRegistry mapRegistry;

    MapDefinition palletTown{};
    palletTown.id = "pallet_town";
    palletTown.mapPath = "assets/maps/pallet_town/pallet_town_map.tmx";
    palletTown.baseTilesetPath = "assets/tilesets/pallet_town/pallet_town_tileset.png";
    palletTown.coverTilesetPath = "";
    mapRegistry.addMap(palletTown);

    MapDefinition route1{};
    route1.id = "route_1";
    route1.mapPath = "assets/maps/route_1/route_1_map.tmx";
    route1.baseTilesetPath = "assets/tilesets/route_1/route_1_tileset.png";
    route1.coverTilesetPath = "";
    mapRegistry.addMap(route1);

    MapDefinition oaksLab{};
    oaksLab.id = "pallet_town_professor_oaks_lab";
    oaksLab.mapPath = "assets/maps/pallet_town_professor_oaks_lab/pallet_town_professor_oaks_lab_map.tmx";
    oaksLab.baseTilesetPath = "assets/tilesets/pallet_town_professor_oaks_lab/pallet_town_professor_oaks_lab_tileset.png";
    oaksLab.coverTilesetPath = "";
    mapRegistry.addMap(oaksLab);

    mapRegistry.addAlias("map_pallet_town", "pallet_town");
    mapRegistry.addAlias("route1", "route_1");
    mapRegistry.addAlias("map_route1", "route_1");
    mapRegistry.addAlias("map_route_1", "route_1");
    mapRegistry.addAlias("oak_lab", "pallet_town_professor_oaks_lab");
    mapRegistry.addAlias("map_pallet_town_professor_oaks_lab", "pallet_town_professor_oaks_lab");

    return mapRegistry;
}
} // namespace

MapRegistry buildDefaultMapRegistry() {
    MapRegistry dataDrivenRegistry;
    std::string error;
    if (loadMapRegistryFromLuaFile("assets/config/maps/map_registry.lua", dataDrivenRegistry, error)) {
        return dataDrivenRegistry;
    }

    if (!error.empty()) {
        std::cout << "Map registry config load failed; using fallback: " << error << '\n';
    }

    return buildFallbackMapRegistry();
}
