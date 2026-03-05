#include "game/world/DefaultMapRegistry.h"

MapRegistry buildDefaultMapRegistry() {
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

    mapRegistry.addAlias("map_pallet_town", "pallet_town");
    mapRegistry.addAlias("route1", "route_1");
    mapRegistry.addAlias("map_route1", "route_1");
    mapRegistry.addAlias("map_route_1", "route_1");

    return mapRegistry;
}
