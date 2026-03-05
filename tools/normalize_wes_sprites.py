import argparse
from pathlib import Path

from PIL import Image

COMPAT_FRAME_WIDTH = 16
COMPAT_FRAME_HEIGHT = 32

# Matches the clip ordering expected by assets/animations/wes_overworld.xml.
FRAME_ORDER = [
    (0, 0),  # idle_down
    (3, 0),  # idle_up
    (1, 0),  # idle_left
    (0, 1),  # walk_down_a
    (0, 3),  # walk_down_b
    (3, 1),  # walk_up_a
    (3, 3),  # walk_up_b
    (1, 1),  # walk_left_a
    (1, 3),  # walk_left_b
]

# Standardized source names and resulting normalized strip names.
SHEETS = [
    ("wes_run_source.png", "wes_overworld.png"),
    ("wes_bike_source.png", "wes_bike_overworld.png"),
    ("wes_surf_source.png", "wes_surf_overworld.png"),
    ("wes_fish_surf_source.png", "wes_fish_surf_overworld.png"),
    ("wes_dive_source.png", "wes_dive_overworld.png"),
    ("wes_fish_source_a.png", "wes_fish_overworld_a.png"),
    ("wes_fish_source_b.png", "wes_fish_overworld_b.png"),
    ("wes_trainer_source.png", "wes_trainer_overworld.png"),
    ("wes_headache_source.png", "wes_headache_overworld.png"),
    ("wes_knocked_out_source.png", "wes_knocked_out_overworld.png"),
]
ACTIVE_RUNTIME_STRIP = "wes_overworld.png"


def extract_robust_trimmed_cell(cell: Image.Image) -> Image.Image:
    bbox = cell.getbbox()
    if not bbox:
        return cell

    cropped = cell.crop(bbox)
    alpha = cropped.getchannel("A")

    left = 0
    right = cropped.width - 1
    top = 0
    bottom = cropped.height - 1

    # Ignore tiny edge protrusions (for example, offset guide fragments) so they
    # do not force an overly small scale for the whole sheet.
    edge_threshold = 2

    while left <= right:
        non_zero = 0
        for y in range(top, bottom + 1):
            if alpha.getpixel((left, y)) > 0:
                non_zero += 1
        if non_zero > edge_threshold:
            break
        left += 1

    while right >= left:
        non_zero = 0
        for y in range(top, bottom + 1):
            if alpha.getpixel((right, y)) > 0:
                non_zero += 1
        if non_zero > edge_threshold:
            break
        right -= 1

    while top <= bottom:
        non_zero = 0
        for x in range(left, right + 1):
            if alpha.getpixel((x, top)) > 0:
                non_zero += 1
        if non_zero > edge_threshold:
            break
        top += 1

    while bottom >= top:
        non_zero = 0
        for x in range(left, right + 1):
            if alpha.getpixel((x, bottom)) > 0:
                non_zero += 1
        if non_zero > edge_threshold:
            break
        bottom -= 1

    if left > right or top > bottom:
        return cropped

    return cropped.crop((left, top, right + 1, bottom + 1))


