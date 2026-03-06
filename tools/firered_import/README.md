# FireRed Import Modules

This package contains reusable import modules used by the CLI scripts in `tools/`.

## Modules
- `summary_importer.py`: summary UI, fonts, Eevee assets.
- `party_menu_importer.py`: party menu UI assets.
- `map_pipeline.py`: active map import pipeline orchestration.
- `map_graphics.py`: map decode, palette, and render helpers.
- `map_layout.py`: map metadata, spawn, warp, encounter, and NPC helpers.
- `map_writers.py`: `tmx` and `pcmap` writers.
- `map_importer.py`: legacy monolithic implementation retained temporarily during the split.
- `image_utils.py`: shared image copy/tilemap render helpers.
- `palette_utils.py`: shared palette parsing and indexed recolor helpers.
- `binary.py`: shared binary readers.
- `context.py`: shared path/context model.

## CLI Wrappers
- `tools/import_firered_summary_assets.py`
- `tools/import_firered_party_menu.py`
- `tools/import_firered_pallet.py`

Each wrapper now supports include flags so imports can be scoped to specific output groups.
