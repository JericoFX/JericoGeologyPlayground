"""Inspect alpha coverage of active triangle tiles and their filter padding."""

import hashlib
import json
import math
import sys
from pathlib import Path

import numpy as np
from PIL import Image


def expanded(mask: np.ndarray, radius: int) -> np.ndarray:
    result = np.zeros_like(mask)
    height, width = mask.shape
    for dy in range(-radius, radius + 1):
        for dx in range(-radius, radius + 1):
            source_y = slice(max(0, -dy), min(height, height - dy))
            source_x = slice(max(0, -dx), min(width, width - dx))
            target_y = slice(max(0, dy), min(height, height + dy))
            target_x = slice(max(0, dx), min(width, width + dx))
            result[target_y, target_x] |= mask[source_y, source_x]
    return result


def audit(folder: Path) -> dict:
    bake = json.loads((folder / "bake.json").read_text(encoding="utf-8"))
    resolution = int(bake["resolution"])
    triangles = int(bake["lowTriangles"])
    padding = int(bake["padding"])
    columns = math.ceil(math.sqrt(triangles))
    tile = resolution // columns
    span = tile - 2 * padding
    alpha = np.asarray(Image.open(folder / "normal.png").getchannel("A"))
    if alpha.shape != (resolution, resolution) or span <= 0:
        raise ValueError(f"invalid atlas geometry: {folder}")
    coordinate = (np.arange(tile) + 0.5 - padding) / span
    u, v = np.meshgrid(coordinate, coordinate)
    covered = (u >= 0) & (v >= 0) & (u + v <= 1)
    near = expanded(covered, padding)
    inside_missing = near_missing = used_missing = affected = 0
    for triangle in range(triangles):
        x0 = (triangle % columns) * tile
        y0 = (triangle // columns) * tile
        missing = alpha[y0 : y0 + tile, x0 : x0 + tile] == 0
        inside_missing += int((missing & covered).sum())
        near_missing += int((missing & near).sum())
        used_missing += int(missing.sum())
        affected += bool((missing & near).any())
    return {
        "family": folder.name,
        "lowTriangles": triangles,
        "tileTexels": tile,
        "paddingTexels": padding,
        "usedTileMissingAlpha": used_missing,
        "coveredMissingAlpha": inside_missing,
        "withinPaddingMissingAlpha": near_missing,
        "chartsWithPaddingMisses": affected,
    }


if __name__ == "__main__":
    if len(sys.argv) != 3:
        raise SystemExit("usage: BakePaddingAudit.py <bake-root> <output-json>")
    root = Path(sys.argv[1])
    cases = [audit(folder) for folder in sorted(root.glob("normal-bake-*")) if folder.is_dir()]
    if not cases:
        raise SystemExit(f"no bake folders in {root}")
    report = {
        "scope": "alpha coverage in active triangle charts; no mip images or GPU filtering",
        "scriptSha256": hashlib.sha256(Path(__file__).read_bytes()).hexdigest(),
        "cases": cases,
    }
    Path(sys.argv[2]).write_text(json.dumps(report, indent=2) + "\n", encoding="utf-8")
    print(json.dumps(cases, indent=2))
