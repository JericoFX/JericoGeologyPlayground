"""Measure whether exported UV interiors actually sample painted normal texels."""

import argparse
import json
from pathlib import Path

import numpy as np
from PIL import Image


def audit(directory: Path, require_specialist_mips: bool = False) -> dict:
    uvs = []
    faces = []
    with (directory / "low.obj").open(encoding="utf-8") as stream:
        for line in stream:
            if line.startswith("vt "):
                uvs.append([float(value) for value in line.split()[1:3]])
            elif line.startswith("f "):
                faces.append([int(value.split("/")[1]) - 1 for value in line.split()[1:4]])
    uv = np.asarray(uvs, dtype=np.float64)
    face = np.asarray(faces, dtype=np.int64)
    image = np.asarray(Image.open(directory / "normal.png").convert("RGBA"))
    rng = np.random.default_rng(42)
    weights = rng.dirichlet([1, 1, 1], size=(len(face), 32))
    points = np.einsum("fsc,fck->fsk", weights, uv[face])
    x = np.clip((points[:, :, 0] * image.shape[1]).astype(int), 0, image.shape[1] - 1)
    y = np.clip(((1 - points[:, :, 1]) * image.shape[0]).astype(int), 0, image.shape[0] - 1)
    sampled = image[y, x]
    holes_per_triangle = np.count_nonzero(sampled[:, :, 3] == 0, axis=1)
    worst = np.argsort(holes_per_triangle)[-8:][::-1]
    pixel_points = uv[face] * np.array([image.shape[1], image.shape[0]])
    edge1 = pixel_points[:, 1] - pixel_points[:, 0]
    edge2 = pixel_points[:, 2] - pixel_points[:, 0]
    twice_area = np.abs(edge1[:, 0] * edge2[:, 1] - edge1[:, 1] * edge2[:, 0])
    channels = ["ambient-occlusion", "contact-occlusion", "signed-curvature", "cavity",
                "convex-edge", "crease-distance-mask", "thickness", "region-id", "signed-height"]
    alpha_mismatches = {}
    for name in channels:
        other = np.asarray(Image.open(directory / f"{name}.png").convert("RGBA"))
        if other.shape != image.shape:
            raise ValueError(f"{name} dimensions differ from normal map")
        alpha_mismatches[name] = int(np.count_nonzero(other[:, :, 3] != image[:, :, 3]))
    mip_dimensions = []
    specialist_mip_alpha_mismatches = {}
    fabricated_region_ids = 0
    mask_peak_by_level = {"cavity": [], "convex-edge": []}
    if require_specialist_mips:
        metadata = json.loads((directory / "bake.json").read_text())
        if metadata.get("specialistMipLevels") != metadata.get("normalMipLevels"):
            raise ValueError("specialist mip count differs from normal mip count")
        base_regions = np.asarray(Image.open(directory / "region-id.png").convert("RGBA"))
        encoded_base = ((base_regions[:, :, 0].astype(np.uint32) << 16) |
                        (base_regions[:, :, 1].astype(np.uint32) << 8) |
                        base_regions[:, :, 2].astype(np.uint32))
        allowed_region_ids = np.unique(encoded_base[base_regions[:, :, 3] > 0])
    for level in range(1, 4):
        normal_mip = np.asarray(Image.open(directory / f"normal-mip-{level}.png").convert("RGBA"))
        mip_dimensions.append([normal_mip.shape[1], normal_mip.shape[0]])
        if require_specialist_mips:
            expected_size = image.shape[0] >> level
            if normal_mip.shape != (expected_size, expected_size, 4):
                raise ValueError(f"normal mip {level} dimensions are wrong")
            for name in channels:
                specialist = np.asarray(Image.open(directory / f"{name}-mip-{level}.png").convert("RGBA"))
                if specialist.shape != normal_mip.shape:
                    raise ValueError(f"{name} mip {level} dimensions are wrong")
                mismatch = int(np.count_nonzero(specialist[:, :, 3] != normal_mip[:, :, 3]))
                specialist_mip_alpha_mismatches[f"{name}:{level}"] = mismatch
                if name == "region-id":
                    encoded = ((specialist[:, :, 0].astype(np.uint32) << 16) |
                               (specialist[:, :, 1].astype(np.uint32) << 8) |
                               specialist[:, :, 2].astype(np.uint32))
                    active_ids = np.unique(encoded[specialist[:, :, 3] > 0])
                    fabricated_region_ids += len(np.setdiff1d(active_ids, allowed_region_ids))
                else:
                    valid = specialist[:, :, 3] > 0
                    if np.any(specialist[:, :, 0][valid] != specialist[:, :, 1][valid]) or \
                       np.any(specialist[:, :, 0][valid] != specialist[:, :, 2][valid]):
                        raise ValueError(f"{name} mip {level} is not scalar grayscale")
                    if name in mask_peak_by_level:
                        mask_peak_by_level[name].append(int(specialist[:, :, 0][valid].max()) if valid.any() else 0)
    return {
        "triangles": len(face),
        "samples": sampled.shape[0] * sampled.shape[1],
        "unpaintedInteriorSamples": int(np.count_nonzero(sampled[:, :, 3] == 0)),
        "worstUnpaintedTriangles": [
            {"triangle": int(index), "holes": int(holes_per_triangle[index]),
             "uvPixelArea": float(twice_area[index] * 0.5)}
            for index in worst if holes_per_triangle[index]
        ],
        "blueBelow64Samples": int(np.count_nonzero(sampled[:, :, 2] < 64)),
        "specialistAlphaMismatches": alpha_mismatches,
        "normalMipDimensions": mip_dimensions,
        "specialistMipAlphaMismatches": specialist_mip_alpha_mismatches,
        "fabricatedRegionIds": fabricated_region_ids,
        "maskPeakByLevel": mask_peak_by_level,
    }


if __name__ == "__main__":
    parser = argparse.ArgumentParser()
    parser.add_argument("directory", type=Path)
    parser.add_argument("--require-covered", action="store_true")
    parser.add_argument("--require-specialist-mips", action="store_true")
    args = parser.parse_args()
    result = audit(args.directory, args.require_specialist_mips)
    print(json.dumps(result, indent=2))
    if args.require_covered and result["unpaintedInteriorSamples"]:
        raise SystemExit("UV interiors sample unpainted texels")
    if args.require_specialist_mips and (any(result["specialistMipAlphaMismatches"].values()) or
                                        result["fabricatedRegionIds"]):
        raise SystemExit("Specialist mip coverage or categorical region identity failed")
