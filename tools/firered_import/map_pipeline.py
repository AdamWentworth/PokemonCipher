from __future__ import annotations

import argparse
import os
from dataclasses import dataclass
from pathlib import Path

from PIL import Image

from .map_graphics import (
    MAPGRID_COLLISION_MASK,
    MAPGRID_COLLISION_SHIFT,
    MAPGRID_METATILE_ID_MASK,
    METATILE_ATTRIBUTE_BEHAVIOR_MASK,
    METATILE_ATTRIBUTE_LAYER_TYPE_MASK,
    METATILE_ATTRIBUTE_LAYER_TYPE_SHIFT,
    METATILE_ATTRIBUTE_TERRAIN_MASK,
    METATILE_ATTRIBUTE_TERRAIN_SHIFT,
    METATILE_LAYER_TYPE_NORMAL,
    METATILE_LAYER_TYPE_SPLIT,
    METATILE_LAYER_TYPE_COVERED,
    ORGANIZED_ATLAS_COLUMNS,
    TALL_GRASS_BEHAVIOR_IDS,
    TILE_TERRAIN_WATER,
    TILE_TERRAIN_WATERFALL,
    WATER_BEHAVIOR_IDS,
    build_global_palette_banks,
    build_organized_metatile_mapping,
    colorize_object_sprite,
    copy_rgba_sprite,
    decode_metatile_attributes,
    decode_metatiles,
    get_metatile_attributes,
    get_metatile_entry,
    load_palette_banks,
    read_layout,
    read_map_grid,
    render_layer_from_atlas,
    render_metatile,
    to_snake_case,
    write_red_animation_xml,
)
from .map_layout import (
    annotate_directional_stair_warps,
    annotate_target_spawn_ids,
    build_connection_warps,
    build_encounter_entries,
    build_oak_npc_events,
    carve_warp_tiles_from_collision,
    find_spawn_tile,
    merge_collision_runs,
    normalize_slug,
    read_spawn_tile_from_tmx,
)
from .map_writers import write_pcmap, write_tmx

DEFAULT_PROJECT_ROOT = Path(__file__).resolve().parents[2]


@dataclass(frozen=True)
class MapImportOptions:
    firered_root: Path = Path(r"C:\Code\pokefirered")
    project_root: Path = DEFAULT_PROJECT_ROOT
    assets_root: Path | None = None
    map_name: str = "PalletTown"
    map_slug: str | None = None
    layout_id: str | None = None
    spawn_tile_x: int | None = None
    spawn_tile_y: int | None = None


