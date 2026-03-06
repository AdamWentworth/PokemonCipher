from __future__ import annotations

import struct
from pathlib import Path


def read_u16_values(path: Path) -> list[int]:
    data = path.read_bytes()
    if len(data) % 2 != 0:
        raise ValueError(f"Unexpected odd-sized tilemap: {path}")
    return list(struct.unpack(f"<{len(data) // 2}H", data))


def read_u8_values(path: Path) -> list[int]:
    return list(path.read_bytes())

