import argparse
import struct
from pathlib import Path

from PIL import Image

TILE_SIZE = 8


def load_tileset_tiles(tileset: Image.Image) -> list[Image.Image]:
    columns = tileset.width // TILE_SIZE
    rows = tileset.height // TILE_SIZE
    tiles: list[Image.Image] = []

    for row in range(rows):
        for col in range(columns):
            left = col * TILE_SIZE
            top = row * TILE_SIZE
            tiles.append(tileset.crop((left, top, left + TILE_SIZE, top + TILE_SIZE)))

    return tiles


def read_u16_values(path: Path) -> list[int]:
    data = path.read_bytes()
    if len(data) % 2 != 0:
        raise ValueError(f"Unexpected odd-sized tilemap: {path}")
    return list(struct.unpack(f"<{len(data) // 2}H", data))


def render_tilemap(entries: list[int], width: int, height: int, tiles: list[Image.Image]) -> Image.Image:
    if len(entries) != width * height:
        raise ValueError(f"Tilemap entry count mismatch: expected {width * height}, got {len(entries)}")

    output = Image.new("RGBA", (width * TILE_SIZE, height * TILE_SIZE), (0, 0, 0, 0))

    for y in range(height):
        for x in range(width):
            entry = entries[y * width + x]
            tile_index = entry & 0x03FF
            hflip = bool(entry & 0x0400)
            vflip = bool(entry & 0x0800)

            if tile_index < 0 or tile_index >= len(tiles):
                tile = Image.new("RGBA", (TILE_SIZE, TILE_SIZE), (255, 0, 255, 255))
            else:
                tile = tiles[tile_index]

            if hflip:
                tile = tile.transpose(Image.FLIP_LEFT_RIGHT)
            if vflip:
                tile = tile.transpose(Image.FLIP_TOP_BOTTOM)

            output.paste(tile, (x * TILE_SIZE, y * TILE_SIZE), tile)

    return output


def apply_magenta_chroma_key(image: Image.Image) -> Image.Image:
    rgba = image.convert("RGBA")
    keyed = []
    for r, g, b, a in rgba.getdata():
        if a > 0 and (r, g, b) == (255, 0, 255):
            keyed.append((0, 0, 0, 0))
        else:
            keyed.append((r, g, b, a))
    rgba.putdata(keyed)
    return rgba


def copy_rgba(source: Path, destination: Path) -> None:
    image = Image.open(source).convert("RGBA")
    destination.parent.mkdir(parents=True, exist_ok=True)
    image.save(destination)


def copy_rgba_with_color_key(source: Path, destination: Path, color_key: tuple[int, int, int]) -> None:
    image = Image.open(source).convert("RGBA")
    output_pixels = []
    for r, g, b, a in image.getdata():
        if a > 0 and (r, g, b) == color_key:
            output_pixels.append((0, 0, 0, 0))
        else:
            output_pixels.append((r, g, b, a))

    image.putdata(output_pixels)
    destination.parent.mkdir(parents=True, exist_ok=True)
    image.save(destination)


def copy_indexed_with_transparent_index(source: Path, destination: Path, transparent_index: int = 0) -> None:
    indexed = Image.open(source)
    if indexed.mode != "P":
        indexed = indexed.convert("P")

    palette = indexed.getpalette()
    if palette is None:
        raise ValueError(f"Indexed image does not contain a palette: {source}")

    rgba = Image.new("RGBA", indexed.size, (0, 0, 0, 0))
    output_pixels = []
    for index in indexed.getdata():
        if index == transparent_index:
            output_pixels.append((0, 0, 0, 0))
            continue

        offset = index * 3
        r, g, b = palette[offset:offset + 3]
        output_pixels.append((r, g, b, 255))

    rgba.putdata(output_pixels)
    destination.parent.mkdir(parents=True, exist_ok=True)
    rgba.save(destination)


def copy_font_rgba(source: Path, destination: Path) -> None:
    # FireRed font PNG export uses blue as tile background and white as glyph cell fill.
    # Keep only shading/outline colors so glyphs composite cleanly over UI backgrounds.
    transparent_colors = {
        (144, 200, 255),  # export background color
        (255, 255, 255),  # glyph cell fill that should not render as a box
    }

    image = Image.open(source).convert("RGBA")
    output_pixels = []
    for r, g, b, a in image.getdata():
        if a > 0 and (r, g, b) in transparent_colors:
            output_pixels.append((0, 0, 0, 0))
        else:
            output_pixels.append((r, g, b, a))

    image.putdata(output_pixels)
    destination.parent.mkdir(parents=True, exist_ok=True)
    image.save(destination)


