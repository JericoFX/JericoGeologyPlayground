"""Measure high-to-low moving-camera fidelity, not only normal-map smoothing."""

import hashlib
import json
import sys
from pathlib import Path

import numpy as np
from PIL import Image


OFFSETS = (-0.001, -0.0005, 0.0, 0.0005, 0.001)
MAPS = ("normal", "ambient-occlusion", "contact-occlusion", "signed-curvature",
        "cavity", "convex-edge", "crease-distance-mask", "thickness", "region-id",
        "signed-height")


def pixels(path: Path) -> np.ndarray:
    result = np.asarray(Image.open(path).convert("RGB"), dtype=np.float32)
    assert result.shape == (960, 1280, 3), path
    return result


def summary(values: list[np.ndarray]) -> dict:
    flat = np.concatenate([value.ravel() for value in values])
    return {"mean": float(flat.mean()), "p95": float(np.percentile(flat, 95)),
            "p99": float(np.percentile(flat, 99))}


def normal(rgb: np.ndarray) -> np.ndarray:
    value = rgb * (2.0 / 255) - 1.0
    return value / np.maximum(np.linalg.norm(value, axis=2, keepdims=True), 1e-8)


def angle(a: np.ndarray, b: np.ndarray) -> np.ndarray:
    return np.degrees(np.arccos(np.clip(np.sum(a * b, axis=2), -1.0, 1.0)))


def interior(mask: np.ndarray) -> np.ndarray:
    result = mask.copy()
    for step in (1, 2):
        result[step:] &= mask[:-step]
        result[:-step] &= mask[step:]
        result[:, step:] &= mask[:, :-step]
        result[:, :-step] &= mask[:, step:]
    return result


def audit(root: Path, baseline: Path) -> dict:
    proof = json.loads((root / "rock-temporal-proof.json").read_text())
    bake = json.loads((baseline / "bake.json").read_text())
    assert proof["imageSize"] == [1280, 960] and proof["textureResolution"] == 2048
    assert np.allclose(proof["yawOffsets"], OFFSETS)
    assert proof["normalMipLevels"] == 4 and proof["misses"] == proof["backfaces"] == 0
    assert proof["highHash"] == bake["highHash"] and proof["lowHash"] == bake["lowHash"]
    for name in MAPS:
        filename = name + ".png"
        assert hashlib.sha256((root / "bake" / filename).read_bytes()).digest() == \
               hashlib.sha256((baseline / filename).read_bytes()).digest(), filename
    masks = [interior(pixels(root / f"high-material-{i}.png").mean(axis=2) > 30)
             for i in range(5)]
    assert all(np.count_nonzero(mask) > 10000 for mask in masks)
    modes = {}
    for mode in ("normal", "material"):
        high = [pixels(root / f"high-{mode}-{i}.png") for i in range(5)]
        if mode == "normal":
            high = [normal(frame) for frame in high]
        variants = {}
        for variant in ("low-mips", "low-no-normal-mips"):
            low = [pixels(root / f"{variant}-{mode}-{i}.png") for i in range(5)]
            if mode == "normal":
                low = [normal(frame) for frame in low]
            spatial, change, residual, high_detail, low_detail = [], [], [], [], []
            for index in range(5):
                mask = masks[index]
                if mode == "normal":
                    spatial.append(angle(high[index], low[index])[mask])
                    edges = mask[:, 1:] & mask[:, :-1]
                    high_detail.append(angle(high[index][:, 1:], high[index][:, :-1])[edges])
                    low_detail.append(angle(low[index][:, 1:], low[index][:, :-1])[edges])
                else:
                    spatial.append(np.abs(high[index] - low[index]).mean(axis=2)[mask])
                if index == 4:
                    continue
                shared = mask & masks[index + 1]
                if mode == "normal":
                    high_change = angle(high[index], high[index + 1])
                    low_change = angle(low[index], low[index + 1])
                else:
                    high_change = np.abs(high[index + 1] - high[index]).mean(axis=2)
                    low_change = np.abs(low[index + 1] - low[index]).mean(axis=2)
                change.append(low_change[shared])
                residual.append(np.abs(low_change - high_change)[shared])
            entry = {"spatial": summary(spatial), "temporalChange": summary(change),
                     "temporalResidual": summary(residual)}
            if mode == "normal":
                entry["neighborDetail"] = {"high": summary(high_detail),
                                           "low": summary(low_detail),
                                           "meanRetention": float(summary(low_detail)["mean"] /
                                                                  summary(high_detail)["mean"])}
            variants[variant] = entry
        modes[mode] = variants
    return {"family": proof["family"], "referenceMapsExact": list(MAPS), "modes": modes,
            "limit": "Five tiny yaw steps and rasterized high are diagnostic; these metrics do "
                     "not establish temporal Beauty or justify a new filter by themselves."}


if __name__ == "__main__":
    result = audit(Path(sys.argv[1]), Path(sys.argv[2]))
    Path(sys.argv[3]).write_text(json.dumps(result, indent=2) + "\n")
    print(f"PASS: family {result['family']} temporal capture and ten maps audited")
