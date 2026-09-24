"""Locate fixed-UV mip-2 normal error across triangle charts."""

import hashlib
import json
import math
import sys
from pathlib import Path

import numpy as np
from PIL import Image

from BakeMipAudit import sample


def audit(folder):
    bake = json.loads((folder / "bake.json").read_text(encoding="utf-8"))
    resolution = int(bake["resolution"])
    triangles = int(bake["lowTriangles"])
    padding = int(bake["padding"])
    columns = math.ceil(math.sqrt(triangles))
    tile = resolution // columns
    span = tile - 2 * padding
    image = np.asarray(Image.open(folder / "normal.png").convert("RGBA"), dtype=np.float32)
    if image.shape != (resolution, resolution, 4) or span <= 0:
        raise ValueError("invalid normal atlas")
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
    per_chart_max = angle.reshape(triangles, len(points)).max(axis=1)
    per_chart_energy = (angle**2).reshape(triangles, len(points)).sum(axis=1)
    vertices = []
    faces = []
    with (folder / "low.obj").open(encoding="utf-8") as source:
        for line in source:
            if line.startswith("v "):
                vertices.append(tuple(map(float, line.split()[1:4])))
            elif line.startswith("f "):
                faces.append(tuple(int(c.split("/")[0]) - 1 for c in line.split()[1:4]))
    if len(faces) != triangles:
        raise ValueError("low OBJ triangle count mismatch")
    centers = np.asarray(vertices)[np.asarray(faces, dtype=np.int32)].mean(axis=1)
    hotspot_ids = np.argsort(per_chart_energy)[-16:][::-1]
    ranked = np.sort(per_chart_energy)[::-1]
    total = float(ranked.sum())
    return {
        "input": str(folder),
        "normalSha256": hashlib.sha256((folder / "normal.png").read_bytes()).hexdigest(),
        "triangles": triangles, "samples": len(angle), "interiorSpanTexels": span,
        "sampleP95Degrees": float(np.percentile(angle, 95)),
        "chartMaximumOver5Degrees": float((per_chart_max > 5).mean()),
        "chartMaximumOver10Degrees": float((per_chart_max > 10).mean()),
        "chartMaximumOver20Degrees": float((per_chart_max > 20).mean()),
        "squaredErrorShareTop10PercentCharts": float(ranked[: math.ceil(triangles * 0.1)].sum() / total),
        "squaredErrorShareTop20PercentCharts": float(ranked[: math.ceil(triangles * 0.2)].sum() / total),
        "top16Charts": [{"triangle": int(index), "center": centers[index].tolist(),
                         "maximumAngleDegrees": float(per_chart_max[index])}
                        for index in hotspot_ids],
    }


if __name__ == "__main__":
    if len(sys.argv) != 4:
        raise SystemExit("usage: BakeMipHotspotAudit.py <first-bake-folder> <second-bake-folder> <output-json>")
    report = {"scope": "four fixed UVs per triangle, naive CPU box mip 2; hotspot ranking is not an adaptive UV solution",
              "scriptSha256": hashlib.sha256(Path(__file__).read_bytes()).hexdigest(),
              "cases": [audit(Path(sys.argv[1])), audit(Path(sys.argv[2]))]}
    Path(sys.argv[3]).write_text(json.dumps(report, indent=2) + "\n", encoding="utf-8")
    print(json.dumps(report["cases"], indent=2))
