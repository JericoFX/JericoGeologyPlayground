"""Compare moving-camera high/low bakes at the same visible surface points."""

import json
import sys
from pathlib import Path

import numpy as np
from PIL import Image


MODES = ("normal", "material")
VARIANTS = ("high", "low-mips", "low-no-normal-mips")


def image(path: Path) -> np.ndarray:
    result = np.asarray(Image.open(path).convert("RGBA"), dtype=np.uint8)
    assert result.shape == (960, 1280, 4), path
    return result


def unpack24(rgb: np.ndarray) -> np.ndarray:
    value = rgb.astype(np.uint32)
    return (value[..., 0] << 16) | (value[..., 1] << 8) | value[..., 2]


def basis(yaw: float, pitch: float) -> tuple[np.ndarray, np.ndarray, np.ndarray]:
    cy, sy, cp, sp = np.cos(yaw), np.sin(yaw), np.cos(pitch), np.sin(pitch)
    right = np.array((cy, 0.0, -sy))
    forward = np.array((sy * cp, sp, cy * cp))
    return right, np.cross(forward, right), forward


def reconstruct(x: np.ndarray, y: np.ndarray, depth: np.ndarray,
                camera: dict) -> np.ndarray:
    right, up, forward = basis(camera["yaw"], camera["pitch"])
    width, height = camera["size"]
    nx = (x + 0.5) * (2.0 / width) - 1.0
    ny = 1.0 - (y + 0.5) * (2.0 / height)
    return (np.asarray(camera["target"]) +
            (nx * depth * camera["aspect"] / camera["scale"])[:, None] * right +
            (ny * depth / camera["scale"])[:, None] * up +
            (depth - camera["distance"])[:, None] * forward)


def project(world: np.ndarray, camera: dict) -> tuple[np.ndarray, np.ndarray, np.ndarray]:
    right, up, forward = basis(camera["yaw"], camera["pitch"])
    q = world - np.asarray(camera["target"])
    depth = q @ forward + camera["distance"]
    width, height = camera["size"]
    x = (q @ right * camera["scale"] / (depth * camera["aspect"]) * 0.5 + 0.5) * width - 0.5
    y = (0.5 - q @ up * camera["scale"] / depth * 0.5) * height - 0.5
    return x, y, depth


def summary(values: list[np.ndarray]) -> dict:
    flat = np.concatenate(values)
    assert flat.size and np.isfinite(flat).all()
    return {"mean": float(flat.mean()), "p95": float(np.percentile(flat, 95)),
            "p99": float(np.percentile(flat, 99))}


def angle(a: np.ndarray, b: np.ndarray) -> np.ndarray:
    return np.degrees(np.arccos(np.clip(np.sum(a * b, axis=-1), -1.0, 1.0)))


def normal(rgb: np.ndarray) -> np.ndarray:
    value = rgb.astype(np.float32) * (2.0 / 255.0) - 1.0
    return value / np.maximum(np.linalg.norm(value, axis=-1, keepdims=True), 1e-8)


def linear(rgb: np.ndarray) -> np.ndarray:
    value = rgb.astype(np.float32) / 255.0
    # The material preview writes pow(linearLighting, 1/2.2) into RGBA8_UNORM.
    return value ** 2.2


def load_frame(root: Path, variant: str, index: int, max_depth: float) -> dict:
    depth_image = image(root / f"{variant}-depth-{index}.png")
    id_image = image(root / f"{variant}-primitive-{index}.png")
    depth = unpack24(depth_image[..., :3]).astype(np.float64) * (max_depth / 16777215.0)
    primitive = unpack24(id_image[..., :3])
    assert np.array_equal(depth_image[..., 3] > 0, id_image[..., 3] > 0)
    assert np.array_equal(depth_image[..., 3] > 0, primitive > 0)
    return {"depth": depth, "primitive": primitive,
            "normal": normal(image(root / f"{variant}-normal-{index}.png")[..., :3]),
            "material": linear(image(root / f"{variant}-material-{index}.png")[..., :3])}


