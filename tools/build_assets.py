import argparse
import hashlib
import json
import shutil
import subprocess
import sys
from datetime import datetime, timezone
from pathlib import Path


def run_python_step(description: str, script_path: Path, script_args: list[str]) -> None:
    command = [sys.executable, str(script_path), *script_args]
    print(f"[asset-build] {description}")
    print(f"[asset-build] -> {' '.join(command)}")
    subprocess.run(command, check=True)


def sha256_file(path: Path) -> str:
    digest = hashlib.sha256()
    with path.open("rb") as stream:
        while True:
            chunk = stream.read(1024 * 1024)
            if not chunk:
                break
            digest.update(chunk)
    return digest.hexdigest()


def load_map_manifest(path: Path) -> list[dict]:
    if not path.exists():
        raise FileNotFoundError(f"Map manifest not found: {path}")

    data = json.loads(path.read_text(encoding="utf-8"))
    maps = data.get("maps", [])
    if not isinstance(maps, list):
        raise ValueError(f"Invalid map manifest format in {path}: expected 'maps' array.")

    normalized: list[dict] = []
    for entry in maps:
        if not isinstance(entry, dict):
            continue

        map_name = entry.get("map_name")
        if not isinstance(map_name, str) or not map_name:
            continue

        normalized.append(entry)

    return normalized


def run_map_imports(
    map_entries: list[dict],
    common_args: list[str],
    map_script: Path,
    *,
    label: str,
) -> None:
    for entry in map_entries:
        step_args = list(common_args)
        step_args.extend(["--map-name", str(entry["map_name"])])

        map_slug = entry.get("map_slug")
        if map_slug:
            step_args.extend(["--map-slug", str(map_slug)])

        layout_id = entry.get("layout_id")
        if layout_id:
            step_args.extend(["--layout-id", str(layout_id)])

        spawn_x = entry.get("spawn_tile_x")
        if isinstance(spawn_x, int):
            step_args.extend(["--spawn-tile-x", str(spawn_x)])

        spawn_y = entry.get("spawn_tile_y")
        if isinstance(spawn_y, int):
            step_args.extend(["--spawn-tile-y", str(spawn_y)])

        run_python_step(
            f"{label} '{entry['map_name']}'",
            map_script,
            step_args,
        )


def remove_if_exists(path: Path) -> None:
    if not path.exists():
        return
    if path.is_dir():
        shutil.rmtree(path)
    else:
        path.unlink()


def clean_generated_assets(assets_root: Path) -> None:
    generated_paths = [
        assets_root / "fonts",
        assets_root / "ui" / "party_menu",
        assets_root / "ui" / "summary_screen",
        assets_root / "maps",
        assets_root / "tilesets",
        assets_root / "pokemon" / "eevee",
        assets_root / "characters" / "red" / "red_normal.png",
        assets_root / "animations" / "red_overworld.xml",
        assets_root / ".build_manifest.json",
    ]
    for path in generated_paths:
        remove_if_exists(path)


def compute_input_hashes(
    project_root: Path,
    map_manifest: Path | None,
    with_summary: bool,
    with_party: bool,
    with_maps: bool,
) -> dict[str, str]:
    candidate_paths: list[Path] = [project_root / "tools" / "build_assets.py"]

    if with_summary:
        candidate_paths.append(project_root / "tools" / "import_firered_summary_assets.py")

    if with_party:
        candidate_paths.append(project_root / "tools" / "import_firered_party_menu.py")

    if with_maps:
        candidate_paths.append(project_root / "tools" / "import_firered_pallet.py")
        if map_manifest is not None:
            candidate_paths.append(map_manifest)

    hashes: dict[str, str] = {}
    for path in candidate_paths:
        if not path.exists() or not path.is_file():
            continue
        relative_path = path.relative_to(project_root).as_posix()
        hashes[relative_path] = sha256_file(path)

    return dict(sorted(hashes.items(), key=lambda pair: pair[0]))


def verify_build_metadata(
    assets_root: Path,
    expected_firered_root: Path,
    expected_with_summary: bool,
    expected_with_party: bool,
    expected_with_maps: bool,
    expected_map_manifest: Path | None,
    expected_input_hashes: dict[str, str],
) -> tuple[bool, list[str]]:
    manifest_path = assets_root / ".build_manifest.json"
    if not manifest_path.exists():
        return False, [f"Missing build manifest: {manifest_path}"]

    try:
        manifest = json.loads(manifest_path.read_text(encoding="utf-8"))
    except Exception as exc:
        return False, [f"Failed to parse build manifest: {exc}"]

    errors: list[str] = []
    if manifest.get("firered_root") != str(expected_firered_root):
        errors.append("firered_root mismatch in build manifest.")

    stages = manifest.get("stages", {})
    if not isinstance(stages, dict):
        errors.append("stages field is missing or invalid in build manifest.")
    else:
        if bool(stages.get("summary")) != expected_with_summary:
            errors.append("summary stage flag mismatch in build manifest.")
        if bool(stages.get("party_menu")) != expected_with_party:
            errors.append("party_menu stage flag mismatch in build manifest.")
        if bool(stages.get("maps")) != expected_with_maps:
            errors.append("maps stage flag mismatch in build manifest.")

    expected_map_manifest_text = str(expected_map_manifest) if expected_with_maps and expected_map_manifest else None
    if manifest.get("map_manifest") != expected_map_manifest_text:
        errors.append("map_manifest path mismatch in build manifest.")

    manifest_hashes = manifest.get("input_hashes", {})
    if not isinstance(manifest_hashes, dict):
        errors.append("input_hashes field is missing or invalid in build manifest.")
    elif manifest_hashes != expected_input_hashes:
        errors.append("input_hashes mismatch in build manifest.")

    return len(errors) == 0, errors


