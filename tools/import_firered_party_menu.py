import argparse
from pathlib import Path

from firered_import.context import make_context
from firered_import.party_menu_importer import PartyMenuImportOptions, import_party_menu_assets


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
    parser.add_argument("--include-background", action=argparse.BooleanOptionalAction, default=True)
    parser.add_argument("--include-slots", action=argparse.BooleanOptionalAction, default=True)
    parser.add_argument("--include-buttons", action=argparse.BooleanOptionalAction, default=True)
    parser.add_argument("--include-icons", action=argparse.BooleanOptionalAction, default=True)
    args = parser.parse_args()

    context = make_context(
        firered_root=args.firered_root,
        project_root=args.project_root,
        assets_root=args.assets_root,
    )
    options = PartyMenuImportOptions(
        include_background=args.include_background,
        include_slots=args.include_slots,
        include_buttons=args.include_buttons,
        include_icons=args.include_icons,
    )

    generated = import_party_menu_assets(context, options)
    print("Imported FireRed party menu graphics:")
    for path in generated:
        print(f"- {path}")


if __name__ == "__main__":
    main()

