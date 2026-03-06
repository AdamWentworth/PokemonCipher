from __future__ import annotations

from dataclasses import dataclass
from pathlib import Path

from PIL import Image

from .binary import read_u16_values
from .context import ImportContext
from .image_utils import (
    apply_color_key,
    copy_font_rgba,
    copy_indexed_with_transparent_index,
    copy_rgba,
    load_tileset_tiles,
    render_tilemap_u16,
)
from .palette_utils import (
    colorize_indexed_image_with_palette,
    parse_jasc_palette,
    recolor_image_by_palette_mapping,
)


@dataclass(frozen=True)
class SummaryImportOptions:
    include_pages: bool = True
    include_shared_ui: bool = True
    include_fonts: bool = True
    include_eevee: bool = True
    include_eevee_shiny: bool = True


def import_summary_assets(context: ImportContext, options: SummaryImportOptions = SummaryImportOptions()) -> list[Path]:
    firered_root = context.firered_root
    assets_root = context.assets_root

    summary_source = firered_root / "graphics" / "summary_screen"
    eevee_source = firered_root / "graphics" / "pokemon" / "eevee"

    summary_output = assets_root / "ui" / "summary_screen"
    eevee_output = assets_root / "pokemon" / "eevee"
    fonts_output = assets_root / "fonts"

    generated: list[Path] = []

    bg_tileset = Image.open(summary_source / "bg.png").convert("RGBA")
    bg_tiles = load_tileset_tiles(bg_tileset, tile_size=8)

    if options.include_pages:
        summary_output.mkdir(parents=True, exist_ok=True)
        tilemap_specs = [
            ("page_info.bin", 32, 20, "page_info.png"),
            ("page_skills.bin", 32, 20, "page_skills.png"),
            ("page_egg.bin", 32, 20, "page_egg.png"),
            ("page_moves_info.bin", 32, 20, "page_moves_info.png"),
            ("page_moves.bin", 32, 32, "page_moves.png"),
        ]

        for source_name, width, height, output_name in tilemap_specs:
            entries = read_u16_values(summary_source / source_name)
            rendered = render_tilemap_u16(entries, width, height, bg_tiles, tile_size=8)
            viewport_crop = apply_color_key(rendered.crop((0, 0, 240, 160)), (255, 0, 255))
            output_path = summary_output / output_name
            viewport_crop.save(output_path)
            generated.append(output_path)

        base_specs = [
            ("moves_info_page.bin", 32, 20, "base_info_skills.png"),
            ("moves_page.bin", 32, 32, "base_moves.png"),
        ]
        for source_name, width, height, output_name in base_specs:
            entries = read_u16_values(summary_source / source_name)
            rendered = render_tilemap_u16(entries, width, height, bg_tiles, tile_size=8)
            viewport_crop = apply_color_key(rendered.crop((0, 0, 240, 160)), (255, 0, 255))
            output_path = summary_output / output_name
            viewport_crop.save(output_path)
            generated.append(output_path)

    if options.include_shared_ui:
        summary_output.mkdir(parents=True, exist_ok=True)
        shared_ui_specs = [
            (summary_source / "bg.png", summary_output / "bg_tiles.png"),
            (summary_source / "status_ailment_icons.png", summary_output / "status_ailment_icons.png"),
            (summary_source / "pokerus_cured.png", summary_output / "pokerus_cured.png"),
            (summary_source / "shiny_star.png", summary_output / "shiny_star.png"),
            (summary_source / "move_selection_cursor_left.png", summary_output / "move_selection_cursor_left.png"),
            (summary_source / "move_selection_cursor_right.png", summary_output / "move_selection_cursor_right.png"),
        ]
        for source, destination in shared_ui_specs:
            copy_rgba(source, destination)
            generated.append(destination)

        copy_indexed_with_transparent_index(
            summary_source / "hp_bar.png",
            summary_output / "hp_bar.png",
            transparent_index=0,
        )
        generated.append(summary_output / "hp_bar.png")

        copy_indexed_with_transparent_index(
            summary_source / "exp_bar.png",
            summary_output / "exp_bar.png",
            transparent_index=0,
        )
        generated.append(summary_output / "exp_bar.png")

        copy_indexed_with_transparent_index(
            firered_root / "graphics" / "interface" / "menu_info.png",
            summary_output / "menu_info.png",
            transparent_index=0,
        )
        generated.append(summary_output / "menu_info.png")

    if options.include_fonts:
        fonts_output.mkdir(parents=True, exist_ok=True)
        copy_font_rgba(
            firered_root / "graphics" / "fonts" / "latin_normal.png",
            fonts_output / "latin_normal.png",
        )
        generated.append(fonts_output / "latin_normal.png")

        copy_font_rgba(
            firered_root / "graphics" / "fonts" / "latin_small.png",
            fonts_output / "latin_small.png",
        )
        generated.append(fonts_output / "latin_small.png")

        copy_indexed_with_transparent_index(
            firered_root / "graphics" / "fonts" / "keypad_icons.png",
            fonts_output / "keypad_icons.png",
            transparent_index=0,
        )
        generated.append(fonts_output / "keypad_icons.png")

    if options.include_eevee:
        eevee_output.mkdir(parents=True, exist_ok=True)
        eevee_specs = [
            (eevee_source / "front.png", eevee_output / "front.png"),
            (eevee_source / "icon.png", eevee_output / "icon.png"),
            (eevee_source / "back.png", eevee_output / "back.png"),
        ]
        for source, destination in eevee_specs:
            copy_rgba(source, destination)
            generated.append(destination)

    if options.include_eevee_shiny:
        eevee_output.mkdir(parents=True, exist_ok=True)
        shiny_palette = parse_jasc_palette(eevee_source / "shiny.pal")
        colorize_indexed_image_with_palette(
            eevee_source / "front.png",
            eevee_output / "shiny_front.png",
            shiny_palette,
        )
        generated.append(eevee_output / "shiny_front.png")

        colorize_indexed_image_with_palette(
            eevee_source / "back.png",
            eevee_output / "shiny_back.png",
            shiny_palette,
        )
        generated.append(eevee_output / "shiny_back.png")

        recolor_image_by_palette_mapping(
            eevee_source / "icon.png",
            eevee_output / "shiny_icon.png",
            parse_jasc_palette(eevee_source / "normal.pal"),
            shiny_palette,
            [1, 2, 3, 4, 5, 10, 11, 12, 13, 14],
        )
        generated.append(eevee_output / "shiny_icon.png")

    return sorted(set(generated))

