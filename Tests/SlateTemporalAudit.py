"""Compare nearby camera captures against the same high-mesh reference."""

import json
import sys
from pathlib import Path

import numpy as np
from PIL import Image


def image(path: Path) -> np.ndarray:
    pixels = np.asarray(Image.open(path).convert("RGB"), dtype=np.float32)
    assert pixels.shape == (960, 1280, 3), path
    return pixels


def interior(mask: np.ndarray) -> np.ndarray:
    result = mask.copy()
    for step in (1, 2):
        result[step:] &= mask[:-step]
        result[:-step] &= mask[step:]
        result[:, step:] &= mask[:, :-step]
        result[:, :-step] &= mask[:, step:]
    return result


def audit(root: Path) -> dict:
    proof = json.loads((root / "slate-temporal-proof.json").read_text())
    assert proof["imageSize"] == [1280, 960]
    assert proof["textureResolution"] == 2048
    assert proof["normalMipLevels"] == 4
    assert proof["misses"] == proof["backfaces"] == 0
    assert len(proof["yawOffsets"]) == 5
    material_mask = [interior(image(root / f"high-material-{i}.png").mean(axis=2) > 30)
                     for i in range(5)]
    result = {"proof": proof, "modes": {}}
    for mode in ("material", "normal"):
        high = [image(root / f"high-{mode}-{i}.png") for i in range(5)]
        variants = {}
        for name in ("low-mips", "low-no-normal-mips"):
            low = [image(root / f"{name}-{mode}-{i}.png") for i in range(5)]
            spatial = [float(np.abs(a - b)[mask].mean())
                       for a, b, mask in zip(high, low, material_mask)]
            residual = []
            change = []
            for i in range(4):
                mask = material_mask[i] & material_mask[i + 1]
                assert np.count_nonzero(mask) > 1000
                high_delta = high[i + 1] - high[i]
                low_delta = low[i + 1] - low[i]
                residual.append(float(np.abs(low_delta - high_delta)[mask].mean()))
                change.append(float(np.abs(low_delta)[mask].mean()))
            variants[name] = {"meanSpatialMae": float(np.mean(spatial)),
                              "meanTemporalResidualMae": float(np.mean(residual)),
                              "meanTemporalChangeMae": float(np.mean(change)),
                              "pairResidualMae": residual}
        result["modes"][mode] = variants
    return result


if __name__ == "__main__":
    report = audit(Path(sys.argv[1]))
    Path(sys.argv[2]).write_text(json.dumps(report, indent=2) + "\n")
    for mode, variants in report["modes"].items():
        print(mode, *(f"{name}={values['meanTemporalResidualMae']:.4f}"
                      for name, values in variants.items()))
