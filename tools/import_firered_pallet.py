import argparse
import json
import math
import os
import struct
from pathlib import Path
from xml.sax.saxutils import escape

from PIL import Image

MAPGRID_METATILE_ID_MASK = 0x03FF
MAPGRID_COLLISION_MASK = 0x0C00
MAPGRID_COLLISION_SHIFT = 10
NUM_PALS_IN_PRIMARY = 7
NUM_PALS_TOTAL = 13

METATILE_ATTRIBUTE_BEHAVIOR_MASK = 0x000001FF
METATILE_ATTRIBUTE_TERRAIN_MASK = 0x00003E00
METATILE_ATTRIBUTE_TERRAIN_SHIFT = 9
METATILE_ATTRIBUTE_LAYER_TYPE_MASK = 0x60000000
METATILE_ATTRIBUTE_LAYER_TYPE_SHIFT = 29

METATILE_LAYER_TYPE_NORMAL = 0
METATILE_LAYER_TYPE_COVERED = 1
METATILE_LAYER_TYPE_SPLIT = 2

TILE_TERRAIN_WATER = 2
TILE_TERRAIN_WATERFALL = 3

WATER_BEHAVIOR_IDS = {
    0x10,  # MB_POND_WATER
    0x11,  # MB_FAST_WATER
    0x12,  # MB_DEEP_WATER
    0x13,  # MB_WATERFALL
    0x15,  # MB_OCEAN_WATER
    0x16,  # MB_PUDDLE
    0x17,  # MB_SHALLOW_WATER
    0x1A,  # MB_UNUSED_WATER
    0x1B,  # MB_CYCLING_ROAD_WATER
}

ORGANIZED_ATLAS_COLUMNS = 16


def to_snake_case(symbol_name: str) -> str:
    chars = []
    for index, ch in enumerate(symbol_name):
        if ch.isupper() and index > 0 and symbol_name[index - 1].islower():
            chars.append("_")
        chars.append(ch.lower())
    return "".join(chars)


def parse_jasc_palette(path: Path) -> list[tuple[int, int, int]]:
    lines = path.read_text(encoding="utf-8").splitlines()
    if len(lines) < 19 or lines[0].strip() != "JASC-PAL":
        raise ValueError(f"Unexpected palette format: {path}")

    colors = []
    for line in lines[3:19]:
        r, g, b = (int(x) for x in line.split())
        colors.append((r, g, b))
    return colors


def load_palette_banks(directory: Path) -> list[list[tuple[int, int, int]]]:
    banks = []
    for index in range(16):
        palette_path = directory / f"{index:02d}.pal"
        banks.append(parse_jasc_palette(palette_path))
    return banks


def decode_metatiles(path: Path) -> list[tuple[int, ...]]:
    data = path.read_bytes()
    values = struct.unpack(f"<{len(data) // 2}H", data)
    return [tuple(values[i:i + 8]) for i in range(0, len(values), 8)]


def decode_metatile_attributes(path: Path) -> list[int]:
    data = path.read_bytes()
    return list(struct.unpack(f"<{len(data) // 4}I", data))


def build_global_palette_banks(
    primary_palettes: list[list[tuple[int, int, int]]],
    secondary_palettes: list[list[tuple[int, int, int]]],
) -> list[list[tuple[int, int, int]]]:
    # FireRed maps load primary palettes into slots 0-6 and secondary into 7-12.
    global_palettes: list[list[tuple[int, int, int]]] = []
    default_palette = [(0, 0, 0)] * 16

    for slot in range(16):
        if slot < NUM_PALS_IN_PRIMARY:
            palette = primary_palettes[slot] if slot < len(primary_palettes) else default_palette
        elif slot < NUM_PALS_TOTAL:
            palette = secondary_palettes[slot] if slot < len(secondary_palettes) else default_palette
        elif slot < len(primary_palettes):
            palette = primary_palettes[slot]
        elif slot < len(secondary_palettes):
            palette = secondary_palettes[slot]
        else:
            palette = default_palette

        global_palettes.append(palette)

    return global_palettes


