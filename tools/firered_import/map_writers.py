from __future__ import annotations

import math
import struct
from pathlib import Path
from xml.sax.saxutils import escape


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

    tmx_path.write_text("\n".join(xml_lines), encoding="utf-8")


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

