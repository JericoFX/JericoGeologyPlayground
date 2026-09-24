"""Estimate normal-detail loss from naive box-filtered atlas mips at fixed UVs."""

import hashlib
import json
import math
import sys
from pathlib import Path

import numpy as np
from PIL import Image


def sample(image: np.ndarray, x: np.ndarray, y: np.ndarray) -> np.ndarray:
    height, width, _ = image.shape
    x = np.clip(x, 0, width - 1)
    y = np.clip(y, 0, height - 1)
    x0 = np.floor(x).astype(np.int32)
    y0 = np.floor(y).astype(np.int32)
    x1 = np.minimum(x0 + 1, width - 1)
    y1 = np.minimum(y0 + 1, height - 1)
    fx = (x - x0)[:, None]
    fy = (y - y0)[:, None]
    rgb = ((1 - fy) * ((1 - fx) * image[y0, x0, :3] + fx * image[y0, x1, :3]) +
           fy * ((1 - fx) * image[y1, x0, :3] + fx * image[y1, x1, :3]))
    vector = rgb * (2 / 255) - 1
    length = np.linalg.norm(vector, axis=1)
    return vector / np.maximum(length[:, None], 1e-12)


def audit(folder: Path) -> dict:
    bake = json.loads((folder / "bake.json").read_text(encoding="utf-8"))
    resolution = int(bake["resolution"])
    triangles = int(bake["lowTriangles"])
    padding = int(bake["padding"])
    columns = math.ceil(math.sqrt(triangles))
    tile = resolution // columns
    span = tile - 2 * padding
    if span <= 0:
        raise ValueError(f"invalid chart size: {folder}")
    image = np.asarray(Image.open(folder / "normal.png").convert("RGBA"), dtype=np.float32)
    if image.shape != (resolution, resolution, 4):
        raise ValueError(f"invalid normal map size: {folder}")
    chart = np.arange(triangles, dtype=np.int32)
    points = np.array([[1 / 3, 1 / 3], [0.15, 0.15], [0.7, 0.15], [0.15, 0.7]])
    x = (chart[:, None] % columns * tile + padding + points[None, :, 0] * span).ravel()
    y = (chart[:, None] // columns * tile + padding + points[None, :, 1] * span).ravel()
    original = sample(image, x, y)
    results = []
    mip = image
    for level in range(1, 4):
        height, width, channels = mip.shape
        mip = mip.reshape(height // 2, 2, width // 2, 2, channels).mean(axis=(1, 3))
        factor = 2**level
        filtered = sample(mip, (x + 0.5) / factor - 0.5, (y + 0.5) / factor - 0.5)
        angle = np.degrees(np.arccos(np.clip((original * filtered).sum(axis=1), -1, 1)))
        results.append({
            "mip": level,
            "medianAngleDegrees": float(np.median(angle)),
            "p95AngleDegrees": float(np.percentile(angle, 95)),
            "p99AngleDegrees": float(np.percentile(angle, 99)),
            "maximumAngleDegrees": float(angle.max()),
            "fractionOver5Degrees": float((angle > 5).mean()),
            "fractionOver10Degrees": float((angle > 10).mean()),
        })
    return {"family": folder.name, "lowTriangles": triangles, "sampledUvs": len(x), "mips": results}


if __name__ == "__main__":
    if len(sys.argv) != 3:
        raise SystemExit("usage: BakeMipAudit.py <bake-root> <output-json>")
    root = Path(sys.argv[1])
    cases = [audit(folder) for folder in sorted(root.glob("normal-bake-*")) if folder.is_dir()]
    if not cases:
        raise SystemExit(f"no bake folders in {root}")
    report = {
        "scope": "fixed-UV angle to base normal after naive 2x2 box mips; not GPU/runtime LOD or Beauty",
        "scriptSha256": hashlib.sha256(Path(__file__).read_bytes()).hexdigest(),
        "cases": cases,
    }
    Path(sys.argv[2]).write_text(json.dumps(report, indent=2) + "\n", encoding="utf-8")
    print(json.dumps(cases, indent=2))