def surface_delta(first: dict, second: dict, camera0: dict, camera1: dict) -> dict:
    occupancy = first["primitive"] > 0
    valid0 = occupancy.copy()
    # Two-pixel silhouette band and image border are excluded before projection.
    for step in (1, 2):
        valid0[step:] &= occupancy[:-step]
        valid0[:-step] &= occupancy[step:]
        valid0[:, step:] &= occupancy[:, :-step]
        valid0[:, :-step] &= occupancy[:, step:]
    ys, xs = np.nonzero(valid0)
    world = reconstruct(xs, ys, first["depth"][ys, xs], camera0)
    x1, y1, projected_depth = project(world, camera1)
    inside = ((x1 >= 2) & (x1 < camera1["size"][0] - 3) &
              (y1 >= 2) & (y1 < camera1["size"][1] - 3) & (projected_depth > 0))
    inside_count = int(inside.sum())
    xs, ys, x1, y1, projected_depth = (value[inside] for value in
                                      (xs, ys, x1, y1, projected_depth))
    ix, iy = np.floor(x1).astype(np.int32), np.floor(y1).astype(np.int32)
    fx, fy = x1 - ix, y1 - iy
    ids = second["primitive"]
    original_ids = first["primitive"][ys, xs]
    nearest_x = np.rint(x1).astype(np.int32)
    nearest_y = np.rint(y1).astype(np.int32)
    same = ids[nearest_y, nearest_x] == original_ids
    same_primitive_count = int(same.sum())
    neighbors = ((iy, ix), (iy, ix + 1), (iy + 1, ix), (iy + 1, ix + 1))
    raw_weights = ((1 - fx) * (1 - fy), fx * (1 - fy), (1 - fx) * fy, fx * fy)
    weights = [weight * (ids[coord] == original_ids) for weight, coord in
               zip(raw_weights, neighbors)]
    weight_sum = sum(weights)
    same &= weight_sum > 0
    weights = [weight / np.maximum(weight_sum, 1e-12) for weight in weights]

    def sample(field: np.ndarray) -> np.ndarray:
        values = (field[coord] for coord in neighbors)
        if field.ndim == 3:
            return sum(weight[:, None] * value for weight, value in zip(weights, values))
        return sum(weight * value for weight, value in zip(weights, values))

    sampled_depth = sample(second["depth"])
    same &= np.abs(sampled_depth - projected_depth) <= 0.02
    xs, ys = xs[same], ys[same]
    normal1 = sample(second["normal"])[same]
    normal1 /= np.maximum(np.linalg.norm(normal1, axis=1, keepdims=True), 1e-8)
    material1 = sample(second["material"])[same]
    shape = first["depth"].shape
    normal_change = np.full(shape, np.nan, dtype=np.float32)
    material_change = np.full(shape, np.nan, dtype=np.float32)
    normal_change[ys, xs] = angle(first["normal"][ys, xs], normal1)
    material_change[ys, xs] = np.abs(first["material"][ys, xs] - material1).mean(axis=1)
    return {"normal": normal_change, "material": material_change,
            "eligible": int(valid0.sum()), "inside": inside_count,
            "samePrimitive": same_primitive_count, "matched": int(same.sum())}


