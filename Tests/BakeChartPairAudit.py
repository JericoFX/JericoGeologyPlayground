"""Estimate adjacent same-region triangle pairs eligible for future UV chart grouping."""

import hashlib
import json
import math
import sys
from collections import defaultdict
from pathlib import Path

import numpy as np
from PIL import Image


def audit(folder):
    bake = json.loads((folder / "bake.json").read_text(encoding="utf-8"))
    triangles = int(bake["lowTriangles"])
    resolution = int(bake["resolution"])
    padding = int(bake["padding"])
    vertices = []
    faces = []
    with (folder / "low.obj").open(encoding="utf-8") as source:
        for line in source:
            if line.startswith("v "):
                vertices.append(tuple(map(float, line.split()[1:4])))
            elif line.startswith("f "):
                faces.append(tuple(int(c.split("/")[0]) - 1 for c in line.split()[1:4]))
    if len(faces) != triangles:
        raise ValueError(f"face count mismatch in {folder}")

    positions = np.asarray(vertices, dtype=np.float64)
    indices = np.asarray(faces, dtype=np.int32)
    edges_a = positions[indices[:, 1]] - positions[indices[:, 0]]
    edges_b = positions[indices[:, 2]] - positions[indices[:, 0]]
    normals = np.cross(edges_a, edges_b)
    lengths = np.linalg.norm(normals, axis=1)
    if np.any(lengths < 1e-12):
        raise ValueError("degenerate low triangle")
    normals /= lengths[:, None]

    columns = math.ceil(math.sqrt(triangles))
    tile = resolution // columns
    span = tile - 2 * padding
    region = np.asarray(Image.open(folder / "region-id.png").convert("RGBA"))
    if region.shape[:2] != (resolution, resolution):
        raise ValueError("region map size mismatch")
    center_x = np.arange(triangles) % columns * tile + padding + span // 3
    center_y = np.arange(triangles) // columns * tile + padding + span // 3
    ids = region[center_y, center_x]
    if np.any(ids[:, 3] == 0):
        raise ValueError("missing center region ID")

    owners = defaultdict(list)
    for face_id, face in enumerate(faces):
        for a, b in ((face[0], face[1]), (face[1], face[2]), (face[2], face[0])):
            owners[(min(a, b), max(a, b))].append(face_id)
    candidates = []
    shared = 0
    same_region = 0
    for adjacent in owners.values():
        if len(adjacent) != 2:
            continue
        shared += 1
        a, b = adjacent
        if not np.array_equal(ids[a], ids[b]):
            continue
        same_region += 1
        dot = float(np.clip(np.dot(normals[a], normals[b]), -1, 1))
        if dot > 0:
            candidates.append((math.degrees(math.acos(dot)), a, b))
    candidates.sort()
    results = []
    for limit in (5, 10, 15, 20):
        paired = np.zeros(triangles, dtype=bool)
        pairs = 0
        for angle, a, b in candidates:
            if angle > limit:
                break
            if not paired[a] and not paired[b]:
                paired[a] = paired[b] = True
                pairs += 1
        charts = triangles - pairs
        target_tile = resolution // math.ceil(math.sqrt(charts))
        results.append({"maximumNormalAngleDegrees": limit, "pairs": pairs,
                        "triangleFractionInPairs": 2 * pairs / triangles,
                        "remainingCharts": charts, "potentialTileTexels": target_tile,
                        "potentialInteriorSpanTexels": target_tile - 2 * padding})
    return {"input": str(folder),
            "lowObjSha256": hashlib.sha256((folder / "low.obj").read_bytes()).hexdigest(),
            "regionMapSha256": hashlib.sha256((folder / "region-id.png").read_bytes()).hexdigest(),
            "triangles": triangles, "sharedEdges": shared,
            "sameRegionSharedEdges": same_region, "originalTileTexels": tile,
            "originalInteriorSpanTexels": span, "thresholds": results}


if __name__ == "__main__":
    if len(sys.argv) != 3:
        raise SystemExit("usage: BakeChartPairAudit.py <normal-bake-folder> <output-json>")
    report = {"scope": "topology, region and face-normal-angle pairing bound only; no UV chart, tangent basis, bake, distortion or seam validation",
              "scriptSha256": hashlib.sha256(Path(__file__).read_bytes()).hexdigest(),
              "case": audit(Path(sys.argv[1]))}
    Path(sys.argv[2]).write_text(json.dumps(report, indent=2) + "\n", encoding="utf-8")
    print(json.dumps(report["case"], indent=2))
