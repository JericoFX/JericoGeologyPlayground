"""Compare same-camera BSP cave silhouettes at original capture resolution."""

import hashlib
import json
import sys
from pathlib import Path

import numpy as np
from PIL import Image


def mask(path: Path) -> np.ndarray:
    rgb = np.asarray(Image.open(path).convert("RGB"))
    assert rgb.shape == (960, 1280, 3)
    return rgb.mean(axis=2) > 40


def compare(left: Path, right: Path) -> dict:
    a, b = mask(left), mask(right)
    return {"silhouetteIou": float((a & b).sum() / (a | b).sum()),
            "changedMaskPixels": int((a ^ b).sum())}


if __name__ == "__main__":
    control, candidate = Path(sys.argv[1]), Path(sys.argv[2])
    result = {"front": compare(control / "arch/high-material.png",
                               candidate / "arch/high-material.png"),
              "oblique": compare(control / "arch/high-oblique-material.png",
                                 candidate / "arch/high-oblique-material.png"),
              "squareControlIdentical": all(
                  hashlib.sha256((control / "square" / name).read_bytes()).digest() ==
                  hashlib.sha256((candidate / "square" / name).read_bytes()).digest()
                  for name in ("high-material.png", "lod-2/low-material.png")),
              "limit": "Silhouette change is not proof of natural-cave Beauty."}
    assert result["squareControlIdentical"]
    Path(sys.argv[3]).write_text(json.dumps(result, indent=2) + "\n")
    print("PASS: compared two arch views; square control unchanged")