def write_build_metadata(
    output_root: Path,
    firered_root: Path,
    with_summary: bool,
    with_party: bool,
    with_maps: bool,
    map_manifest: Path | None,
    input_hashes: dict[str, str],
) -> None:
    metadata = {
        "generated_at_utc": datetime.now(timezone.utc).isoformat(),
        "firered_root": str(firered_root),
        "stages": {
            "summary": with_summary,
            "party_menu": with_party,
            "maps": with_maps,
        },
        "map_manifest": str(map_manifest) if map_manifest else None,
        "input_hashes": input_hashes,
    }

    output_root.mkdir(parents=True, exist_ok=True)
    (output_root / ".build_manifest.json").write_text(json.dumps(metadata, indent=2), encoding="utf-8")


def main() -> None:
    parser = argparse.ArgumentParser(description="Build PokemonCipher assets directly into the canonical assets/ folder.")
    parser.add_argument("--project-root", type=Path, default=Path(__file__).resolve().parents[1])
    parser.add_argument("--firered-root", type=Path, default=Path(r"C:\Code\pokefirered"))
    parser.add_argument("--assets-raw-root", type=Path, default=None, help="Raw source workspace root.")
    parser.add_argument("--assets-root", type=Path, default=None, help="Canonical assets output root.")
    parser.add_argument("--map-manifest", type=Path, default=None, help="JSON manifest for map imports.")
    parser.add_argument("--clean-assets", action="store_true", help="Delete generated asset outputs before rebuilding.")
    parser.add_argument("--verify-manifest", action="store_true", help="Verify existing .build_manifest.json and exit.")
    parser.add_argument("--with-summary", action=argparse.BooleanOptionalAction, default=True)
    parser.add_argument("--with-party", action=argparse.BooleanOptionalAction, default=True)
    parser.add_argument("--with-maps", action=argparse.BooleanOptionalAction, default=True)
    args = parser.parse_args()

    project_root = args.project_root.resolve()
    firered_root = args.firered_root.resolve()
    assets_raw_root = args.assets_raw_root.resolve() if args.assets_raw_root else (project_root / "assets_raw")
    assets_root = args.assets_root.resolve() if args.assets_root else (project_root / "assets")
    map_manifest = args.map_manifest.resolve() if args.map_manifest else (project_root / "tools" / "map_import_manifest.json")

    assets_raw_root.mkdir(parents=True, exist_ok=True)
    assets_root.mkdir(parents=True, exist_ok=True)

    if args.clean_assets:
        print(f"[asset-build] Cleaning generated assets under {assets_root}")
        clean_generated_assets(assets_root)

    summary_script = project_root / "tools" / "import_firered_summary_assets.py"
    party_script = project_root / "tools" / "import_firered_party_menu.py"
    map_script = project_root / "tools" / "import_firered_pallet.py"

    common_args = [
        "--firered-root",
        str(firered_root),
        "--project-root",
        str(project_root),
        "--assets-root",
        str(assets_root),
    ]

    input_hashes = compute_input_hashes(
        project_root=project_root,
        map_manifest=map_manifest if args.with_maps else None,
        with_summary=args.with_summary,
        with_party=args.with_party,
        with_maps=args.with_maps,
    )

    if args.verify_manifest:
        is_valid, errors = verify_build_metadata(
            assets_root=assets_root,
            expected_firered_root=firered_root,
            expected_with_summary=args.with_summary,
            expected_with_party=args.with_party,
            expected_with_maps=args.with_maps,
            expected_map_manifest=map_manifest if args.with_maps else None,
            expected_input_hashes=input_hashes,
        )
        if is_valid:
            print("[asset-build] Build manifest is valid for current inputs.")
            return

        print("[asset-build] Build manifest verification failed:")
        for error in errors:
            print(f"[asset-build] - {error}")
        raise SystemExit(2)

    if args.with_summary:
        run_python_step("Importing FireRed summary + Eevee assets", summary_script, common_args)

    if args.with_party:
        run_python_step("Importing FireRed party menu assets", party_script, common_args)

    if args.with_maps:
        map_entries = load_map_manifest(map_manifest)
        # Pass 1 creates/updates all TMX files.
        run_map_imports(
            map_entries,
            common_args,
            map_script,
            label="Importing FireRed map",
        )
        # Pass 2 re-runs imports so auto-generated connection warps can resolve against all maps.
        run_map_imports(
            map_entries,
            common_args,
            map_script,
            label="Reconciling connection warps for map",
        )

    write_build_metadata(
        output_root=assets_root,
        firered_root=firered_root,
        with_summary=args.with_summary,
        with_party=args.with_party,
        with_maps=args.with_maps,
        map_manifest=map_manifest if args.with_maps else None,
        input_hashes=input_hashes,
    )

    print("[asset-build] Done.")


if __name__ == "__main__":
    main()
