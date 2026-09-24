"""Diagnostic screen-space comparison of captured High, Low and Low+normal images."""

import json
import hashlib
import sys
from pathlib import Path

import numpy as np
from PIL import __version__ as pillow_version
from PIL import Image


def rgb(path: Path) -> np.ndarray:
    return np.asarray(Image.open(path).convert("RGB"), dtype=np.float32)


def main(root: Path) -> None:
    cases = []
    for folder in sorted(root.glob("normal-bake-*")):
        if not folder.is_dir():
            continue
        parts = folder.name.split("-")
        family = int(parts[2])
        seed = int(parts[4]) if len(parts) == 5 and parts[3] == "seed" else 123
        view = int(parts[4]) if len(parts) == 5 and parts[3] == "yaw" else None
        high, low, baked = (rgb(folder / f"{name}.png") for name in
                            ("high", "low", "low-normal"))
        if high.shape != (960, 1280, 3) or low.shape != high.shape or baked.shape != high.shape:
            raise ValueError(f"family {family}: capture dimensions differ")
        high_mask, low_mask, baked_mask = (image.mean(2) > 40 for image in
                                           (high, low, baked))
        common = high_mask & low_mask & baked_mask
        if not common.any():
            raise ValueError(f"family {family}: no common foreground")
        low_error = np.abs(high - low).mean(2)
        baked_error = np.abs(high - baked).mean(2)
        cases.append({
            "family": family,
            "seed": seed,
            "viewIndex": view,
            "captureSha256": {name: hashlib.sha256((folder / f"{name}.png").read_bytes()).hexdigest()
                              for name in ("high", "low", "low-normal")},
            "commonPixels": int(common.sum()),
            "silhouetteIouLow": float((high_mask & low_mask).sum() /
                                      (high_mask | low_mask).sum()),
            "silhouetteIouBaked": float((high_mask & baked_mask).sum() /
                                        (high_mask | baked_mask).sum()),
            "meanRgbAbsErrorLow": float(low_error[common].mean()),
            "meanRgbAbsErrorBaked": float(baked_error[common].mean()),
            "fractionPixelsImproved": float((baked_error[common] < low_error[common]).mean()),
        })
    print(json.dumps({
        "metric": "same-camera 1280x960 foreground RGB MAE; threshold mean RGB > 40",
        "limit": "single-camera screen-space diagnostic, not perceptual or M8 Beauty acceptance",
        "scriptSha256": hashlib.sha256(Path(__file__).read_bytes()).hexdigest(),
        "numpyVersion": np.__version__,
        "pillowVersion": pillow_version,
        "cases": cases,
    }, indent=2))


if __name__ == "__main__":
    if len(sys.argv) != 2:
        raise SystemExit("usage: BakeVisualAudit.py <normal-bake-evidence-directory>")
    main(Path(sys.argv[1]))
