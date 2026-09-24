"""Separate cross-chart filtering from within-chart normal loss in the test atlas."""

import hashlib
import json
import math
import sys
from pathlib import Path

import numpy as np
from PIL import Image

from BakeMipAudit import sample


def chart_mip_sample(image, x, y, tile_x, tile_y, tile, level):
    factor = 2**level
    mip_x = (x + 0.5) / factor - 0.5
    mip_y = (y + 0.5) / factor - 0.5
    x0 = np.floor(mip_x).astype(np.int32)
    y0 = np.floor(mip_y).astype(np.int32)
    fx = (mip_x - x0)[:, None]
    fy = (mip_y - y0)[:, None]

    def filtered(ix, iy):
        rgb = np.zeros((len(x), 3), dtype=np.float64)
        for dy in range(factor):
            py = np.clip(iy * factor + dy, tile_y, tile_y + tile - 1)
            for dx in range(factor):
                px = np.clip(ix * factor + dx, tile_x, tile_x + tile - 1)
                rgb += image[py, px, :3]
        return rgb / (factor * factor)

    c00 = filtered(x0, y0)
    c10 = filtered(x0 + 1, y0)
    c01 = filtered(x0, y0 + 1)
    c11 = filtered(x0 + 1, y0 + 1)
    rgb = (1 - fy) * ((1 - fx) * c00 + fx * c10) + fy * ((1 - fx) * c01 + fx * c11)
    vector = rgb * (2 / 255) - 1
    return vector / np.maximum(np.linalg.norm(vector, axis=1)[:, None], 1e-12)


def metrics(reference, candidate):
    angle = np.degrees(np.arccos(np.clip((reference * candidate).sum(axis=1), -1, 1)))
    return {
        "medianDegrees": float(np.median(angle)),
        "p95Degrees": float(np.percentile(angle, 95)),
        "p99Degrees": float(np.percentile(angle, 99)),
        "fractionOver10Degrees": float((angle > 10).mean()),
    }


def audit(folder):
    bake = json.loads((folder / "bake.json").read_text(encoding="utf-8"))
    resolution = int(bake["resolution"])
    triangles = int(bake["lowTriangles"])
    padding = int(bake["padding"])
    layout_path = folder / "chart-layout.json"
    if layout_path.exists():
        layout = np.asarray(json.loads(layout_path.read_text(encoding="utf-8")), dtype=np.int32)
        if layout.shape != (triangles, 3):
            raise ValueError("invalid explicit chart count")
        base_x, base_y, sizes = layout.T
    else:
        columns = math.ceil(math.sqrt(triangles))
        size = resolution // columns
        chart = np.arange(triangles, dtype=np.int32)
        base_x = chart % columns * size
        base_y = chart // columns * size
        sizes = np.full(triangles, size, dtype=np.int32)
    spans = sizes - 2 * padding
    if np.any(spans <= 0):
        raise ValueError("invalid chart size")
    image = np.asarray(Image.open(folder / "normal.png").convert("RGBA"), dtype=np.float32)
    points = np.tile(np.array([[1 / 3, 1 / 3], [0.15, 0.15], [0.7, 0.15], [0.15, 0.7]]), (triangles, 1))
    tile_x = np.repeat(base_x, 4)
    tile_y = np.repeat(base_y, 4)
    tile = np.repeat(sizes, 4)
    x = tile_x + padding + points[:, 0] * np.repeat(spans, 4)
    y = tile_y + padding + points[:, 1] * np.repeat(spans, 4)
    reference = sample(image, x, y)
    mip = image
    levels = []
    for level in range(1, 4):
        height, width, channels = mip.shape
        mip = mip.reshape(height // 2, 2, width // 2, 2, channels).mean(axis=(1, 3))
        factor = 2**level
        naive = sample(mip, (x + 0.5) / factor - 0.5, (y + 0.5) / factor - 0.5)
        isolated = chart_mip_sample(image, x, y, tile_x, tile_y, tile, level)
        levels.append({"level": level, "naive": metrics(reference, naive),
                       "chartConstrained": metrics(reference, isolated),
                       "naiveToConstrained": metrics(naive, isolated)})
    return {"input": str(folder), "normalSha256": hashlib.sha256((folder / "normal.png").read_bytes()).hexdigest(),
            "resolution": resolution, "triangles": triangles,
            "samples": len(x), "levels": levels}


if __name__ == "__main__":
    if len(sys.argv) != 3:
        raise SystemExit("usage: BakeMipIsolationAudit.py <normal-bake-folder> <output-json>")
    report = {"scope": "CPU fixed-UV diagnostic, chart-edge-clamped box footprint versus naive atlas box mip; neither is a production GPU mip chain",
              "scriptSha256": hashlib.sha256(Path(__file__).read_bytes()).hexdigest(),
              "case": audit(Path(sys.argv[1]))}
    Path(sys.argv[2]).write_text(json.dumps(report, indent=2) + "\n", encoding="utf-8")
    print(json.dumps(report["case"]["levels"], indent=2))
