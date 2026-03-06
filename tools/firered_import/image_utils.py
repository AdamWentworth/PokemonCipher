from __future__ import annotations

from pathlib import Path

from PIL import Image


def load_tileset_tiles(tileset: Image.Image, tile_size: int = 8) -> list[Image.Image]:
    columns = tileset.width // tile_size
    rows = tileset.height // tile_size
    tiles: list[Image.Image] = []

    for row in range(rows):
        for col in range(columns):
            left = col * tile_size
            top = row * tile_size
            tiles.append(tileset.crop((left, top, left + tile_size, top + tile_size)))

    return tiles


def render_tilemap_u16(
    entries: list[int],
    width: int,
    height: int,
    tiles: list[Image.Image],
    tile_size: int = 8,
) -> Image.Image:
    if len(entries) != width * height:
        raise ValueError(f"Tilemap entry count mismatch: expected {width * height}, got {len(entries)}")

    output = Image.new("RGBA", (width * tile_size, height * tile_size), (0, 0, 0, 0))

    for y in range(height):
        for x in range(width):
            entry = entries[y * width + x]
            tile_index = entry & 0x03FF
            hflip = bool(entry & 0x0400)
            vflip = bool(entry & 0x0800)

            if 0 <= tile_index < len(tiles):
                tile = tiles[tile_index]
            else:
                tile = Image.new("RGBA", (tile_size, tile_size), (255, 0, 255, 255))

            if hflip:
                tile = tile.transpose(Image.FLIP_LEFT_RIGHT)
            if vflip:
                tile = tile.transpose(Image.FLIP_TOP_BOTTOM)

            output.paste(tile, (x * tile_size, y * tile_size), tile)

    return output


def render_tilemap_u8(
    entries: list[int],
    width: int,
    height: int,
    tiles: list[Image.Image],
    tile_size: int = 8,
) -> Image.Image:
    if len(entries) != width * height:
        raise ValueError(f"Tilemap entry count mismatch: expected {width * height}, got {len(entries)}")

    output = Image.new("RGBA", (width * tile_size, height * tile_size), (0, 0, 0, 0))
    for y in range(height):
        for x in range(width):
            tile_index = entries[y * width + x]
            if 0 <= tile_index < len(tiles):
                tile = tiles[tile_index]
            else:
                tile = Image.new("RGBA", (tile_size, tile_size), (255, 0, 255, 255))

            output.paste(tile, (x * tile_size, y * tile_size), tile)

    return output


def apply_color_key(image: Image.Image, color_key: tuple[int, int, int]) -> Image.Image:
    rgba = image.convert("RGBA")
    pixels: list[tuple[int, int, int, int]] = []
    for r, g, b, a in rgba.getdata():
        if a > 0 and (r, g, b) == color_key:
            pixels.append((0, 0, 0, 0))
        else:
            pixels.append((r, g, b, a))
    rgba.putdata(pixels)
    return rgba


def copy_rgba(source: Path, destination: Path) -> None:
    image = Image.open(source).convert("RGBA")
    destination.parent.mkdir(parents=True, exist_ok=True)
    image.save(destination)


def copy_rgba_with_color_key(source: Path, destination: Path, color_key: tuple[int, int, int]) -> None:
    keyed = apply_color_key(Image.open(source).convert("RGBA"), color_key)
    destination.parent.mkdir(parents=True, exist_ok=True)
    keyed.save(destination)


def copy_indexed_with_transparent_index(source: Path, destination: Path, transparent_index: int = 0) -> None:
    indexed = Image.open(source)
    if indexed.mode != "P":
        indexed = indexed.convert("P")

    palette = indexed.getpalette()
    if palette is None:
        raise ValueError(f"Indexed image does not contain a palette: {source}")

    rgba = Image.new("RGBA", indexed.size, (0, 0, 0, 0))
    pixels: list[tuple[int, int, int, int]] = []
    for index in indexed.getdata():
        if index == transparent_index:
            pixels.append((0, 0, 0, 0))
            continue

        offset = index * 3
        r, g, b = palette[offset:offset + 3]
        pixels.append((r, g, b, 255))

    rgba.putdata(pixels)
    destination.parent.mkdir(parents=True, exist_ok=True)
    rgba.save(destination)


def copy_font_rgba(source: Path, destination: Path) -> None:
    transparent_colors = {
        (144, 200, 255),
        (255, 255, 255),
    }

    image = Image.open(source).convert("RGBA")
    pixels: list[tuple[int, int, int, int]] = []
    for r, g, b, a in image.getdata():
        if a > 0 and (r, g, b) in transparent_colors:
            pixels.append((0, 0, 0, 0))
        else:
            pixels.append((r, g, b, a))

    image.putdata(pixels)
    destination.parent.mkdir(parents=True, exist_ok=True)
    image.save(destination)

