"""Measure normal and specialist-map changes across exported bake mips at fixed UVs."""

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


def sample_scalar(image: np.ndarray, x: np.ndarray, y: np.ndarray) -> np.ndarray:
    height, width, _ = image.shape
    x = np.clip(x, 0, width - 1)
    y = np.clip(y, 0, height - 1)
    x0 = np.floor(x).astype(np.int32)
    y0 = np.floor(y).astype(np.int32)
    x1 = np.minimum(x0 + 1, width - 1)
    y1 = np.minimum(y0 + 1, height - 1)
    fx = x - x0
    fy = y - y0
    return ((1 - fy) * ((1 - fx) * image[y0, x0, 0] + fx * image[y0, x1, 0]) +
            fy * ((1 - fx) * image[y1, x0, 0] + fx * image[y1, x1, 0]))


def sample_region(image: np.ndarray, x: np.ndarray, y: np.ndarray) -> np.ndarray:
    ix = np.clip(np.rint(x).astype(np.int32), 0, image.shape[1] - 1)
    iy = np.clip(np.rint(y).astype(np.int32), 0, image.shape[0] - 1)
    rgb = image[iy, ix, :3].astype(np.uint32)
    return (rgb[:, 0] << 16) | (rgb[:, 1] << 8) | rgb[:, 2]


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


