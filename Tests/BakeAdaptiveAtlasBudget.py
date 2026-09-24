"""Build deterministic two-tier chart layouts for a future high-to-low rebake experiment."""

import hashlib
import json
import math
import sys
from pathlib import Path

import numpy as np
from PIL import Image

from BakeMipAudit import sample


def priority(folder):
    bake = json.loads((folder / "bake.json").read_text(encoding="utf-8"))
    resolution = int(bake["resolution"])
    triangles = int(bake["lowTriangles"])
    padding = int(bake["padding"])
    columns = math.ceil(math.sqrt(triangles))
    tile = resolution // columns
    span = tile - 2 * padding
    image = np.asarray(Image.open(folder / "normal.png").convert("RGBA"), dtype=np.float32)
    if image.shape != (resolution, resolution, 4) or span <= 0:
        raise ValueError("invalid source atlas")
    chart = np.arange(triangles, dtype=np.int32)
    points = np.array([[1 / 3, 1 / 3], [0.15, 0.15], [0.7, 0.15], [0.15, 0.7]])
    x = (chart[:, None] % columns * tile + padding + points[None, :, 0] * span).ravel()
    y = (chart[:, None] // columns * tile + padding + points[None, :, 1] * span).ravel()
    base = sample(image, x, y)
    mip = image
    for _ in range(2):
        height, width, channels = mip.shape
        mip = mip.reshape(height // 2, 2, width // 2, 2, channels).mean(axis=(1, 3))
    filtered = sample(mip, (x + 0.5) / 4 - 0.5, (y + 0.5) / 4 - 0.5)
    angle = np.degrees(np.arccos(np.clip((base * filtered).sum(axis=1), -1, 1)))
    energy = (angle**2).reshape(triangles, len(points)).sum(axis=1)
    top_count = math.ceil(triangles * 0.1)
    order = sorted(range(triangles), key=lambda index: (-energy[index], index))
    selected = set(order[:top_count])
    return bake, selected, float(energy[order[:top_count]].sum() / energy.sum())


def layout(triangles, selected, high_tile, other_tile, resolution, padding):
    if min(high_tile, other_tile) <= 2 * padding:
        raise ValueError("chart has no interior")
    ordered = sorted(selected) + [index for index in range(triangles) if index not in selected]
    rectangles = [None] * triangles
    occupied = np.zeros((resolution, resolution), dtype=np.bool_)
    x = y = row_height = 0
    for index in ordered:
        size = high_tile if index in selected else other_tile
        if x + size > resolution:
            y += row_height
            x = row_height = 0
        if y + size > resolution:
            raise ValueError(f"layout overflow: {high_tile}/{other_tile} at chart {index}")
        if occupied[y : y + size, x : x + size].any():
            raise ValueError("layout overlap")
        occupied[y : y + size, x : x + size] = True
        rectangles[index] = [x, y, size]
        x += size
        row_height = max(row_height, size)
    return rectangles, y + row_height, int(occupied.sum())


if __name__ == "__main__":
    if len(sys.argv) != 3:
        raise SystemExit("usage: BakeAdaptiveAtlasBudget.py <normal-bake-folder> <output-directory>")
    folder = Path(sys.argv[1])
    output = Path(sys.argv[2])
    output.mkdir(parents=True, exist_ok=True)
    bake, selected, share = priority(folder)
    resolution = int(bake["resolution"])
    triangles = int(bake["lowTriangles"])
    padding = int(bake["padding"])
    summary = []
    for high_tile, other_tile in ((22, 14), (24, 13)):
        rectangles, height, pixels = layout(triangles, selected, high_tile, other_tile,
                                            resolution, padding)
        name = f"chart-layout-{high_tile}-{other_tile}.json"
        report = {
            "scope": "deterministic shelf-packing feasibility only; no UV transfer, rebake, tangent or mip acceptance",
            "sourceNormalSha256": hashlib.sha256((folder / "normal.png").read_bytes()).hexdigest(),
            "sourceLowObjSha256": hashlib.sha256((folder / "low.obj").read_bytes()).hexdigest(),
            "sourceHighHash": int(bake["highHash"]),
            "sourceLowHash": int(bake["lowHash"]),
            "scriptSha256": hashlib.sha256(Path(__file__).read_bytes()).hexdigest(),
            "resolution": resolution, "padding": padding, "triangles": triangles,
            "priorityCharts": len(selected), "priorityEnergyShare": share,
            "priorityTile": high_tile, "otherTile": other_tile,
            "priorityInteriorSpan": high_tile - 2 * padding,
            "otherInteriorSpan": other_tile - 2 * padding,
            "usedHeight": height, "usedPixels": pixels,
            "occupancy": pixels / (resolution * resolution),
            "rectanglesByTriangle": rectangles,
        }
        (output / name).write_text(json.dumps(report, separators=(",", ":")) + "\n", encoding="utf-8")
        summary.append({"file": name, "priorityTile": high_tile, "otherTile": other_tile,
                        "usedHeight": height, "occupancy": report["occupancy"]})
    (output / "summary.json").write_text(json.dumps(summary, indent=2) + "\n", encoding="utf-8")
    print(json.dumps(summary, indent=2))
