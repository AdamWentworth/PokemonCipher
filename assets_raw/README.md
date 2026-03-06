# assets_raw

`assets_raw/` is the editable import workspace.

Rules:
- Treat files here as source inputs only (raw extraction, palette experiments, temporary conversion inputs).
- Do not load runtime assets from this folder in game code.
- Build canonical runtime assets into `assets/` using `tools/build_assets.py`.
- Use `python tools/build_assets.py --verify-manifest` to ensure generated `assets/` still match current import scripts/manifests.

Typical workflow:
1. Update import scripts/manifests under `tools/`.
2. Rebuild generated assets into `assets/`.
3. Verify manifest before commit.
