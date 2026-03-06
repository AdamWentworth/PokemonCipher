import argparse
import struct
from pathlib import Path

from PIL import Image

TILE_SIZE = 8
BG_TILEMAP_WIDTH = 32
BG_TILEMAP_HEIGHT = 32
VISIBLE_WIDTH = 240
VISIBLE_HEIGHT = 160


def load_tiles(tileset: Image.Image) -> list[Image.Image]:
    columns = tileset.width // TILE_SIZE
    rows = tileset.height // TILE_SIZE
    tiles: list[Image.Image] = []

    for row in range(rows):
        for col in range(columns):
            left = col * TILE_SIZE
            top = row * TILE_SIZE
            tiles.append(tileset.crop((left, top, left + TILE_SIZE, top + TILE_SIZE)))

    return tiles


def render_tilemap_u16(entries: list[int], width: int, height: int, tiles: list[Image.Image]) -> Image.Image:
    output = Image.new("RGBA", (width * TILE_SIZE, height * TILE_SIZE), (0, 0, 0, 0))

    for y in range(height):
        for x in range(width):
            entry = entries[y * width + x]
            tile_index = entry & 0x03FF
            hflip = bool(entry & 0x0400)
            vflip = bool(entry & 0x0800)

            if 0 <= tile_index < len(tiles):
                tile = tiles[tile_index]
            else:
                tile = Image.new("RGBA", (TILE_SIZE, TILE_SIZE), (255, 0, 255, 255))

            if hflip:
                tile = tile.transpose(Image.FLIP_LEFT_RIGHT)
            if vflip:
                tile = tile.transpose(Image.FLIP_TOP_BOTTOM)

            output.paste(tile, (x * TILE_SIZE, y * TILE_SIZE), tile)

    return output


def render_tilemap_u8(entries: list[int], width: int, height: int, tiles: list[Image.Image]) -> Image.Image:
    output = Image.new("RGBA", (width * TILE_SIZE, height * TILE_SIZE), (0, 0, 0, 0))

    for y in range(height):
        for x in range(width):
            tile_index = entries[y * width + x]
            if 0 <= tile_index < len(tiles):
                tile = tiles[tile_index]
            else:
                tile = Image.new("RGBA", (TILE_SIZE, TILE_SIZE), (255, 0, 255, 255))

            output.paste(tile, (x * TILE_SIZE, y * TILE_SIZE), tile)

    return output


def read_u16_file(path: Path) -> list[int]:
    data = path.read_bytes()
    return list(struct.unpack(f"<{len(data) // 2}H", data))


def read_u8_file(path: Path) -> list[int]:
    return list(path.read_bytes())


def copy_png(source: Path, destination: Path) -> None:
    image = Image.open(source).convert("RGBA")
    destination.parent.mkdir(parents=True, exist_ok=True)
    image.save(destination)


def main() -> None:
    parser = argparse.ArgumentParser(description="Import FireRed party menu graphics for PokemonCipher.")
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

    source_dir = firered_root / "graphics" / "party_menu"
    output_dir = assets_root / "ui" / "party_menu"
    output_dir.mkdir(parents=True, exist_ok=True)

    bg_tilesheet = Image.open(source_dir / "bg.png").convert("RGBA")
    bg_tiles = load_tiles(bg_tilesheet)

    bg_entries = read_u16_file(source_dir / "bg.bin")
    expected_bg_entries = BG_TILEMAP_WIDTH * BG_TILEMAP_HEIGHT
    if len(bg_entries) != expected_bg_entries:
        raise ValueError(
            f"Unexpected bg.bin size: expected {expected_bg_entries} tilemap entries, got {len(bg_entries)}."
        )

    full_background = render_tilemap_u16(bg_entries, BG_TILEMAP_WIDTH, BG_TILEMAP_HEIGHT, bg_tiles)
    visible_background = full_background.crop((0, 0, VISIBLE_WIDTH, VISIBLE_HEIGHT))
    visible_background.save(output_dir / "bg_full.png")

    slot_specs = [
        ("slot_main.bin", 10, 7, "slot_main.png"),
        ("slot_main_no_hp.bin", 10, 7, "slot_main_no_hp.png"),
        ("slot_wide.bin", 18, 3, "slot_wide.png"),
        ("slot_wide_no_hp.bin", 18, 3, "slot_wide_no_hp.png"),
        ("slot_wide_empty.bin", 18, 3, "slot_wide_empty.png"),
    ]

    for source_name, width, height, output_name in slot_specs:
        entries = read_u8_file(source_dir / source_name)
        expected_entries = width * height
        if len(entries) != expected_entries:
            raise ValueError(
                f"Unexpected {source_name} size: expected {expected_entries} entries, got {len(entries)}."
            )

        rendered = render_tilemap_u8(entries, width, height, bg_tiles)
        rendered.save(output_dir / output_name)

    button_specs = [
        ("cancel_button.bin", 7, 2, "cancel_button.png"),
        ("confirm_button.bin", 7, 2, "confirm_button.png"),
    ]

    for source_name, width, height, output_name in button_specs:
        entries = read_u16_file(source_dir / source_name)
        expected_entries = width * height
        if len(entries) != expected_entries:
            raise ValueError(
                f"Unexpected {source_name} size: expected {expected_entries} tilemap entries, got {len(entries)}."
            )

        rendered = render_tilemap_u16(entries, width, height, bg_tiles)
        rendered.save(output_dir / output_name)

    copy_png(source_dir / "pokeball_small.png", output_dir / "pokeball_small.png")
    copy_png(source_dir / "pokeball.png", output_dir / "pokeball.png")
    copy_png(source_dir / "hold_icons.png", output_dir / "hold_icons.png")

    print("Imported FireRed party menu graphics:")
    for generated in sorted(output_dir.glob("*.png")):
        print(f"- {generated}")


if __name__ == "__main__":
    main()