def import_map_assets(options: MapImportOptions) -> list[Path]:
    firered_root = options.firered_root.resolve()
    project_root = options.project_root.resolve()
    assets_root = options.assets_root.resolve() if options.assets_root is not None else (project_root / "assets")
    map_name = options.map_name
    map_dir = firered_root / "data" / "maps" / map_name
    map_json_path = map_dir / "map.json"
    if not map_json_path.exists():
        raise ValueError(f"Map JSON not found: {map_json_path}")

    import json

    map_json = json.loads(map_json_path.read_text(encoding="utf-8"))
    layout_id = options.layout_id or map_json.get("layout")
    if not layout_id:
        raise ValueError(f"No layout id found for map '{map_name}'.")

    layouts_path = firered_root / "data" / "layouts" / "layouts.json"
    layout = read_layout(layouts_path, layout_id)

    map_width = int(layout["width"])
    map_height = int(layout["height"])
    tile_size = 16

    primary_symbol = layout["primary_tileset"]
    secondary_symbol = layout["secondary_tileset"]

    primary_name = to_snake_case(primary_symbol.replace("gTileset_", ""))
    secondary_name = to_snake_case(secondary_symbol.replace("gTileset_", ""))

    primary_dir = firered_root / "data" / "tilesets" / "primary" / primary_name
    secondary_dir = firered_root / "data" / "tilesets" / "secondary" / secondary_name

    primary_tiles = Image.open(primary_dir / "tiles.png").convert("P")
    secondary_tiles = Image.open(secondary_dir / "tiles.png").convert("P")

    primary_palettes = load_palette_banks(primary_dir / "palettes")
    secondary_palettes = load_palette_banks(secondary_dir / "palettes")
    global_palettes = build_global_palette_banks(primary_palettes, secondary_palettes)

    primary_metatiles = decode_metatiles(primary_dir / "metatiles.bin")
    secondary_metatiles = decode_metatiles(secondary_dir / "metatiles.bin")
    primary_attributes = decode_metatile_attributes(primary_dir / "metatile_attributes.bin")
    secondary_attributes = decode_metatile_attributes(secondary_dir / "metatile_attributes.bin")

    blockdata_filepath = layout.get("blockdata_filepath")
    if not blockdata_filepath:
        raise ValueError(f"Layout '{layout_id}' has no blockdata filepath.")
    map_values = read_map_grid(firered_root / blockdata_filepath, map_width, map_height)

    metatile_ids = [value & MAPGRID_METATILE_ID_MASK for value in map_values]
    ordered_metatile_ids, base_gid_by_metatile_id = build_organized_metatile_mapping(metatile_ids)
    collision_bits = [(value & MAPGRID_COLLISION_MASK) >> MAPGRID_COLLISION_SHIFT for value in map_values]
    atlas_columns = ORGANIZED_ATLAS_COLUMNS
    cover_layer_values = [0] * len(metatile_ids)
    blocked_tiles = [False] * len(metatile_ids)
    tall_grass_tiles = [False] * len(metatile_ids)
    rendered_base_by_metatile_id: dict[int, Image.Image] = {}
    rendered_cover_by_metatile_id: dict[int, Image.Image] = {}
    cover_metatile_ids: list[int] = []

    for metatile_id in ordered_metatile_ids:
        entry = get_metatile_entry(metatile_id, primary_metatiles, secondary_metatiles)
        attributes = get_metatile_attributes(metatile_id, primary_attributes, secondary_attributes)
        layer_type = (attributes & METATILE_ATTRIBUTE_LAYER_TYPE_MASK) >> METATILE_ATTRIBUTE_LAYER_TYPE_SHIFT

        if entry is None:
            base_rendered = Image.new("RGBA", (tile_size, tile_size), (255, 0, 255, 255))
            cover_rendered = Image.new("RGBA", (tile_size, tile_size), (0, 0, 0, 0))
        else:
            draw_top_in_base = layer_type == METATILE_LAYER_TYPE_COVERED
            draw_top_in_cover = layer_type in (METATILE_LAYER_TYPE_NORMAL, METATILE_LAYER_TYPE_SPLIT)

            base_rendered = render_metatile(
                entry,
                primary_tiles,
                secondary_tiles,
                global_palettes,
                draw_bottom_layer=True,
                draw_top_layer=draw_top_in_base,
            )

            cover_rendered = render_metatile(
                entry,
                primary_tiles,
                secondary_tiles,
                global_palettes,
                draw_bottom_layer=False,
                draw_top_layer=draw_top_in_cover,
            )

        rendered_base_by_metatile_id[metatile_id] = base_rendered
        rendered_cover_by_metatile_id[metatile_id] = cover_rendered
        if cover_rendered.getbbox() is not None:
            cover_metatile_ids.append(metatile_id)

    cover_gid_start = len(ordered_metatile_ids) + 1
    cover_gid_by_metatile_id = {
        metatile_id: cover_gid_start + index
        for index, metatile_id in enumerate(cover_metatile_ids)
    }

    atlas_tile_count = len(ordered_metatile_ids) + len(cover_metatile_ids)
    atlas_rows = (atlas_tile_count + atlas_columns - 1) // atlas_columns
    organized_atlas = Image.new("RGBA", (atlas_columns * tile_size, atlas_rows * tile_size), (0, 0, 0, 0))

    for metatile_id in ordered_metatile_ids:
        base_tile_index = base_gid_by_metatile_id[metatile_id] - 1
        base_dst_x = (base_tile_index % atlas_columns) * tile_size
        base_dst_y = (base_tile_index // atlas_columns) * tile_size
        base_rendered = rendered_base_by_metatile_id[metatile_id]
        organized_atlas.paste(base_rendered, (base_dst_x, base_dst_y), base_rendered)

        cover_gid = cover_gid_by_metatile_id.get(metatile_id)
        if cover_gid is not None:
            cover_tile_index = cover_gid - 1
            cover_dst_x = (cover_tile_index % atlas_columns) * tile_size
            cover_dst_y = (cover_tile_index // atlas_columns) * tile_size
            cover_rendered = rendered_cover_by_metatile_id[metatile_id]
            organized_atlas.paste(cover_rendered, (cover_dst_x, cover_dst_y), cover_rendered)

    for index, metatile_id in enumerate(metatile_ids):
        attributes = get_metatile_attributes(metatile_id, primary_attributes, secondary_attributes)
        behavior = attributes & METATILE_ATTRIBUTE_BEHAVIOR_MASK
        terrain = (attributes & METATILE_ATTRIBUTE_TERRAIN_MASK) >> METATILE_ATTRIBUTE_TERRAIN_SHIFT
        layer_type = (attributes & METATILE_ATTRIBUTE_LAYER_TYPE_MASK) >> METATILE_ATTRIBUTE_LAYER_TYPE_SHIFT

        is_map_collision_blocked = collision_bits[index] != 0
        is_water_blocked = terrain in (TILE_TERRAIN_WATER, TILE_TERRAIN_WATERFALL) or behavior in WATER_BEHAVIOR_IDS
        blocked_tiles[index] = is_map_collision_blocked or is_water_blocked
        tall_grass_tiles[index] = behavior in TALL_GRASS_BEHAVIOR_IDS

        if layer_type in (METATILE_LAYER_TYPE_NORMAL, METATILE_LAYER_TYPE_SPLIT):
            cover_layer_values[index] = cover_gid_by_metatile_id.get(metatile_id, 0)

    map_slug = normalize_slug(options.map_slug or to_snake_case(map_json.get("name", map_name)))

    maps_dir = assets_root / "maps"
    tilesets_dir = assets_root / "tilesets"
    characters_dir = assets_root / "characters"
    animations_dir = assets_root / "animations"
    effects_dir = assets_root / "effects"
    map_output_dir = maps_dir / map_slug
    tileset_output_dir = tilesets_dir / map_slug

    map_output_dir.mkdir(parents=True, exist_ok=True)
    tileset_output_dir.mkdir(parents=True, exist_ok=True)

    tileset_output = tileset_output_dir / f"{map_slug}_tileset.png"
    organized_atlas.save(tileset_output)

    map_warp_events = [dict(warp) for warp in map_json.get("warp_events", [])]
    annotate_target_spawn_ids(map_warp_events)
    annotate_directional_stair_warps(
        map_warp_events,
        map_width,
        map_height,
        metatile_ids,
        primary_attributes,
        secondary_attributes,
    )

    warp_events = list(map_warp_events)
    carve_warp_tiles_from_collision(blocked_tiles, map_width, map_height, map_warp_events)

    connection_warps = build_connection_warps(
        map_json.get("connections", []),
        map_width,
        map_height,
        tile_size,
        maps_dir,
    )
    warp_events.extend(connection_warps)
    npc_events = build_oak_npc_events(map_json, map_name)
    encounter_entries = build_encounter_entries(
        tall_grass_tiles,
        map_width,
        map_height,
        tile_size,
        f"{map_slug}_grass",
    )

    collision_rects = merge_collision_runs(blocked_tiles, map_width, map_height, tile_size)

    tmx_output = map_output_dir / f"{map_slug}_map.tmx"

    preferred_spawn_x = options.spawn_tile_x
    preferred_spawn_y = options.spawn_tile_y
    existing_spawn_tile = read_spawn_tile_from_tmx(tmx_output, tile_size)
    if existing_spawn_tile is not None:
        if preferred_spawn_x is None:
            preferred_spawn_x = existing_spawn_tile[0]
        if preferred_spawn_y is None:
            preferred_spawn_y = existing_spawn_tile[1]

    spawn_tile_x, spawn_tile_y = find_spawn_tile(
        blocked_tiles,
        map_width,
        map_height,
        preferred_spawn_x,
        preferred_spawn_y,
    )
    spawn_x = spawn_tile_x * tile_size
    spawn_y = spawn_tile_y * tile_size
    ground_layer_values = [base_gid_by_metatile_id[metatile_id] for metatile_id in metatile_ids]
    tileset_image_rel_path = os.path.relpath(tileset_output, map_output_dir).replace("\\", "/")

    write_tmx(
        tmx_output,
        map_slug,
        map_width,
        map_height,
        tile_size,
        tileset_image_rel_path,
        atlas_columns,
        atlas_tile_count,
        ground_layer_values,
        cover_layer_values,
        collision_rects,
        spawn_x,
        spawn_y,
        warp_events,
        map_warp_events,
        encounter_entries,
        npc_events,
    )

    pcmap_output = map_output_dir / f"{map_slug}_map.pcmap"
    write_pcmap(
        pcmap_output,
        map_width,
        map_height,
        tile_size,
        ground_layer_values,
        cover_layer_values,
        collision_rects,
        spawn_x,
        spawn_y,
        warp_events,
        map_warp_events,
        encounter_entries,
        npc_events,
    )

    map_ground_image = render_layer_from_atlas(organized_atlas, ground_layer_values, map_width, map_height, tile_size)
    map_cover_image = render_layer_from_atlas(organized_atlas, cover_layer_values, map_width, map_height, tile_size)
    map_composite_image = map_ground_image.copy()
    map_composite_image.alpha_composite(map_cover_image)
    map_composite_output = map_output_dir / f"{map_slug}_map_preview.png"
    map_composite_image.save(map_composite_output)

    colorize_object_sprite(
        firered_root / "graphics" / "object_events" / "pics" / "people" / "red_normal.png",
        characters_dir / "red" / "red_normal.png",
        firered_root / "graphics" / "object_events" / "palettes" / "player.pal",
    )
    copy_rgba_sprite(
        firered_root / "graphics" / "field_effects" / "pics" / "tall_grass.png",
        effects_dir / "tall_grass_rustle.png",
    )

    write_red_animation_xml(animations_dir / "red_overworld.xml")

    generated = [
        tmx_output,
        pcmap_output,
        tileset_output,
        map_composite_output,
        characters_dir / "red" / "red_normal.png",
        effects_dir / "tall_grass_rustle.png",
        animations_dir / "red_overworld.xml",
    ]
    for path in generated:
        print(f"- {path}")
    return generated


def build_arg_parser() -> argparse.ArgumentParser:
    parser = argparse.ArgumentParser(description="Import a FireRed map into PokemonCipher asset format")
    parser.add_argument("--firered-root", type=Path, default=Path(r"C:\Code\pokefirered"))
    parser.add_argument("--project-root", type=Path, default=DEFAULT_PROJECT_ROOT)
    parser.add_argument(
        "--assets-root",
        type=Path,
        default=None,
        help="Output assets root (defaults to <project-root>/assets).",
    )
    parser.add_argument("--map-name", type=str, default="PalletTown", help="FireRed map directory name under data/maps")
    parser.add_argument("--map-slug", type=str, default=None, help="Output folder/file slug (defaults from map name)")
    parser.add_argument("--layout-id", type=str, default=None, help="Optional layout id override")
    parser.add_argument("--spawn-tile-x", type=int, default=None, help="Optional preferred spawn tile x")
    parser.add_argument("--spawn-tile-y", type=int, default=None, help="Optional preferred spawn tile y")
    return parser


def parse_options_from_args(argv: list[str] | None = None) -> MapImportOptions:
    args = build_arg_parser().parse_args(argv)
    return MapImportOptions(
        firered_root=args.firered_root,
        project_root=args.project_root,
        assets_root=args.assets_root,
        map_name=args.map_name,
        map_slug=args.map_slug,
        layout_id=args.layout_id,
        spawn_tile_x=args.spawn_tile_x,
        spawn_tile_y=args.spawn_tile_y,
    )


def main() -> None:
    options = parse_options_from_args()
    print(f"Imported FireRed map assets for {options.map_name}:")
    import_map_assets(options)