def parse_jasc_palette(path: Path) -> list[tuple[int, int, int]]:
    lines = path.read_text(encoding="utf-8").splitlines()
    if len(lines) < 19 or lines[0].strip() != "JASC-PAL":
        raise ValueError(f"Unexpected palette format: {path}")

    colors: list[tuple[int, int, int]] = []
    for line in lines[3:19]:
        r, g, b = (int(component) for component in line.split())
        colors.append((r, g, b))
    return colors


def colorize_indexed_image_with_palette(source: Path, destination: Path, palette: list[tuple[int, int, int]]) -> None:
    indexed = Image.open(source).convert("P")
    rgba = Image.new("RGBA", indexed.size, (0, 0, 0, 0))

    output_pixels = []
    for index in indexed.getdata():
        if index == 0:
            output_pixels.append((0, 0, 0, 0))
            continue

        clamped_index = min(max(index, 0), len(palette) - 1)
        r, g, b = palette[clamped_index]
        output_pixels.append((r, g, b, 255))

    rgba.putdata(output_pixels)
    destination.parent.mkdir(parents=True, exist_ok=True)
    rgba.save(destination)


def recolor_image_by_palette_mapping(
    source: Path,
    destination: Path,
    source_palette: list[tuple[int, int, int]],
    target_palette: list[tuple[int, int, int]],
    allowed_indices: list[int],
) -> None:
    image = Image.open(source).convert("RGBA")
    output = Image.new("RGBA", image.size, (0, 0, 0, 0))

    allowed = [index for index in allowed_indices if 0 <= index < len(source_palette) and 0 <= index < len(target_palette)]
    if not allowed:
        raise ValueError("No valid palette indices provided for recolor_image_by_palette_mapping.")

    source_colors = [source_palette[index] for index in allowed]
    target_colors = [target_palette[index] for index in allowed]

    mapped_pixels: list[tuple[int, int, int, int]] = []
    for r, g, b, a in image.getdata():
        if a == 0:
            mapped_pixels.append((0, 0, 0, 0))
            continue

        best_index = 0
        best_distance = float("inf")
        for i, (sr, sg, sb) in enumerate(source_colors):
            dr = float(r - sr)
            dg = float(g - sg)
            db = float(b - sb)
            distance = (dr * dr) + (dg * dg) + (db * db)
            if distance < best_distance:
                best_distance = distance
                best_index = i

        tr, tg, tb = target_colors[best_index]
        mapped_pixels.append((tr, tg, tb, 255))

    output.putdata(mapped_pixels)
    destination.parent.mkdir(parents=True, exist_ok=True)
    output.save(destination)


