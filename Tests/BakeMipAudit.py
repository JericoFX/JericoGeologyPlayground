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
    weights = np.asarray([[1 / 3, 1 / 3, 1 / 3], [0.75, 0.125, 0.125],
                          [0.125, 0.75, 0.125], [0.125, 0.125, 0.75]])
    points = np.einsum("sc,fck->fsk", weights, uv[face]).reshape(-1, 2)
    base = np.asarray(Image.open(folder / "normal.png").convert("RGBA"), dtype=np.float32)
    if base.shape != (resolution, resolution, 4):
        raise ValueError(f"invalid automatic UV normal dimensions: {folder}")

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
        mip = np.asarray(Image.open(folder / f"normal-mip-{level}.png").convert("RGBA"),
                         dtype=np.float32)
        if mip.shape != (size, size, 4):
            raise ValueError(f"invalid automatic UV mip dimensions: {folder} level {level}")
        x, y = coordinates(size)
        filtered = sample(mip, x, y)
        angle = np.degrees(np.arccos(np.clip((original * filtered).sum(axis=1), -1, 1)))
        results.append({
            "mip": level,
            "medianAngleDegrees": float(np.median(angle)),
            "p95AngleDegrees": float(np.percentile(angle, 95)),
            "p99AngleDegrees": float(np.percentile(angle, 99)),
            "maximumAngleDegrees": float(angle.max()),
            "fractionOver10Degrees": float((angle > 10).mean()),
            "unpaintedInteriorSamples": unpainted(mip, x, y),
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
                               "p95AbsoluteDifference": float(np.percentile(difference, 95))})
        specialist[name] = levels
    return {"family": folder.parent.name, "lod": folder.name,
            "lowTriangles": len(face), "sampledUvs": len(points),
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
        "scope": ("fixed-UV normal angle, scalar-map difference, and region mismatch "
                  "from exported automatic-UV mips; not moving-runtime or perceptual Beauty"
                  if auto_folders else
                  "fixed-UV angle to base normal after naive 2x2 box mips; "
                  "not GPU/runtime LOD or Beauty"),
        "scriptSha256": hashlib.sha256(Path(__file__).read_bytes()).hexdigest(),
        "automaticUvSamplesPerTriangle": 4 if auto_folders else None,
        "cases": cases,
    }
    Path(sys.argv[2]).write_text(json.dumps(report, indent=2) + "\n", encoding="utf-8")
    print(json.dumps(cases, indent=2))
