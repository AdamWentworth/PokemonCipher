from __future__ import annotations

import json
import math
import struct
from pathlib import Path

from PIL import Image

from .palette_utils import parse_jasc_palette

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
    0x10,
    0x11,
    0x12,
    0x13,
    0x15,
    0x16,
    0x17,
    0x1A,
    0x1B,
}

TALL_GRASS_BEHAVIOR_IDS = {
    0x02,
    0xD1,
}

ORGANIZED_ATLAS_COLUMNS = 16


def to_snake_case(symbol_name: str) -> str:
    chars = []
    for index, ch in enumerate(symbol_name):
        if index > 0:
            prev = symbol_name[index - 1]
            is_word_boundary = ch.isupper() and prev.islower()
            is_alpha_num_boundary = (ch.isdigit() and prev.isalpha()) or (ch.isalpha() and prev.isdigit())
            if is_word_boundary or is_alpha_num_boundary:
                chars.append("_")
        chars.append(ch.lower())
    return "".join(chars)
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


def copy_rgba_sprite(source_path: Path, output_path: Path) -> None:
    sprite = Image.open(source_path).convert("RGBA")
    output_path.parent.mkdir(parents=True, exist_ok=True)
    sprite.save(output_path)


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