def main() -> None:
    parser = argparse.ArgumentParser(description="Import FireRed summary screen and Eevee assets for PokemonCipher.")
    parser.add_argument("--firered-root", type=Path, default=Path(r"C:\Code\pokefirered"))
    parser.add_argument("--project-root", type=Path, default=Path(__file__).resolve().parents[1])
    parser.add_argument(
        "--assets-root",
        type=Path,
        default=None,
        help="Output assets root (defaults to <project-root>/assets).",
    )
    args = parser.parse_args()

    firered_root = args.firered_root
    project_root = args.project_root
    assets_root = args.assets_root if args.assets_root is not None else (project_root / "assets")

    summary_source = firered_root / "graphics" / "summary_screen"
    eevee_source = firered_root / "graphics" / "pokemon" / "eevee"

    summary_output = assets_root / "ui" / "summary_screen"
    eevee_output = assets_root / "pokemon" / "eevee"
    fonts_output = assets_root / "fonts"
    summary_output.mkdir(parents=True, exist_ok=True)
    eevee_output.mkdir(parents=True, exist_ok=True)
    fonts_output.mkdir(parents=True, exist_ok=True)

    bg_tileset = Image.open(summary_source / "bg.png").convert("RGBA")
    bg_tiles = load_tileset_tiles(bg_tileset)

    tilemap_specs = [
        ("page_info.bin", 32, 20, "page_info.png"),
        ("page_skills.bin", 32, 20, "page_skills.png"),
        ("page_egg.bin", 32, 20, "page_egg.png"),
        ("page_moves_info.bin", 32, 20, "page_moves_info.png"),
        ("page_moves.bin", 32, 32, "page_moves.png"),
    ]

    for source_name, width, height, output_name in tilemap_specs:
        entries = read_u16_values(summary_source / source_name)
        rendered = render_tilemap(entries, width, height, bg_tiles)
        viewport_crop = apply_magenta_chroma_key(rendered.crop((0, 0, 240, 160)))
        viewport_crop.save(summary_output / output_name)

    base_specs = [
        ("moves_info_page.bin", 32, 20, "base_info_skills.png"),
        ("moves_page.bin", 32, 32, "base_moves.png"),
    ]
    for source_name, width, height, output_name in base_specs:
        entries = read_u16_values(summary_source / source_name)
        rendered = render_tilemap(entries, width, height, bg_tiles)
        viewport_crop = apply_magenta_chroma_key(rendered.crop((0, 0, 240, 160)))
        viewport_crop.save(summary_output / output_name)

    copy_specs = [
        (summary_source / "bg.png", summary_output / "bg_tiles.png"),
        (summary_source / "status_ailment_icons.png", summary_output / "status_ailment_icons.png"),
        (summary_source / "pokerus_cured.png", summary_output / "pokerus_cured.png"),
        (summary_source / "shiny_star.png", summary_output / "shiny_star.png"),
        (summary_source / "move_selection_cursor_left.png", summary_output / "move_selection_cursor_left.png"),
        (summary_source / "move_selection_cursor_right.png", summary_output / "move_selection_cursor_right.png"),
        (eevee_source / "front.png", eevee_output / "front.png"),
        (eevee_source / "icon.png", eevee_output / "icon.png"),
        (eevee_source / "back.png", eevee_output / "back.png"),
    ]

    for source, destination in copy_specs:
        copy_rgba(source, destination)

    # Keep index-0 transparent for sheet-style UI assets so no opaque backing appears.
    copy_indexed_with_transparent_index(
        summary_source / "hp_bar.png",
        summary_output / "hp_bar.png",
        transparent_index=0,
    )
    copy_indexed_with_transparent_index(
        summary_source / "exp_bar.png",
        summary_output / "exp_bar.png",
        transparent_index=0,
    )
    copy_indexed_with_transparent_index(
        firered_root / "graphics" / "interface" / "menu_info.png",
        summary_output / "menu_info.png",
        transparent_index=0,
    )

    copy_font_rgba(
        firered_root / "graphics" / "fonts" / "latin_normal.png",
        fonts_output / "latin_normal.png",
    )
    copy_font_rgba(
        firered_root / "graphics" / "fonts" / "latin_small.png",
        fonts_output / "latin_small.png",
    )
    copy_indexed_with_transparent_index(
        firered_root / "graphics" / "fonts" / "keypad_icons.png",
        fonts_output / "keypad_icons.png",
        transparent_index=0,
    )

    shiny_palette = parse_jasc_palette(eevee_source / "shiny.pal")
    colorize_indexed_image_with_palette(eevee_source / "front.png", eevee_output / "shiny_front.png", shiny_palette)
    colorize_indexed_image_with_palette(eevee_source / "back.png", eevee_output / "shiny_back.png", shiny_palette)
    # Icon graphics use separate icon palettes, so remap by nearest species-normal sprite color
    # and then substitute with the corresponding species-shiny sprite color.
    recolor_image_by_palette_mapping(
        eevee_source / "icon.png",
        eevee_output / "shiny_icon.png",
        parse_jasc_palette(eevee_source / "normal.pal"),
        shiny_palette,
        # Keep only meaningful non-magenta species palette slots.
        [1, 2, 3, 4, 5, 10, 11, 12, 13, 14],
    )

    print("Imported FireRed summary assets:")
    for path in sorted(summary_output.glob("*.png")):
        print(f"- {path}")
    print("Imported Eevee assets:")
    for path in sorted(eevee_output.glob("*.png")):
        print(f"- {path}")
    print("Imported font assets:")
    for path in sorted(fonts_output.glob("*.png")):
        print(f"- {path}")


if __name__ == "__main__":
    main()
