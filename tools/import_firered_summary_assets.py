import argparse
from pathlib import Path

from firered_import.context import make_context
from firered_import.summary_importer import SummaryImportOptions, import_summary_assets


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
    parser.add_argument("--include-pages", action=argparse.BooleanOptionalAction, default=True)
    parser.add_argument("--include-shared-ui", action=argparse.BooleanOptionalAction, default=True)
    parser.add_argument("--include-fonts", action=argparse.BooleanOptionalAction, default=True)
    parser.add_argument("--include-eevee", action=argparse.BooleanOptionalAction, default=True)
    parser.add_argument("--include-eevee-shiny", action=argparse.BooleanOptionalAction, default=True)
    args = parser.parse_args()

    context = make_context(
        firered_root=args.firered_root,
        project_root=args.project_root,
        assets_root=args.assets_root,
    )
    options = SummaryImportOptions(
        include_pages=args.include_pages,
        include_shared_ui=args.include_shared_ui,
        include_fonts=args.include_fonts,
        include_eevee=args.include_eevee,
        include_eevee_shiny=args.include_eevee_shiny,
    )

    generated = import_summary_assets(context, options)
    print("Imported FireRed summary assets:")
    for path in generated:
        print(f"- {path}")


if __name__ == "__main__":
    main()