def audit_auto_uv(folder: Path) -> dict:
    bake = json.loads((folder / "bake.json").read_text(encoding="utf-8"))
    resolution = int(bake["resolution"])
    mip_count = int(bake["normalMipLevels"])
    if mip_count < 2:
        raise ValueError(f"automatic UV bake has no normal mips: {folder}")
    uvs = []
    faces = []
    with (folder / "low.obj").open(encoding="utf-8") as stream:
        for line in stream:
            if line.startswith("vt "):
                uvs.append([float(value) for value in line.split()[1:3]])
            elif line.startswith("f "):
                faces.append([int(value.split("/")[1]) - 1 for value in line.split()[1:4]])
    uv = np.asarray(uvs, dtype=np.float64)
    face = np.asarray(faces, dtype=np.int64)
    if face.shape != (int(bake["lowTriangles"]), 3) or not np.isfinite(uv).all():
        raise ValueError(f"invalid automatic UV mesh: {folder}")
    island = np.asarray(json.loads((folder / "uv-chart-islands.json").read_text(encoding="utf-8")),
                        dtype=np.int64)
    if (not len(face) or island.shape != (len(face),) or np.any(island < 0) or
            np.max(island) >= int(bake["automaticUvIslandCount"])):
        raise ValueError(f"invalid automatic UV island identities: {folder}")
    weights = np.asarray([[1 / 3, 1 / 3, 1 / 3], [0.75, 0.125, 0.125],
                          [0.125, 0.75, 0.125], [0.125, 0.125, 0.75]])
    points = np.einsum("sc,fck->fsk", weights, uv[face]).reshape(-1, 2)
    island_corners = np.repeat(island, 3)
    corners = uv[face].reshape(-1, 2)
    island_count = int(bake["automaticUvIslandCount"])
    island_min = np.full((island_count, 2), np.inf)
    island_max = np.full((island_count, 2), -np.inf)
    np.minimum.at(island_min, island_corners, corners)
    np.maximum.at(island_max, island_corners, corners)
    if not np.isfinite(island_min).all() or not np.isfinite(island_max).all():
        raise ValueError(f"automatic UV island without triangles: {folder}")
    sample_island = np.repeat(island, len(weights))
    chart_boundary_distance = np.minimum(points - island_min[sample_island],
                                         island_max[sample_island] - points).min(axis=1) * resolution
    # A mip-3 bilinear footprint spans up to 16 base pixels. Samples farther from
    # the packed island rectangle cannot incorporate a different island's texels.
    interior = chart_boundary_distance >= 16
    base = np.asarray(Image.open(folder / "normal.png").convert("RGBA"), dtype=np.float32)
    if base.shape != (resolution, resolution, 4):
        raise ValueError(f"invalid automatic UV normal dimensions: {folder}")
    owner = np.full((resolution, resolution), -1, dtype=np.int32)
    for chart in range(island_count):
        x0 = max(0, math.floor(island_min[chart, 0] * resolution) - int(bake["padding"]))
        x1 = min(resolution, math.ceil(island_max[chart, 0] * resolution) + int(bake["padding"]))
        y0 = max(0, math.floor((1 - island_max[chart, 1]) * resolution) - int(bake["padding"]))
        y1 = min(resolution, math.ceil((1 - island_min[chart, 1]) * resolution) + int(bake["padding"]))
        if np.any(owner[y0:y1, x0:x1] >= 0):
            raise ValueError(f"automatic UV chart padding rectangles overlap: {folder}")
        owner[y0:y1, x0:x1] = chart
    owner[base[:, :, 3] == 0] = -1
    if np.any((base[:, :, 3] > 0) & (owner < 0)):
        raise ValueError(f"automatic UV painted texel has no chart owner: {folder}")

    def coordinates(size):
        return points[:, 0] * size - 0.5, (1 - points[:, 1]) * size - 0.5

    def unpainted(image, x, y):
        ix = np.clip(np.rint(x).astype(np.int32), 0, image.shape[1] - 1)
        iy = np.clip(np.rint(y).astype(np.int32), 0, image.shape[0] - 1)
        return int(np.count_nonzero(image[iy, ix, 3] == 0))

    x, y = coordinates(resolution)
    original = sample(base, x, y)
    base_holes = unpainted(base, x, y)
    results = []
    for level in range(1, mip_count):
        size = resolution // (2**level)
        blocks = owner.reshape(size, 2, size, 2).transpose(0, 2, 1, 3).reshape(size, size, 4)
        lowest = np.where(blocks >= 0, blocks, island_count).min(axis=2)
        highest = blocks.max(axis=2)
        mixed = np.any(blocks == -2, axis=2) | ((highest >= 0) & (lowest != highest))
        owner = np.where(mixed, -2, np.where(highest < 0, -1, highest))
        mixed_texels = int(np.count_nonzero(owner == -2))
        mip = np.asarray(Image.open(folder / f"normal-mip-{level}.png").convert("RGBA"),
                         dtype=np.float32)
        if mip.shape != (size, size, 4):
            raise ValueError(f"invalid automatic UV mip dimensions: {folder} level {level}")
        if np.any((mip[:, :, 3] > 0) != (owner != -1)):
            raise ValueError(f"automatic UV mip alpha disagrees with chart ownership: {folder} level {level}")
        x, y = coordinates(size)
        filtered = sample(mip, x, y)
        angle = np.degrees(np.arccos(np.clip((original * filtered).sum(axis=1), -1, 1)))
        x0 = np.clip(np.floor(x).astype(np.int32), 0, size - 1)
        y0 = np.clip(np.floor(y).astype(np.int32), 0, size - 1)
        x1 = np.minimum(x0 + 1, size - 1)
        y1 = np.minimum(y0 + 1, size - 1)
        footprint = np.stack((owner[y0, x0], owner[y0, x1],
                              owner[y1, x0], owner[y1, x1]), axis=1)
        cross_island = np.any((footprint != -1) & (footprint != sample_island[:, None]), axis=1)
        fx = np.clip(x, 0, size - 1) - x0
        fy = np.clip(y, 0, size - 1) - y0
        weights = np.stack(((1 - fx) * (1 - fy), fx * (1 - fy),
                            (1 - fx) * fy, fx * fy), axis=1)
        empty_weight = np.sum(weights * (footprint == -1), axis=1)
        results.append({
            "mip": level,
            "medianAngleDegrees": float(np.median(angle)),
            "p95AngleDegrees": float(np.percentile(angle, 95)),
            "islandInteriorP95AngleDegrees": (float(np.percentile(angle[interior], 95))
                                                  if np.any(interior) else None),
            "islandMarginP95AngleDegrees": (float(np.percentile(angle[~interior], 95))
                                                if np.any(~interior) else None),
            "p99AngleDegrees": float(np.percentile(angle, 99)),
            "maximumAngleDegrees": float(angle.max()),
            "fractionOver10Degrees": float((angle > 10).mean()),
            "unpaintedInteriorSamples": unpainted(mip, x, y),
            "mixedIslandTexels": mixed_texels,
            "crossIslandBilinearSamples": int(np.count_nonzero(cross_island)),
            "emptyGutterBilinearSamples": int(np.count_nonzero(empty_weight > 1e-8)),
            "meanEmptyGutterWeight": float(np.mean(empty_weight)),
            "p95EmptyGutterWeight": float(np.percentile(empty_weight, 95)),
            "maximumEmptyGutterWeight": float(np.max(empty_weight)),
        })
    specialist = {}
    scalar_maps = ("ambient-occlusion", "contact-occlusion", "signed-curvature",
                   "cavity", "convex-edge", "crease-distance-mask", "thickness",
                   "signed-height")
    for name in scalar_maps + ("region-id",):
        image = np.asarray(Image.open(folder / f"{name}.png").convert("RGBA"))
        if image.shape != (resolution, resolution, 4):
            raise ValueError(f"invalid {name} dimensions: {folder}")
        x, y = coordinates(resolution)
        base_values = (sample_region(image, x, y) if name == "region-id" else
                       sample_scalar(image, x, y))
        levels = []
        for level in range(1, mip_count):
            size = resolution // (2**level)
            mip = np.asarray(Image.open(folder / f"{name}-mip-{level}.png").convert("RGBA"))
            if mip.shape != (size, size, 4):
                raise ValueError(f"invalid {name} mip dimensions: {folder} level {level}")
            x, y = coordinates(size)
            values = (sample_region(mip, x, y) if name == "region-id" else
                      sample_scalar(mip, x, y))
            if name == "region-id":
                levels.append({"mip": level, "regionMismatchFraction":
                               float(np.mean(base_values != values))})
            else:
                difference = np.abs(base_values - values) / 255
                levels.append({"mip": level, "meanAbsoluteDifference": float(np.mean(difference)),
                               "p95AbsoluteDifference": float(np.percentile(difference, 95)),
                               "islandInteriorP95Difference":
                               (float(np.percentile(difference[interior], 95))
                                if np.any(interior) else None),
                               "islandMarginP95Difference":
                               (float(np.percentile(difference[~interior], 95))
                                if np.any(~interior) else None)})
        specialist[name] = levels
    return {"family": folder.parent.name, "lod": folder.name,
            "lowTriangles": len(face), "sampledUvs": len(points),
            "islandInteriorSamples": int(np.count_nonzero(interior)),
            "islandMarginSamples": int(np.count_nonzero(~interior)),
            "baseUnpaintedInteriorSamples": base_holes, "mips": results,
            "specialistMips": specialist}


