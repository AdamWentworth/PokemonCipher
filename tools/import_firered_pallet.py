from firered_import.map_pipeline import import_map_assets, parse_options_from_args


def main() -> None:
    options = parse_options_from_args()
    print(f"Imported FireRed map assets for {options.map_name}:")
    import_map_assets(options)


if __name__ == "__main__":
    main()
