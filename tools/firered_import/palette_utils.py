from __future__ import annotations

from pathlib import Path

from PIL import Image


def parse_jasc_palette(path: Path) -> list[tuple[int, int, int]]:
    lines = path.read_text(encoding="utf-8").splitlines()
    if len(lines) < 19 or lines[0].strip() != "JASC-PAL":
        raise ValueError(f"Unexpected palette format: {path}")

    colors: list[tuple[int, int, int]] = []
    for line in lines[3:19]:
        r, g, b = (int(component) for component in line.split())
        colors.append((r, g, b))
    return colors


def colorize_indexed_image_with_palette(
    source: Path,
    destination: Path,
    palette: list[tuple[int, int, int]],
) -> None:
    indexed = Image.open(source).convert("P")
    rgba = Image.new("RGBA", indexed.size, (0, 0, 0, 0))

    pixels: list[tuple[int, int, int, int]] = []
    for index in indexed.getdata():
        if index == 0:
            pixels.append((0, 0, 0, 0))
            continue

        clamped_index = min(max(index, 0), len(palette) - 1)
        r, g, b = palette[clamped_index]
        pixels.append((r, g, b, 255))

    rgba.putdata(pixels)
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