def build_hq_strip(
    source_path: Path,
    *,
    frame_width: int,
    frame_height: int,
    allow_upscale: bool,
) -> Image.Image:
    source = Image.open(source_path).convert("RGBA")
    rows = 4
    cols = 4
    cell_width = source.width // cols
    cell_height = source.height // rows

    trimmed_frames: list[Image.Image] = []
    max_trim_width = 1
    max_trim_height = 1

    for row, col in FRAME_ORDER:
        cell = source.crop((
            col * cell_width,
            row * cell_height,
            (col + 1) * cell_width,
            (row + 1) * cell_height,
        ))
        trimmed = extract_robust_trimmed_cell(cell)
        trimmed_frames.append(trimmed)
        max_trim_width = max(max_trim_width, trimmed.width)
        max_trim_height = max(max_trim_height, trimmed.height)

    # One scale per sheet: avoids frame-to-frame "wobble" and distorted details.
    scale = min(
        frame_width / max_trim_width,
        (frame_height - 1) / max_trim_height,
    )
    if not allow_upscale:
        scale = min(scale, 1.0)

    strip_width = frame_width * len(FRAME_ORDER)
    output = Image.new("RGBA", (strip_width, frame_height), (0, 0, 0, 0))

    for frame_index, trimmed in enumerate(trimmed_frames):
        target_width = max(1, int(round(trimmed.width * scale)))
        target_height = max(1, int(round(trimmed.height * scale)))
        resized = trimmed.resize((target_width, target_height), Image.NEAREST)

        dest_x = frame_index * frame_width + ((frame_width - target_width) // 2)
        # Keep one transparent pixel at the bottom to avoid edge clipping artifacts.
        dest_y = (frame_height - 1) - target_height
        output.paste(resized, (dest_x, dest_y), resized)

    return output


def build_hq_strip_native(source_path: Path) -> tuple[Image.Image, int, int]:
    source = Image.open(source_path).convert("RGBA")
    rows = 4
    cols = 4
    cell_width = source.width // cols
    cell_height = source.height // rows

    trimmed_frames: list[Image.Image] = []
    max_trim_width = 1
    max_trim_height = 1
    for row, col in FRAME_ORDER:
        cell = source.crop((
            col * cell_width,
            row * cell_height,
            (col + 1) * cell_width,
            (row + 1) * cell_height,
        ))
        trimmed = extract_robust_trimmed_cell(cell)
        trimmed_frames.append(trimmed)
        max_trim_width = max(max_trim_width, trimmed.width)
        max_trim_height = max(max_trim_height, trimmed.height)

    frame_width = max_trim_width
    frame_height = max_trim_height + 1
    output = Image.new(
        "RGBA",
        (frame_width * len(FRAME_ORDER), frame_height),
        (0, 0, 0, 0),
    )

    for frame_index, trimmed in enumerate(trimmed_frames):
        dest_x = frame_index * frame_width + ((frame_width - trimmed.width) // 2)
        dest_y = (frame_height - 1) - trimmed.height
        output.paste(trimmed, (dest_x, dest_y), trimmed)

    return output, frame_width, frame_height


def write_sheet_pair(
    source_path: Path,
    compat_output_path: Path,
    hq_output_path: Path,
    *,
    allow_upscale: bool,
) -> None:
    # Runtime-compatible strip (fixed 16x32).
    compat_strip = build_hq_strip(
        source_path,
        frame_width=COMPAT_FRAME_WIDTH,
        frame_height=COMPAT_FRAME_HEIGHT,
        allow_upscale=allow_upscale,
    )
    # High-quality strip (source-native, no scaling).
    hq_strip, _, _ = build_hq_strip_native(source_path)

    hq_output_path.parent.mkdir(parents=True, exist_ok=True)
    compat_output_path.parent.mkdir(parents=True, exist_ok=True)
    hq_strip.save(hq_output_path)
    compat_strip.save(compat_output_path)


def main() -> None:
    parser = argparse.ArgumentParser(
        description="Normalize Wes sprite sheets into 16x32 overworld strips."
    )
    parser.add_argument(
        "--project-root",
        type=Path,
        default=Path(__file__).resolve().parents[1],
        help="PokemonCipher project root.",
    )
    parser.add_argument(
        "--all-compat",
        action="store_true",
        help="Generate compat 16x32 strips for all variants (default keeps only active wes_overworld).",
    )
    args = parser.parse_args()

    project_root = args.project_root
    source_root = project_root / "assets" / "characters" / "wes" / "source"
    output_compat_root = project_root / "assets" / "characters" / "wes" / "overworld"
    output_hq_root = project_root / "assets" / "characters" / "wes" / "overworld_hq"

    generated_compat: list[Path] = []
    generated_hq: list[Path] = []
    for source_name, output_name in SHEETS:
        source_path = source_root / source_name
        if not source_path.exists():
            continue

        compat_output_path = output_compat_root / output_name
        hq_output_path = output_hq_root / output_name
        if args.all_compat or output_name == ACTIVE_RUNTIME_STRIP:
            write_sheet_pair(
                source_path,
                compat_output_path,
                hq_output_path,
                allow_upscale=False,
            )
            generated_compat.append(compat_output_path)
        else:
            # Keep non-runtime variants HQ-only by default to reduce clutter.
            hq_strip, _, _ = build_hq_strip_native(source_path)
            hq_output_path.parent.mkdir(parents=True, exist_ok=True)
            hq_strip.save(hq_output_path)
        generated_hq.append(hq_output_path)

    print("Generated Wes overworld strips (compat 16x32):")
    for path in generated_compat:
        print(f"- {path.relative_to(project_root)}")

    print("Generated Wes overworld strips (hq native per-sheet size):")
    for path in generated_hq:
        print(f"- {path.relative_to(project_root)}")


if __name__ == "__main__":
    main()
