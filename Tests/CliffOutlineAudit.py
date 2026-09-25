"""Compare a signed cliff rim against the same-camera inward-only baseline."""

import hashlib
import json
import sys
from pathlib import Path

import numpy as np
from PIL import Image


def mask(path: Path) -> np.ndarray:
    pixels = np.asarray(Image.open(path).convert("RGB"))
    assert pixels.shape == (960, 1280, 3), path
    return np.max(pixels, axis=2) > 70


def edges(shape: np.ndarray) -> tuple[np.ndarray, np.ndarray]:
    valid = shape.any(axis=1)
    left = np.argmax(shape[valid], axis=1)
    right = shape.shape[1] - 1 - np.argmax(shape[valid, ::-1], axis=1)
    return left, right


def audit(baseline: Path, candidate: Path) -> dict:
    base_path = baseline / "rock-8/lod-0/high-material.png"
    high_path = candidate / "rock-8/lod-0/high-material.png"
    low_path = candidate / "rock-8/lod-2/low-material.png"
    base, high, low = map(mask, (base_path, high_path, low_path))
    common_rows = base.any(axis=1) & high.any(axis=1)
    base_left, base_right = edges(base[common_rows])
    high_left, high_right = edges(high[common_rows])
    left_shift, right_shift = high_left - base_left, high_right - base_right
    inward = int(max(left_shift.max(), -right_shift.min()))
    outward = int(max(-left_shift.min(), right_shift.max()))
    changed = int(np.count_nonzero(base ^ high))
    center_changed = int(np.count_nonzero((base ^ high)[350:560, 450:800]))
    low_iou = float(np.count_nonzero(high & low) / np.count_nonzero(high | low))
    assert changed > 10000 and inward > 3 and outward > 3
    assert center_changed == 0 and low_iou > 0.98
    return {"changedSilhouettePixels": changed,
            "silhouetteIouAgainstBaseline": float(np.count_nonzero(base & high) /
                                                  np.count_nonzero(base | high)),
            "largestInwardEdgeShiftPixels": inward,
            "largestOutwardEdgeShiftPixels": outward,
            "centerRoiChangedMaskPixels": center_changed,
            "lod2LowVsHighSilhouetteIou": low_iou,
            "imageSha256": {"baselineHigh": hashlib.sha256(base_path.read_bytes()).hexdigest(),
                            "candidateHigh": hashlib.sha256(high_path.read_bytes()).hexdigest(),
                            "candidateLow": hashlib.sha256(low_path.read_bytes()).hexdigest()},
            "limit": "Silhouette change and high-low agreement do not prove geological Beauty."}


if __name__ == "__main__":
    result = audit(Path(sys.argv[1]), Path(sys.argv[2]))
    Path(sys.argv[3]).write_text(json.dumps(result, indent=2) + "\n")
    print(f"PASS: {result['changedSilhouettePixels']} silhouette pixels changed")
