"""Validate explicit chart UVs and compare fixed-UV mip loss after a native rebake."""

import hashlib
import json
import math
import sys
from pathlib import Path

import numpy as np
from PIL import Image

from BakeMipAudit import sample


def p95(values):
    return float(np.percentile(values, 95))


def measure_mips(image, x, y, priority):
    reference = sample(image, x, y)
    priority_samples = np.repeat(priority, 4)
    results = []
    mip = image
    for level in range(1, 4):
        height, width, channels = mip.shape
        mip = mip.reshape(height // 2, 2, width // 2, 2, channels).mean(axis=(1, 3))
        factor = 2**level
        filtered = sample(mip, (x + 0.5) / factor - 0.5, (y + 0.5) / factor - 0.5)
        angle = np.degrees(np.arccos(np.clip((reference * filtered).sum(axis=1), -1, 1)))
        results.append({"mip": level, "p95Degrees": p95(angle),
                        "p95PriorityDegrees": p95(angle[priority_samples]),
                        "p95OtherDegrees": p95(angle[~priority_samples]),
                        "fractionOver10Degrees": float((angle > 10).mean())})
    return results


def audit(folder: Path, source_layout: Path, baseline_folder: Path):
    bake = json.loads((folder / "bake.json").read_text(encoding="utf-8"))
    layout = json.loads((folder / "chart-layout.json").read_text(encoding="utf-8"))
    source = json.loads(source_layout.read_text(encoding="utf-8"))
    resolution = int(bake["resolution"])
    triangles = int(bake["lowTriangles"])
    padding = int(bake["padding"])
    if layout != source["rectanglesByTriangle"] or len(layout) != triangles:
        raise ValueError("exported layout differs from input or triangle count")
    if int(bake["highHash"]) != int(source["sourceHighHash"]) or \
            int(bake["lowHash"]) != int(source["sourceLowHash"]):
        raise ValueError("exported mesh identity differs from layout source")
    rectangles = np.asarray(layout, dtype=np.int32)
    x0, y0, tile = rectangles.T
    span = tile - 2 * padding
    if np.any(x0 < 0) or np.any(y0 < 0) or np.any(span <= 0) or \
            np.any(x0 + tile > resolution) or np.any(y0 + tile > resolution):
        raise ValueError("chart outside valid atlas bounds")
    occupied = np.zeros((resolution, resolution), dtype=np.bool_)
    for x, y, size in rectangles:
        area = occupied[y:y + size, x:x + size]
        if area.any():
            raise ValueError("chart overlap")
        area[:] = True

    uv = []
    with (folder / "low.obj").open(encoding="utf-8") as obj:
        for line in obj:
            if line.startswith("vt "):
                uv.append(tuple(map(float, line.split()[1:3])))
    uv = np.asarray(uv)
    if uv.shape != (triangles * 3, 2):
        raise ValueError("low OBJ UV count mismatch")
    for corner in range(3):
        expected_u = (x0 + padding + (span if corner == 1 else 0)) / resolution
        expected_v = 1 - (y0 + padding + (span if corner == 2 else 0)) / resolution
        if np.max(np.abs(uv[corner::3, 0] - expected_u)) > 1e-6 or \
                np.max(np.abs(uv[corner::3, 1] - expected_v)) > 1e-6:
            raise ValueError("exported OBJ UV does not match explicit chart rectangle")

    image = np.asarray(Image.open(folder / "normal.png").convert("RGBA"), dtype=np.float32)
    if image.shape != (resolution, resolution, 4):
        raise ValueError("normal map dimensions differ from atlas")
    used_missing = sum(int((image[y:y + size, x:x + size, 3] == 0).sum())
                       for x, y, size in rectangles)
    if used_missing:
        raise ValueError(f"used charts contain {used_missing} alpha holes")
    points = np.array([[1 / 3, 1 / 3], [0.15, 0.15], [0.7, 0.15], [0.15, 0.7]])
    x = (x0[:, None] + padding + points[None, :, 0] * span[:, None]).ravel()
    y = (y0[:, None] + padding + points[None, :, 1] * span[:, None]).ravel()
    priority = tile == int(source["priorityTile"])
    results = measure_mips(image, x, y, priority)
    baseline_bake = json.loads((baseline_folder / "bake.json").read_text(encoding="utf-8"))
    if int(baseline_bake["lowHash"]) != int(bake["lowHash"]) or \
            int(baseline_bake["highHash"]) != int(bake["highHash"]):
        raise ValueError("baseline geometry differs from adaptive bake")
    baseline = np.asarray(Image.open(baseline_folder / "normal.png").convert("RGBA"),
                          dtype=np.float32)
    columns = math.ceil(math.sqrt(triangles))
    baseline_tile = resolution // columns
    baseline_span = baseline_tile - 2 * padding
    chart = np.arange(triangles, dtype=np.int32)
    baseline_x = (chart[:, None] % columns * baseline_tile + padding +
                  points[None, :, 0] * baseline_span).ravel()
    baseline_y = (chart[:, None] // columns * baseline_tile + padding +
                  points[None, :, 1] * baseline_span).ravel()
    baseline_results = measure_mips(baseline, baseline_x, baseline_y, priority)
    return {"normalSha256": hashlib.sha256((folder / "normal.png").read_bytes()).hexdigest(),
            "layoutSha256": hashlib.sha256((folder / "chart-layout.json").read_bytes()).hexdigest(),
            "lowObjSha256": hashlib.sha256((folder / "low.obj").read_bytes()).hexdigest(),
            "resolution": resolution, "triangles": triangles,
            "priorityCharts": int(priority.sum()), "occupiedPixels": int(occupied.sum()),
            "uvTolerance": 1e-6, "usedChartMissingAlpha": used_missing,
            "mips": results, "baselineMips": baseline_results}


if __name__ == "__main__":
    if len(sys.argv) != 5:
        raise SystemExit("usage: BakeAdaptiveExportAudit.py <normal-bake-folder> <source-layout> <baseline-bake-folder> <output-json>")
    report = {"scope": "explicit atlas export integrity and naive CPU fixed-UV box mips; not GPU LOD or Beauty",
              "scriptSha256": hashlib.sha256(Path(__file__).read_bytes()).hexdigest(),
              "result": audit(Path(sys.argv[1]), Path(sys.argv[2]), Path(sys.argv[3]))}
    Path(sys.argv[4]).write_text(json.dumps(report, indent=2) + "\n", encoding="utf-8")
    print(json.dumps(report["result"], indent=2))
