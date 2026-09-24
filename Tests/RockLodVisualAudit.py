"""Measure same-camera high versus baked-low rock LOD captures at original resolution."""

import hashlib
import json
import sys
from pathlib import Path

import numpy as np
from PIL import Image, __version__ as pillow_version


def compare(high_path: Path, low_path: Path) -> dict:
    high = np.asarray(Image.open(high_path).convert("RGB"), dtype=np.float32)
    low = np.asarray(Image.open(low_path).convert("RGB"), dtype=np.float32)
    assert high.shape == low.shape == (960, 1280, 3)
    high_mask = high.mean(axis=2) > 40
    low_mask = low.mean(axis=2) > 40
    common = high_mask & low_mask
    assert common.any()
    return {
        "highSha256": hashlib.sha256(high_path.read_bytes()).hexdigest(),
        "lowSha256": hashlib.sha256(low_path.read_bytes()).hexdigest(),
        "commonPixels": int(common.sum()),
        "silhouetteIou": float((high_mask & low_mask).sum() /
                               (high_mask | low_mask).sum()),
        "foregroundRgbMae": float(np.abs(high - low).mean(axis=2)[common].mean()),
    }


def audit(root: Path) -> dict:
    proof = json.loads((root / "rock-lod-bakes-proof.json").read_text())
    cases = []
    for case in proof["cases"]:
        family = case["family"]
        for lod in case["lods"]:
            if lod["status"] == "REJECTED":
                continue
            directory = root / f"rock-{family}" / f"lod-{lod['lod']}"
            entry = {"family": family, "lod": lod["lod"],
                     "near": compare(directory / "high.png", directory / "low-normal.png")}
            if (directory / "high-far.png").exists() or (directory / "low-normal-far.png").exists():
                entry["far"] = compare(directory / "high-far.png",
                                       directory / "low-normal-far.png")
            cases.append(entry)
    return {
        "metric": "same-camera 1280x960 foreground RGB MAE; threshold mean RGB > 40",
        "limit": "screen-space diagnostic, not perceptual or M8 Beauty acceptance",
        "scriptSha256": hashlib.sha256(Path(__file__).read_bytes()).hexdigest(),
        "numpyVersion": np.__version__, "pillowVersion": pillow_version,
        "cases": cases,
    }


if __name__ == "__main__":
    result = audit(Path(sys.argv[1]))
    Path(sys.argv[2]).write_text(json.dumps(result, indent=2) + "\n")
    print(f"PASS: measured {len(result['cases'])} rock LOD captures")
