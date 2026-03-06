"""Legacy map importer retained temporarily during modularization."""

import argparse
import json
import math
import os
import re
import struct
from dataclasses import dataclass
from pathlib import Path
from xml.etree import ElementTree
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

TALL_GRASS_BEHAVIOR_IDS = {
    0x02,  # MB_TALL_GRASS
    0xD1,  # MB_CYCLING_ROAD_PULL_DOWN_GRASS
}

STAIR_WARP_RIGHT_BEHAVIOR_IDS = {
    0x6C,  # MB_UP_RIGHT_STAIR_WARP
    0x6E,  # MB_DOWN_RIGHT_STAIR_WARP
}

STAIR_WARP_LEFT_BEHAVIOR_IDS = {
    0x6D,  # MB_UP_LEFT_STAIR_WARP
    0x6F,  # MB_DOWN_LEFT_STAIR_WARP
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


def normalize_slug(value: str) -> str:
    chars: list[str] = []
    previous_was_separator = False

    for ch in value:
        if ch.isalnum():
            chars.append(ch.lower())
            previous_was_separator = False
        else:
            if not previous_was_separator:
                chars.append("_")
                previous_was_separator = True

    slug = "".join(chars).strip("_")
    return slug or "map"


def find_spawn_tile(
    blocked_grid: list[bool],
    width: int,
    height: int,
    preferred_x: int | None = None,
    preferred_y: int | None = None,
) -> tuple[int, int]:
    if width <= 0 or height <= 0:
        return 0, 0

    start_x = preferred_x if preferred_x is not None else width // 2
    start_y = preferred_y if preferred_y is not None else height // 2
    start_x = max(0, min(width - 1, start_x))
    start_y = max(0, min(height - 1, start_y))

    best_x = start_x
    best_y = start_y
    best_distance = float("inf")

    for y in range(height):
        for x in range(width):
            index = y * width + x
            if blocked_grid[index]:
                continue

            dx = x - start_x
            dy = y - start_y
            distance = float((dx * dx) + (dy * dy))
            if distance < best_distance:
                best_distance = distance
                best_x = x
                best_y = y

    return best_x, best_y


def read_spawn_tile_from_tmx(tmx_path: Path, tile_size: int) -> tuple[int, int] | None:
    if not tmx_path.exists() or tile_size <= 0:
        return None

    try:
        root = ElementTree.fromstring(tmx_path.read_text(encoding="utf-8"))
    except Exception:
        return None

    for object_group in root.findall("objectgroup"):
        group_name = object_group.attrib.get("name", "")
        if "Spawn" not in group_name and "Player" not in group_name:
            continue

        for object_node in object_group.findall("object"):
            object_name = object_node.attrib.get("name", "").lower()
            if object_name and object_name != "default":
                continue

            try:
                spawn_x = int(float(object_node.attrib.get("x", "0"))) // tile_size
                spawn_y = int(float(object_node.attrib.get("y", "0"))) // tile_size
                return spawn_x, spawn_y
            except ValueError:
                continue

    return None


def read_tmx_metrics(tmx_path: Path) -> tuple[int, int, int, int] | None:
    if not tmx_path.exists():
        return None

    try:
        root = ElementTree.fromstring(tmx_path.read_text(encoding="utf-8"))
    except Exception:
        return None

    try:
        width = int(root.attrib.get("width", "0"))
        height = int(root.attrib.get("height", "0"))
        tile_width = int(root.attrib.get("tilewidth", "16"))
        tile_height = int(root.attrib.get("tileheight", "16"))
    except ValueError:
        return None

    if width <= 0 or height <= 0 or tile_width <= 0 or tile_height <= 0:
        return None

    return width, height, tile_width, tile_height


def map_constant_to_slug(map_constant: str) -> str:
    normalized = map_constant
    if normalized.startswith("MAP_"):
        normalized = normalized[4:]
    return normalize_slug(normalized)


def map_constant_slug_candidates(map_constant: str) -> list[str]:
    raw = map_constant[4:] if map_constant.startswith("MAP_") else map_constant
    candidates = [normalize_slug(raw)]

    with_underscores = re.sub(r"([A-Za-z])([0-9])", r"\1_\2", raw)
    with_underscores = re.sub(r"([0-9])([A-Za-z])", r"\1_\2", with_underscores)
    candidates.append(normalize_slug(with_underscores))

    deduped: list[str] = []
    for candidate in candidates:
        if candidate and candidate not in deduped:
            deduped.append(candidate)
    return deduped


def build_connection_warps(
    map_connections: list[dict] | None,
    map_width: int,
    map_height: int,
    tile_size: int,
    maps_dir: Path,
) -> list[dict]:
    warps: list[dict] = []
    if not map_connections:
        return warps

    source_mid_x = map_width // 2
    source_mid_y = map_height // 2

    for connection in map_connections:
        target_map = str(connection.get("map", ""))
        direction = str(connection.get("direction", "")).lower()

        try:
            offset = int(connection.get("offset", 0))
        except (TypeError, ValueError):
            offset = 0

        target_slug = ""
        target_metrics = None
        for slug_candidate in map_constant_slug_candidates(target_map):
            candidate_tmx = maps_dir / slug_candidate / f"{slug_candidate}_map.tmx"
            target_metrics = read_tmx_metrics(candidate_tmx)
            if target_metrics is not None:
                target_slug = slug_candidate
                break

        if target_metrics is None:
            continue

        target_width, target_height, target_tile_width, target_tile_height = target_metrics

        if direction == "up":
            x = 0
            y = 0
            width = map_width
            height = 1
            target_x_tile = max(1, min(target_width - 2, source_mid_x + offset))
            target_y_tile = max(0, target_height - 2)
        elif direction == "down":
            x = 0
            y = max(0, map_height - 1)
            width = map_width
            height = 1
            target_x_tile = max(1, min(target_width - 2, source_mid_x + offset))
            target_y_tile = 1 if target_height > 1 else 0
        elif direction == "left":
            x = 0
            y = 0
            width = 1
            height = map_height
            target_x_tile = max(0, target_width - 2)
            target_y_tile = max(1, min(target_height - 2, source_mid_y + offset))
        elif direction == "right":
            x = max(0, map_width - 1)
            y = 0
            width = 1
            height = map_height
            target_x_tile = 1 if target_width > 1 else 0
            target_y_tile = max(1, min(target_height - 2, source_mid_y + offset))
        else:
            continue

        warp = {
            "x": x,
            "y": y,
            "width": width,
            "height": height,
            "dest_map": target_map,
            "target_spawn_x": target_x_tile * target_tile_width,
            "target_spawn_y": target_y_tile * target_tile_height,
        }
        warps.append(warp)

    return warps


def parse_warp_id(value: object) -> int | None:
    if value is None:
        return None

    if isinstance(value, int):
        return value if value >= 0 else None

    try:
        parsed = int(str(value), 0)
    except (TypeError, ValueError):
        return None

    return parsed if parsed >= 0 else None


def annotate_target_spawn_ids(map_warp_events: list[dict]) -> None:
    for warp in map_warp_events:
        warp_id = parse_warp_id(warp.get("dest_warp_id"))
        if warp_id is None:
            continue

        warp["target_spawn_id"] = f"warp_{warp_id}"


def annotate_directional_stair_warps(
    map_warp_events: list[dict],
    map_width: int,
    map_height: int,
    metatile_ids: list[int],
    primary_attributes: list[int],
    secondary_attributes: list[int],
) -> None:
    for warp in map_warp_events:
        try:
            x = int(warp.get("x", 0))
            y = int(warp.get("y", 0))
        except (TypeError, ValueError):
            continue

        if x < 0 or y < 0 or x >= map_width or y >= map_height:
            continue

        metatile_id = metatile_ids[y * map_width + x]
        attributes = get_metatile_attributes(metatile_id, primary_attributes, secondary_attributes)
        behavior = attributes & METATILE_ATTRIBUTE_BEHAVIOR_MASK
        if behavior in STAIR_WARP_LEFT_BEHAVIOR_IDS:
            warp["required_direction"] = "left"
        elif behavior in STAIR_WARP_RIGHT_BEHAVIOR_IDS:
            warp["required_direction"] = "right"


def movement_type_to_facing(movement_type: str) -> str:
    normalized = movement_type.upper()
    if "FACE_UP" in normalized:
        return "up"
    if "FACE_LEFT" in normalized:
        return "left"
    if "FACE_RIGHT" in normalized:
        return "right"
    return "down"


def build_oak_npc_events(map_json: dict, map_name: str) -> list[dict]:
    if map_name != "PalletTown_ProfessorOaksLab":
        return []

    npc_events: list[dict] = []
    for obj in map_json.get("object_events", []):
        graphics_id = str(obj.get("graphics_id", ""))
        local_id = str(obj.get("local_id", ""))
        graphics_upper = graphics_id.upper()
        local_upper = local_id.upper()
        has_oak_graphic = "PROF_OAK" in graphics_upper
        has_oak_token = re.search(r"(^|_)OAK($|_)", local_upper) is not None
        if not has_oak_graphic and not has_oak_token:
            continue

        try:
            x = int(obj.get("x", 0))
            y = int(obj.get("y", 0))
        except (TypeError, ValueError):
            continue

        npc_id = normalize_slug(local_id or graphics_id or f"oak_{len(npc_events)}")
        movement_type = str(obj.get("movement_type", ""))
        npc_events.append(
            {
                "id": npc_id,
                "x": x,
                "y": y,
                "width": 1,
                "height": 1,
                "facing": movement_type_to_facing(movement_type),
                "script_id": "oak_lab_eevee",
                "speaker": "PROF. OAK",
                "sprite": "assets/characters/oak/oak_overworld.png",
                "animation": "assets/animations/red_overworld.xml",
            }
        )

    return npc_events


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


def build_encounter_entries(
    grass_grid: list[bool],
    width: int,
    height: int,
    tile_size: int,
    table_id: str,
) -> list[dict]:
    entries: list[dict] = []
    for x, y, w, h in merge_collision_runs(grass_grid, width, height, tile_size):
        entries.append(
            {
                "x": float(x),
                "y": float(y),
                "w": float(w),
                "h": float(h),
                "table_id": table_id,
            }
        )
    return entries


def carve_warp_tiles_from_collision(
    blocked_grid: list[bool],
    width: int,
    height: int,
    warp_events: list[dict],
) -> None:
    for warp in warp_events:
        try:
            start_x = int(warp.get("x", 0))
            start_y = int(warp.get("y", 0))
        except (TypeError, ValueError):
            continue

        try:
            warp_width = max(1, int(warp.get("width", 1)))
            warp_height = max(1, int(warp.get("height", 1)))
        except (TypeError, ValueError):
            warp_width = 1
            warp_height = 1

        for dy in range(warp_height):
            tile_y = start_y + dy
            if tile_y < 0 or tile_y >= height:
                continue

            for dx in range(warp_width):
                tile_x = start_x + dx
                if tile_x < 0 or tile_x >= width:
                    continue

                blocked_grid[tile_y * width + tile_x] = False


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
    tileset_name: str,
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
    map_warp_events: list[dict],
    encounter_entries: list[dict],
    npc_events: list[dict],
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

    spawn_objects = [f'  <object id="1" name="default" x="{spawn_x}" y="{spawn_y}"><point/></object>']
    for i, warp in enumerate(map_warp_events, start=1):
        warp_spawn_x = int(warp.get("x", 0)) * tile_size
        warp_spawn_y = int(warp.get("y", 0)) * tile_size
        spawn_objects.append(
            f'  <object id="{i + 1}" name="warp_{i - 1}" x="{warp_spawn_x}" y="{warp_spawn_y}"><point/></object>'
        )

    warp_objects = []
    for i, warp in enumerate(warp_events, start=1):
        dest_map = warp.get("dest_map", "")
        x = int(warp.get("x", 0)) * tile_size
        y = int(warp.get("y", 0)) * tile_size
        width = int(warp.get("width", 1)) * tile_size
        height = int(warp.get("height", 1)) * tile_size

        target_spawn_x = warp.get("target_spawn_x")
        target_spawn_y = warp.get("target_spawn_y")
        target_spawn_id = str(warp.get("target_spawn_id", "")).strip()
        required_direction = str(warp.get("required_direction", "")).strip().lower()
        properties = [f'    <property name="target_map" value="{escape(dest_map)}"/>']
        if target_spawn_id:
            properties.append(f'    <property name="target_spawn_id" value="{escape(target_spawn_id)}"/>')
        if required_direction:
            properties.append(f'    <property name="required_direction" value="{escape(required_direction)}"/>')
        if target_spawn_x is not None and target_spawn_y is not None:
            properties.append(f'    <property name="target_spawn_x" value="{target_spawn_x}"/>')
            properties.append(f'    <property name="target_spawn_y" value="{target_spawn_y}"/>')

        warp_objects.append(
            "\n".join([
                f'  <object id="{i}" x="{x}" y="{y}" width="{width}" height="{height}">',
                "   <properties>",
                *properties,
                "   </properties>",
                "  </object>",
            ])
        )

    encounter_objects = []
    for i, encounter in enumerate(encounter_entries, start=1):
        x = float(encounter.get("x", 0.0))
        y = float(encounter.get("y", 0.0))
        width = max(1.0, float(encounter.get("w", tile_size)))
        height = max(1.0, float(encounter.get("h", tile_size)))
        table_id = str(encounter.get("table_id", "default_grass")).strip() or "default_grass"
        encounter_objects.append(
            "\n".join([
                f'  <object id="{i}" x="{x:g}" y="{y:g}" width="{width:g}" height="{height:g}">',
                "   <properties>",
                f'    <property name="encounter_table" value="{escape(table_id)}"/>',
                "   </properties>",
                "  </object>",
            ])
        )

    npc_objects = []
    for i, npc in enumerate(npc_events, start=1):
        npc_x = int(npc.get("x", 0)) * tile_size
        npc_y = int(npc.get("y", 0)) * tile_size
        npc_w = int(npc.get("width", 1)) * tile_size
        npc_h = int(npc.get("height", 1)) * tile_size
        npc_name = str(npc.get("id", f"npc_{i}"))

        npc_properties = []
        for field in ("script_id", "facing", "speaker", "dialogue", "sprite", "animation"):
            value = str(npc.get(field, "")).strip()
            if not value:
                continue
            npc_properties.append(f'    <property name="{field}" value="{escape(value)}"/>')

        npc_objects.append(
            "\n".join([
                f'  <object id="{i}" name="{escape(npc_name)}" x="{npc_x}" y="{npc_y}" width="{npc_w}" height="{npc_h}">',
                "   <properties>",
                *npc_properties,
                "   </properties>",
                "  </object>",
            ])
        )

    xml_lines = [
        "<?xml version=\"1.0\" encoding=\"UTF-8\"?>",
        (
            f"<map version=\"1.10\" tiledversion=\"1.11.2\" orientation=\"orthogonal\" "
            f"renderorder=\"right-down\" width=\"{map_width}\" height=\"{map_height}\" "
            f"tilewidth=\"{tile_size}\" tileheight=\"{tile_size}\" infinite=\"0\">"
        ),
        f" <tileset firstgid=\"1\" name=\"{escape(tileset_name)}\" tilewidth=\"{tile_size}\" tileheight=\"{tile_size}\" tilecount=\"{atlas_tile_count}\" columns=\"{atlas_columns}\">",
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
        *spawn_objects,
        " </objectgroup>",
        " <objectgroup id=\"5\" name=\"Warp Layer\">",
        *warp_objects,
        " </objectgroup>",
        " <objectgroup id=\"6\" name=\"NPC Layer\">",
        *npc_objects,
        " </objectgroup>",
        "</map>",
        "",
    ]

    if encounter_objects:
        xml_lines[-2:-2] = [
            " <objectgroup id=\"7\" name=\"Encounter Layer\">",
            *encounter_objects,
            " </objectgroup>",
        ]

    xml = "\n".join(xml_lines)

    tmx_path.write_text(xml, encoding="utf-8")


def write_length_prefixed_string(stream, value: str) -> None:
    encoded = value.encode("utf-8")
    stream.write(struct.pack("<I", len(encoded)))
    stream.write(encoded)


def int_or_default(value: object, default: int = 0) -> int:
    try:
        return int(value)
    except (TypeError, ValueError):
        return default


def write_pcmap(
    output_path: Path,
    map_width: int,
    map_height: int,
    tile_size: int,
    ground_layer_values: list[int],
    cover_layer_values: list[int],
    collision_rects: list[tuple[int, int, int, int]],
    spawn_x: int,
    spawn_y: int,
    warp_events: list[dict],
    map_warp_events: list[dict],
    encounter_entries: list[dict],
    npc_events: list[dict],
) -> None:
    spawn_entries: list[tuple[str, float, float]] = [
        ("default", float(spawn_x), float(spawn_y)),
    ]
    for i, warp in enumerate(map_warp_events):
        warp_x = int_or_default(warp.get("x", 0), 0) * tile_size
        warp_y = int_or_default(warp.get("y", 0), 0) * tile_size
        spawn_entries.append((f"warp_{i}", float(warp_x), float(warp_y)))

    with output_path.open("wb") as stream:
        stream.write(struct.pack("<4sI", b"PCMP", 1))
        stream.write(struct.pack("<IIII", map_width, map_height, tile_size, tile_size))
        stream.write(
            struct.pack(
                "<IIIIIII",
                len(ground_layer_values),
                len(cover_layer_values),
                len(collision_rects),
                len(warp_events),
                len(encounter_entries),
                len(npc_events),
                len(spawn_entries),
            )
        )
        stream.write(struct.pack("<ff", float(spawn_x), float(spawn_y)))

        if ground_layer_values:
            stream.write(struct.pack(f"<{len(ground_layer_values)}i", *ground_layer_values))
        if cover_layer_values:
            stream.write(struct.pack(f"<{len(cover_layer_values)}i", *cover_layer_values))

        for x, y, w, h in collision_rects:
            stream.write(struct.pack("<ffff", float(x), float(y), float(w), float(h)))

        for warp in warp_events:
            x = int_or_default(warp.get("x", 0), 0) * tile_size
            y = int_or_default(warp.get("y", 0), 0) * tile_size
            width = max(1, int_or_default(warp.get("width", 1), 1)) * tile_size
            height = max(1, int_or_default(warp.get("height", 1), 1)) * tile_size
            stream.write(struct.pack("<ffff", float(x), float(y), float(width), float(height)))

            target_map = str(warp.get("dest_map", "")).strip()
            target_spawn_id = str(warp.get("target_spawn_id", "")).strip()
            required_direction = str(warp.get("required_direction", "")).strip().lower()
            write_length_prefixed_string(stream, target_map)
            write_length_prefixed_string(stream, target_spawn_id)
            write_length_prefixed_string(stream, required_direction)

            target_spawn_x = warp.get("target_spawn_x")
            target_spawn_y = warp.get("target_spawn_y")
            has_target_spawn = target_spawn_x is not None and target_spawn_y is not None
            stream.write(struct.pack("<B", 1 if has_target_spawn else 0))
            if has_target_spawn:
                stream.write(struct.pack("<ff", float(target_spawn_x), float(target_spawn_y)))

        for encounter in encounter_entries:
            x = float(encounter.get("x", 0.0))
            y = float(encounter.get("y", 0.0))
            w = float(encounter.get("w", tile_size))
            h = float(encounter.get("h", tile_size))
            table_id = str(encounter.get("table_id", ""))
            stream.write(struct.pack("<ffff", x, y, w, h))
            write_length_prefixed_string(stream, table_id)

        for npc in npc_events:
            npc_x = float(int_or_default(npc.get("x", 0), 0) * tile_size)
            npc_y = float(int_or_default(npc.get("y", 0), 0) * tile_size)
            npc_w = float(max(1, int_or_default(npc.get("width", 1), 1)) * tile_size)
            npc_h = float(max(1, int_or_default(npc.get("height", 1), 1)) * tile_size)
            stream.write(struct.pack("<ffff", npc_x, npc_y, npc_w, npc_h))

            write_length_prefixed_string(stream, str(npc.get("id", "")))
            write_length_prefixed_string(stream, str(npc.get("script_id", "")))
            write_length_prefixed_string(stream, str(npc.get("speaker", "")))
            write_length_prefixed_string(stream, str(npc.get("dialogue", "")))
            write_length_prefixed_string(stream, str(npc.get("facing", "")))
            write_length_prefixed_string(stream, str(npc.get("sprite", "")))
            write_length_prefixed_string(stream, str(npc.get("animation", "")))

        for spawn_id, point_x, point_y in spawn_entries:
            write_length_prefixed_string(stream, spawn_id)
            stream.write(struct.pack("<ff", point_x, point_y))


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
        tall_grass_tiles[index] = behavior in TALL_GRASS_BEHAVIOR_IDS

        if layer_type in (METATILE_LAYER_TYPE_NORMAL, METATILE_LAYER_TYPE_SPLIT):
            cover_layer_values[index] = cover_gid_by_metatile_id.get(metatile_id, 0)

    map_slug = normalize_slug(options.map_slug or to_snake_case(map_json.get("name", map_name)))

    assets_dir = assets_root
    maps_dir = assets_dir / "maps"
    tilesets_dir = assets_dir / "tilesets"
    characters_dir = assets_dir / "characters"
    animations_dir = assets_dir / "animations"
    effects_dir = assets_dir / "effects"
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
    # FireRed warp events are traversable trigger tiles even when collision bits are set.
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

    print(f"Imported FireRed map assets for {map_name}:")
    print(f"- {tmx_output}")
    print(f"- {pcmap_output}")
    print(f"- {tileset_output}")
    print(f"- {map_composite_output}")
    print(f"- {characters_dir / 'red' / 'red_normal.png'}")
    print(f"- {effects_dir / 'tall_grass_rustle.png'}")
    print(f"- {animations_dir / 'red_overworld.xml'}")

    return [
        tmx_output,
        pcmap_output,
        tileset_output,
        map_composite_output,
        characters_dir / "red" / "red_normal.png",
        effects_dir / "tall_grass_rustle.png",
        animations_dir / "red_overworld.xml",
    ]


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
    import_map_assets(parse_options_from_args())


if __name__ == "__main__":
    main()
