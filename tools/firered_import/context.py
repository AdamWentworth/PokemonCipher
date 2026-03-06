from __future__ import annotations

from dataclasses import dataclass
from pathlib import Path


@dataclass(frozen=True)
class ImportContext:
    firered_root: Path
    project_root: Path
    assets_root: Path


def make_context(
    firered_root: Path,
    project_root: Path,
    assets_root: Path | None = None,
) -> ImportContext:
    resolved_project_root = project_root.resolve()
    resolved_assets_root = assets_root.resolve() if assets_root is not None else (resolved_project_root / "assets")

    return ImportContext(
        firered_root=firered_root.resolve(),
        project_root=resolved_project_root,
        assets_root=resolved_assets_root,
    )

