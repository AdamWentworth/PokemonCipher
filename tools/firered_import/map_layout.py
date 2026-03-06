from __future__ import annotations

import re
from pathlib import Path
from xml.etree import ElementTree

from .map_graphics import METATILE_ATTRIBUTE_BEHAVIOR_MASK, get_metatile_attributes

STAIR_WARP_RIGHT_BEHAVIOR_IDS = {
    0x6C,
    0x6E,
}

STAIR_WARP_LEFT_BEHAVIOR_IDS = {
    0x6D,
    0x6F,
}


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

        target_metrics = None
        for slug_candidate in map_constant_slug_candidates(target_map):
            candidate_tmx = maps_dir / slug_candidate / f"{slug_candidate}_map.tmx"
            target_metrics = read_tmx_metrics(candidate_tmx)
            if target_metrics is not None:
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

        warps.append(
            {
                "x": x,
                "y": y,
                "width": width,
                "height": height,
                "dest_map": target_map,
                "target_spawn_x": target_x_tile * target_tile_width,
                "target_spawn_y": target_y_tile * target_tile_height,
            }
        )

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

