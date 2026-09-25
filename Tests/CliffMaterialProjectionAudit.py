"""Compare high and baked-low cliff material in the authored cut-loop view."""

import sys
from pathlib import Path

import numpy as np
from PIL import Image


def rgb(path: Path) -> np.ndarray:
    image = np.asarray(Image.open(path).convert("RGB"), dtype=np.float32)
    assert image.shape == (960, 1280, 3)
    return image


if __name__ == "__main__":
    root = Path(sys.argv[1]) / "rock-8"
    cut = np.s_[320:620, 580:910]
    for lod in range(3):
        directory = root / f"lod-{lod}"
        high = rgb(directory / "high-material.png")
        low = rgb(directory / "low-material.png")
        error = float(np.abs(high[cut] - low[cut]).mean())
        print(f"cliff LOD{lod} cut material RGB MAE: {error:.3f}; limit 4.5")
        assert error < 4.5, "baked low material still exposes low-geometry facets at the cut"