if __name__ == "__main__":
    if len(sys.argv) != 3:
        raise SystemExit("usage: BakeMipAudit.py <bake-root> <output-json>")
    root = Path(sys.argv[1])
    auto_folders = sorted(root.glob("rock-*/lod-*/bake.json"))
    cases = ([audit_auto_uv(path.parent) for path in auto_folders] if auto_folders else
             [audit(folder) for folder in sorted(root.glob("normal-bake-*")) if folder.is_dir()])
    if not cases:
        raise SystemExit(f"no bake folders in {root}")
    report = {
        "scope": ("fixed-UV normal angle, scalar-map difference, region mismatch, and "
                  "inter-island footprint audit from exported automatic-UV mips; "
                  "not moving-runtime or perceptual Beauty"
                  if auto_folders else
                  "fixed-UV angle to base normal after naive 2x2 box mips; "
                  "not GPU/runtime LOD or Beauty"),
        "scriptSha256": hashlib.sha256(Path(__file__).read_bytes()).hexdigest(),
        "automaticUvSamplesPerTriangle": 4 if auto_folders else None,
        "cases": cases,
    }
    Path(sys.argv[2]).write_text(json.dumps(report, indent=2) + "\n", encoding="utf-8")
    if auto_folders and any(result["mixedIslandTexels"] or result["crossIslandBilinearSamples"] or
                            result["emptyGutterBilinearSamples"]
                            for case in cases for result in case["mips"]):
        raise SystemExit("automatic UV mips can sample another island or empty gutter")
    print(json.dumps(cases, indent=2))