def audit(root: Path, near: bool = False) -> dict:
    proof = json.loads((root / "rock-temporal-proof.json").read_text())
    variants = (VARIANTS + tuple(proof.get("normalMipBiasVariants", {})) +
                tuple(proof.get("normalMomentVariants", ())))
    if near:
        variants = tuple("near-" + variant for variant in variants)
    high_name = variants[0]
    assert proof["reprojectionVersion"] == 1 and proof["depthEncoding"] == "linear-view-24-bit-rgb"
    assert proof["primitiveEncoding"] == "triangle-index-plus-one-24-bit-rgb"
    size = proof["imageSize"]
    assert size == [1280, 960] and proof["projectionScale"] == 2.4
    cameras = [{"yaw": proof["cameraYaw"] + offset, "pitch": proof["cameraPitch"],
                "distance": proof["nearCameraDistance" if near else "cameraDistance"],
                "target": proof["cameraTarget"],
                "size": size, "aspect": size[0] / size[1], "scale": proof["projectionScale"]}
               for offset in proof["yawOffsets"]]
    assert len(cameras) == 5
    frames = {variant: [load_frame(root, variant, i, proof["depthEncodingMaxMeters"])
                        for i in range(5)] for variant in variants}
    stationary = surface_delta(frames[high_name][2], frames[high_name][2], cameras[2], cameras[2])
    valid_normal = stationary["normal"][np.isfinite(stationary["normal"])]
    valid_material = stationary["material"][np.isfinite(stationary["material"])]
    stationary_normal = summary([valid_normal])
    stationary_material = summary([valid_material])
    assert stationary["matched"] >= 0.98 * stationary["eligible"]
    assert stationary_normal["mean"] < 0.02 and stationary_material["mean"] < 1e-6
    temporal = {variant: [surface_delta(frames[variant][i], frames[variant][i + 1],
                                        cameras[i], cameras[i + 1]) for i in range(4)]
                for variant in variants}
    result = {"family": proof["family"], "transitions": 4,
              "cameraDistance": cameras[0]["distance"],
              "stationary": {"eligible": stationary["eligible"],
                             "matched": stationary["matched"],
                             "normalChangeDegrees": stationary_normal,
                             "linearRenderedColorChange": stationary_material},
              "variants": {}}
    for variant in variants:
        sequence = temporal[variant]
        assert all(entry["matched"] >= 10000 and
                   entry["matched"] >= 0.95 * entry["eligible"] for entry in sequence), variant
        result["variants"][variant] = {
            "eligible": sum(entry["eligible"] for entry in sequence),
            "matched": sum(entry["matched"] for entry in sequence),
            "perTransition": [{key: entry[key] for key in
                               ("eligible", "inside", "samePrimitive", "matched")}
                              for entry in sequence],
            "normalChangeDegrees": summary([entry["normal"][np.isfinite(entry["normal"])]
                                             for entry in sequence]),
            "linearRenderedColorChange": summary([entry["material"][np.isfinite(entry["material"])]
                                                   for entry in sequence])}
    result["shared"] = {}
    for variant in variants[1:]:
        paired = []
        for i in range(4):
            high, low = temporal[high_name][i], temporal[variant][i]
            common = np.isfinite(high["normal"]) & np.isfinite(low["normal"])
            common &= np.abs(frames[high_name][i]["depth"] -
                             frames[variant][i]["depth"]) <= 0.05
            assert common.sum() >= 10000, (variant, i)
            paired.append((i, common))
        assert sum(mask.sum() for _, mask in paired) >= 0.90 * min(
            result["variants"][high_name]["eligible"], result["variants"][variant]["eligible"])
        result["shared"][variant] = {
            "pixels": int(sum(mask.sum() for _, mask in paired)),
            "perTransitionNormalResidualDegrees": [
                {"pixels": int(mask.sum()), **summary([
                    np.abs(temporal[variant][i]["normal"][mask] -
                           temporal[high_name][i]["normal"][mask])])}
                for i, mask in paired],
            "normalTemporalResidualDegrees": summary([
                np.abs(temporal[variant][i]["normal"][mask] - temporal[high_name][i]["normal"][mask])
                for i, mask in paired]),
            "linearRenderedColorTemporalResidual": summary([
                np.abs(temporal[variant][i]["material"][mask] - temporal[high_name][i]["material"][mask])
                for i, mask in paired]),
            "normalSpatialDegrees": summary([
                angle(frames[high_name][i]["normal"], frames[variant][i]["normal"])[mask]
                for i, mask in paired]),
            "linearRenderedColorSpatial": summary([
                np.abs(frames[high_name][i]["material"] - frames[variant][i]["material"])
                .mean(axis=2)[mask] for i, mask in paired])}
    result["limit"] = ("Mode-5 color includes lighting and view-dependent specular; visibility "
                       "is matched within each mesh, then high/low depth at the starting pixel. "
                       "Five tiny yaw steps and one light remain a diagnostic, "
                       "not representative Beauty acceptance.")
    return result


if __name__ == "__main__":
    if len(sys.argv) == 2 and sys.argv[1] == "--self-test":
        cam = {"yaw": 0.5, "pitch": -0.28, "distance": 11.6, "target": [0, 0, 0],
               "size": [1280, 960], "aspect": 1280 / 960, "scale": 2.4}
        x, y = np.array([2.0, 620.3, 1275.0]), np.array([4.0, 460.1, 954.0])
        d = np.array([9.0, 11.6, 14.0])
        px, py, pd = project(reconstruct(x, y, d, cam), cam)
        assert np.allclose(px, x) and np.allclose(py, y) and np.allclose(pd, d)
        print("PASS: camera reconstruction and projection roundtrip")
    else:
        result = audit(Path(sys.argv[1]))
        Path(sys.argv[2]).write_text(json.dumps(result, indent=2) + "\n")
        print(f"PASS: family {result['family']} visibility-matched temporal reprojection")
