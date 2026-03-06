from __future__ import annotations

from dataclasses import dataclass
from pathlib import Path

from PIL import Image

from .binary import read_u16_values, read_u8_values
from .context import ImportContext
from .image_utils import copy_rgba, load_tileset_tiles, render_tilemap_u16, render_tilemap_u8

TILE_SIZE = 8
BG_TILEMAP_WIDTH = 32
BG_TILEMAP_HEIGHT = 32
VISIBLE_WIDTH = 240
VISIBLE_HEIGHT = 160


@dataclass(frozen=True)
class PartyMenuImportOptions:
    include_background: bool = True
    include_slots: bool = True
    include_buttons: bool = True
    include_icons: bool = True


def import_party_menu_assets(
    context: ImportContext,
    options: PartyMenuImportOptions = PartyMenuImportOptions(),
) -> list[Path]:
    firered_root = context.firered_root
    assets_root = context.assets_root

    source_dir = firered_root / "graphics" / "party_menu"
    output_dir = assets_root / "ui" / "party_menu"
    output_dir.mkdir(parents=True, exist_ok=True)

    generated: list[Path] = []

    bg_tilesheet = Image.open(source_dir / "bg.png").convert("RGBA")
    bg_tiles = load_tileset_tiles(bg_tilesheet, tile_size=TILE_SIZE)

    if options.include_background:
        bg_entries = read_u16_values(source_dir / "bg.bin")
        expected_bg_entries = BG_TILEMAP_WIDTH * BG_TILEMAP_HEIGHT
        if len(bg_entries) != expected_bg_entries:
            raise ValueError(
                f"Unexpected bg.bin size: expected {expected_bg_entries} tilemap entries, got {len(bg_entries)}."
            )

        full_background = render_tilemap_u16(bg_entries, BG_TILEMAP_WIDTH, BG_TILEMAP_HEIGHT, bg_tiles, tile_size=TILE_SIZE)
        visible_background = full_background.crop((0, 0, VISIBLE_WIDTH, VISIBLE_HEIGHT))
        output_path = output_dir / "bg_full.png"
        visible_background.save(output_path)
        generated.append(output_path)

    if options.include_slots:
        slot_specs = [
            ("slot_main.bin", 10, 7, "slot_main.png"),
            ("slot_main_no_hp.bin", 10, 7, "slot_main_no_hp.png"),
            ("slot_wide.bin", 18, 3, "slot_wide.png"),
            ("slot_wide_no_hp.bin", 18, 3, "slot_wide_no_hp.png"),
            ("slot_wide_empty.bin", 18, 3, "slot_wide_empty.png"),
        ]

        for source_name, width, height, output_name in slot_specs:
            entries = read_u8_values(source_dir / source_name)
            rendered = render_tilemap_u8(entries, width, height, bg_tiles, tile_size=TILE_SIZE)
            output_path = output_dir / output_name
            rendered.save(output_path)
            generated.append(output_path)

    if options.include_buttons:
        button_specs = [
            ("cancel_button.bin", 7, 2, "cancel_button.png"),
            ("confirm_button.bin", 7, 2, "confirm_button.png"),
        ]

        for source_name, width, height, output_name in button_specs:
            entries = read_u16_values(source_dir / source_name)
            rendered = render_tilemap_u16(entries, width, height, bg_tiles, tile_size=TILE_SIZE)
            output_path = output_dir / output_name
            rendered.save(output_path)
            generated.append(output_path)

    if options.include_icons:
        icon_specs = [
            ("pokeball_small.png", "pokeball_small.png"),
            ("pokeball.png", "pokeball.png"),
            ("hold_icons.png", "hold_icons.png"),
        ]
        for source_name, output_name in icon_specs:
            destination = output_dir / output_name
            copy_rgba(source_dir / source_name, destination)
            generated.append(destination)

    return sorted(set(generated))