def extract_tile_rgba(
    primary_tiles_image: Image.Image,
    secondary_tiles_image: Image.Image,
    tile_index: int,
    palette_banks: list[list[tuple[int, int, int]]],
    palette_bank: int,
    hflip: bool,
    vflip: bool,
) -> Image.Image:
    primary_tiles_per_row = primary_tiles_image.width // 8
    primary_tile_count = primary_tiles_per_row * (primary_tiles_image.height // 8)

    if tile_index < primary_tile_count:
        source_image = primary_tiles_image
        source_index = tile_index
    else:
        secondary_tiles_per_row = secondary_tiles_image.width // 8
        secondary_tile_count = secondary_tiles_per_row * (secondary_tiles_image.height // 8)
        source_index = tile_index - primary_tile_count
        if source_index < 0 or source_index >= secondary_tile_count:
            return Image.new("RGBA", (8, 8), (255, 0, 255, 255))
        source_image = secondary_tiles_image

    tiles_per_row = source_image.width // 8
    src_x = (source_index % tiles_per_row) * 8
    src_y = (source_index // tiles_per_row) * 8

    tile = source_image.crop((src_x, src_y, src_x + 8, src_y + 8))
    indices = list(tile.getdata())

    output = Image.new("RGBA", (8, 8), (0, 0, 0, 0))
    output_pixels = []

    bank = palette_banks[min(max(palette_bank, 0), len(palette_banks) - 1)]
    for index in indices:
        if index == 0:
            output_pixels.append((0, 0, 0, 0))
        else:
            r, g, b = bank[index]
            output_pixels.append((r, g, b, 255))

    output.putdata(output_pixels)

    if hflip:
        output = output.transpose(Image.FLIP_LEFT_RIGHT)
    if vflip:
        output = output.transpose(Image.FLIP_TOP_BOTTOM)

    return output


def render_metatile(
    entry_values: tuple[int, ...],
    primary_tiles_image: Image.Image,
    secondary_tiles_image: Image.Image,
    palette_banks: list[list[tuple[int, int, int]]],
    *,
    draw_bottom_layer: bool,
    draw_top_layer: bool,
) -> Image.Image:
    metatile = Image.new("RGBA", (16, 16), (0, 0, 0, 0))
    positions = [(0, 0), (8, 0), (0, 8), (8, 8)]

    layer_starts = []
    if draw_bottom_layer:
        layer_starts.append(0)
    if draw_top_layer:
        layer_starts.append(4)

    for layer_start in layer_starts:
        for i in range(4):
            tile_value = entry_values[layer_start + i]
            tile_index = tile_value & 0x03FF
            hflip = bool(tile_value & 0x0400)
            vflip = bool(tile_value & 0x0800)
            palette_bank = (tile_value >> 12) & 0x0F

            rendered_tile = extract_tile_rgba(
                primary_tiles_image,
                secondary_tiles_image,
                tile_index,
                palette_banks,
                palette_bank,
                hflip,
                vflip,
            )
            metatile.paste(rendered_tile, positions[i], rendered_tile)

    return metatile


def get_metatile_entry(
    metatile_id: int,
    primary_entries: list[tuple[int, ...]],
    secondary_entries: list[tuple[int, ...]],
) -> tuple[int, ...] | None:
    if metatile_id < len(primary_entries):
        return primary_entries[metatile_id]

    secondary_index = metatile_id - len(primary_entries)
    if 0 <= secondary_index < len(secondary_entries):
        return secondary_entries[secondary_index]

    return None


def get_metatile_attributes(
    metatile_id: int,
    primary_attributes: list[int],
    secondary_attributes: list[int],
) -> int:
    if metatile_id < len(primary_attributes):
        return primary_attributes[metatile_id]

    secondary_index = metatile_id - len(primary_attributes)
    if 0 <= secondary_index < len(secondary_attributes):
        return secondary_attributes[secondary_index]

    return 0


def read_layout(layouts_path: Path, layout_id: str) -> dict:
    layouts_data = json.loads(layouts_path.read_text(encoding="utf-8"))
    for layout in layouts_data["layouts"]:
        if isinstance(layout, dict) and layout.get("id") == layout_id:
            return layout
    raise ValueError(f"Layout '{layout_id}' not found in {layouts_path}")


def read_map_grid(path: Path, width: int, height: int) -> list[int]:
    values = list(struct.unpack(f"<{path.stat().st_size // 2}H", path.read_bytes()))
    expected = width * height
    if len(values) != expected:
        raise ValueError(f"Unexpected map size in {path}: expected {expected} words, got {len(values)}")
    return values


def merge_collision_runs(blocked_grid: list[bool], width: int, height: int, tile_size: int) -> list[tuple[int, int, int, int]]:
    rectangles = []
    for y in range(height):
        x = 0
        while x < width:
            idx = y * width + x
            if not blocked_grid[idx]:
                x += 1
                continue

            run_start = x
            while x < width and blocked_grid[y * width + x]:
                x += 1

            run_width = x - run_start
            rectangles.append((run_start * tile_size, y * tile_size, run_width * tile_size, tile_size))
    return rectangles


def build_organized_metatile_mapping(metatile_ids: list[int]) -> tuple[list[int], dict[int, int]]:
    ordered_ids = sorted(set(metatile_ids))
    base_gid_by_metatile_id = {old_id: index + 1 for index, old_id in enumerate(ordered_ids)}
    return ordered_ids, base_gid_by_metatile_id


def render_layer_from_atlas(
    atlas_image: Image.Image,
    layer_values: list[int],
    map_width: int,
    map_height: int,
    tile_size: int,
) -> Image.Image:
    rendered = Image.new("RGBA", (map_width * tile_size, map_height * tile_size), (0, 0, 0, 0))
    atlas_columns = atlas_image.width // tile_size

    for index, gid in enumerate(layer_values):
        if gid <= 0:
            continue

        tile_id = gid - 1
        src_x = (tile_id % atlas_columns) * tile_size
        src_y = (tile_id // atlas_columns) * tile_size
        tile = atlas_image.crop((src_x, src_y, src_x + tile_size, src_y + tile_size))

        dst_x = (index % map_width) * tile_size
        dst_y = (index // map_width) * tile_size
        rendered.paste(tile, (dst_x, dst_y), tile)

    return rendered


def write_tmx(
    tmx_path: Path,
    map_width: int,
    map_height: int,
    tile_size: int,
    tileset_image_rel_path: str,
    atlas_columns: int,
    atlas_tile_count: int,
    ground_layer_values: list[int],
    cover_layer_values: list[int],
    collision_rects: list[tuple[int, int, int, int]],
    spawn_x: int,
    spawn_y: int,
    warp_events: list[dict],
) -> None:
    ground_csv_lines = []
    for row in range(map_height):
        start = row * map_width
        row_values = ground_layer_values[start:start + map_width]
        ground_csv_lines.append(",".join(str(value) for value in row_values))
    ground_csv_data = ",\n".join(ground_csv_lines)

    cover_csv_lines = []
    for row in range(map_height):
        start = row * map_width
        row_values = cover_layer_values[start:start + map_width]
        cover_csv_lines.append(",".join(str(value) for value in row_values))
    cover_csv_data = ",\n".join(cover_csv_lines)

    collision_objects = []
    for i, (x, y, w, h) in enumerate(collision_rects, start=1):
        collision_objects.append(f'  <object id="{i}" x="{x}" y="{y}" width="{w}" height="{h}"/>')

    warp_objects = []
    for i, warp in enumerate(warp_events, start=1):
        dest_map = warp.get("dest_map", "")
        x = int(warp.get("x", 0)) * tile_size
        y = int(warp.get("y", 0)) * tile_size
        warp_objects.append(
            "\n".join([
                f'  <object id="{i}" x="{x}" y="{y}" width="{tile_size}" height="{tile_size}">',
                "   <properties>",
                f'    <property name="target_map" value="{escape(dest_map)}"/>',
                "   </properties>",
                "  </object>",
            ])
        )

    xml = "\n".join([
        "<?xml version=\"1.0\" encoding=\"UTF-8\"?>",
        (
            f"<map version=\"1.10\" tiledversion=\"1.11.2\" orientation=\"orthogonal\" "
            f"renderorder=\"right-down\" width=\"{map_width}\" height=\"{map_height}\" "
            f"tilewidth=\"{tile_size}\" tileheight=\"{tile_size}\" infinite=\"0\">"
        ),
        f" <tileset firstgid=\"1\" name=\"pallet_town\" tilewidth=\"{tile_size}\" tileheight=\"{tile_size}\" tilecount=\"{atlas_tile_count}\" columns=\"{atlas_columns}\">",
        f"  <image source=\"{escape(tileset_image_rel_path)}\" width=\"{atlas_columns * tile_size}\" height=\"{math.ceil(atlas_tile_count / atlas_columns) * tile_size}\"/>",
        " </tileset>",
        f" <layer id=\"1\" name=\"Terrain Layer\" width=\"{map_width}\" height=\"{map_height}\">",
        "  <data encoding=\"csv\">",
        ground_csv_data,
"</data>",
        " </layer>",
        f" <layer id=\"2\" name=\"Cover Layer\" width=\"{map_width}\" height=\"{map_height}\">",
        "  <data encoding=\"csv\">",
        cover_csv_data,
"</data>",
        " </layer>",
        " <objectgroup id=\"3\" name=\"Collision Layer\">",
        *collision_objects,
        " </objectgroup>",
        " <objectgroup id=\"4\" name=\"Player Spawn Points\">",
        f"  <object id=\"1\" name=\"default\" x=\"{spawn_x}\" y=\"{spawn_y}\"><point/></object>",
        " </objectgroup>",
        " <objectgroup id=\"5\" name=\"Warp Layer\">",
        *warp_objects,
        " </objectgroup>",
        "</map>",
        "",
    ])

    tmx_path.write_text(xml, encoding="utf-8")


def colorize_object_sprite(indexed_sprite_path: Path, output_path: Path, palette_path: Path) -> None:
    sprite = Image.open(indexed_sprite_path).convert("P")
    palette = parse_jasc_palette(palette_path)

    rgba = Image.new("RGBA", sprite.size, (0, 0, 0, 0))
    pixels = []
    for index in sprite.getdata():
        if index == 0:
            pixels.append((0, 0, 0, 0))
        else:
            r, g, b = palette[index]
            pixels.append((r, g, b, 255))

    rgba.putdata(pixels)
    output_path.parent.mkdir(parents=True, exist_ok=True)
    rgba.save(output_path)


def write_red_animation_xml(path: Path) -> None:
    content = """<animations>
    <walk_down>
        <frame x=\"48\" y=\"0\" w=\"16\" h=\"32\" />
        <frame x=\"0\" y=\"0\" w=\"16\" h=\"32\" />
        <frame x=\"64\" y=\"0\" w=\"16\" h=\"32\" />
        <frame x=\"0\" y=\"0\" w=\"16\" h=\"32\" />
    </walk_down>
    <walk_up>
        <frame x=\"80\" y=\"0\" w=\"16\" h=\"32\" />
        <frame x=\"16\" y=\"0\" w=\"16\" h=\"32\" />
        <frame x=\"96\" y=\"0\" w=\"16\" h=\"32\" />
        <frame x=\"16\" y=\"0\" w=\"16\" h=\"32\" />
    </walk_up>
    <walk_left>
        <frame x=\"112\" y=\"0\" w=\"16\" h=\"32\" />
        <frame x=\"32\" y=\"0\" w=\"16\" h=\"32\" />
        <frame x=\"128\" y=\"0\" w=\"16\" h=\"32\" />
        <frame x=\"32\" y=\"0\" w=\"16\" h=\"32\" />
    </walk_left>
    <walk_right>
        <frame x=\"112\" y=\"0\" w=\"16\" h=\"32\" />
        <frame x=\"32\" y=\"0\" w=\"16\" h=\"32\" />
        <frame x=\"128\" y=\"0\" w=\"16\" h=\"32\" />
        <frame x=\"32\" y=\"0\" w=\"16\" h=\"32\" />
    </walk_right>
    <idle_down>
        <frame x=\"0\" y=\"0\" w=\"16\" h=\"32\" />
    </idle_down>
    <idle_up>
        <frame x=\"16\" y=\"0\" w=\"16\" h=\"32\" />
    </idle_up>
    <idle_left>
        <frame x=\"32\" y=\"0\" w=\"16\" h=\"32\" />
    </idle_left>
    <idle_right>
        <frame x=\"32\" y=\"0\" w=\"16\" h=\"32\" />
    </idle_right>
</animations>
"""
    path.parent.mkdir(parents=True, exist_ok=True)
    path.write_text(content, encoding="utf-8")


def main() -> None:
    parser = argparse.ArgumentParser(description="Import FireRed Pallet Town assets for PokemonCipher")
    parser.add_argument("--firered-root", type=Path, default=Path(r"C:\Code\pokefirered"))
    parser.add_argument("--project-root", type=Path, default=Path(__file__).resolve().parents[1])
    args = parser.parse_args()

    firered_root = args.firered_root
    project_root = args.project_root

    layouts_path = firered_root / "data" / "layouts" / "layouts.json"
    layout = read_layout(layouts_path, "LAYOUT_PALLET_TOWN")

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

    map_values = read_map_grid(firered_root / "data" / "layouts" / "PalletTown" / "map.bin", map_width, map_height)

    metatile_ids = [value & MAPGRID_METATILE_ID_MASK for value in map_values]
    ordered_metatile_ids, base_gid_by_metatile_id = build_organized_metatile_mapping(metatile_ids)
    collision_bits = [(value & MAPGRID_COLLISION_MASK) >> MAPGRID_COLLISION_SHIFT for value in map_values]
    atlas_columns = ORGANIZED_ATLAS_COLUMNS
    cover_layer_values = [0] * len(metatile_ids)
    blocked_tiles = [False] * len(metatile_ids)
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
    atlas_rows = math.ceil(atlas_tile_count / atlas_columns)
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

        if layer_type in (METATILE_LAYER_TYPE_NORMAL, METATILE_LAYER_TYPE_SPLIT):
            cover_layer_values[index] = cover_gid_by_metatile_id.get(metatile_id, 0)

    assets_dir = project_root / "assets"
    maps_dir = assets_dir / "maps"
    tilesets_dir = assets_dir / "tilesets"
    characters_dir = assets_dir / "characters"
    animations_dir = assets_dir / "animations"
    map_slug = "pallet_town"
    map_output_dir = maps_dir / map_slug
    tileset_key = f"{primary_name}__{secondary_name}"
    tileset_output_dir = tilesets_dir / "frlg" / tileset_key

    map_output_dir.mkdir(parents=True, exist_ok=True)
    tileset_output_dir.mkdir(parents=True, exist_ok=True)

    tileset_output = tileset_output_dir / "metatiles_organized.png"
    organized_atlas.save(tileset_output)

    map_json = json.loads((firered_root / "data" / "maps" / "PalletTown" / "map.json").read_text(encoding="utf-8"))
    warp_events = map_json.get("warp_events", [])

    collision_rects = merge_collision_runs(blocked_tiles, map_width, map_height, tile_size)

    spawn_x = 6 * tile_size
    spawn_y = 9 * tile_size
    ground_layer_values = [base_gid_by_metatile_id[metatile_id] for metatile_id in metatile_ids]
    tmx_output = map_output_dir / "map.tmx"
    tileset_image_rel_path = os.path.relpath(tileset_output, map_output_dir).replace("\\", "/")

    write_tmx(
        tmx_output,
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
    )

    map_ground_image = render_layer_from_atlas(organized_atlas, ground_layer_values, map_width, map_height, tile_size)
    map_cover_image = render_layer_from_atlas(organized_atlas, cover_layer_values, map_width, map_height, tile_size)
    map_composite_image = map_ground_image.copy()
    map_composite_image.alpha_composite(map_cover_image)
    map_composite_output = map_output_dir / "preview.png"
    map_composite_image.save(map_composite_output)

    colorize_object_sprite(
        firered_root / "graphics" / "object_events" / "pics" / "people" / "red_normal.png",
        characters_dir / "red_normal.png",
        firered_root / "graphics" / "object_events" / "palettes" / "player.pal",
    )

    write_red_animation_xml(animations_dir / "red_overworld.xml")

    print("Imported FireRed Pallet Town assets:")
    print(f"- {tmx_output}")
    print(f"- {tileset_output}")
    print(f"- {map_composite_output}")
    print(f"- {characters_dir / 'red_normal.png'}")
    print(f"- {animations_dir / 'red_overworld.xml'}")


if __name__ == "__main__":
    main()
