"""Measure silhouette changes in the native angular-rock profile captures."""

import hashlib
import json
import sys
from pathlib import Path

import numpy as np
from PIL import Image


def silhouette(path):
    image = np.asarray(Image.open(path).convert("RGB"), dtype=np.uint8)
    if image.shape != (960, 1280, 3):
        raise ValueError(f"unexpected capture resolution: {path}")
    mask = image.mean(axis=2) > 40
    if not mask.any():
        raise ValueError(f"no visible rock: {path}")
    return mask


def perimeter(mask):
    return int(np.count_nonzero(mask[:, 1:] != mask[:, :-1]) +
               np.count_nonzero(mask[1:, :] != mask[:-1, :]))


def audit(folder):
    results = []
    for view in (0, 1):
        baseline = silhouette(folder / f"baseline-yaw-{view}.png")
        for name in ("baseline", "more-cuts", "cross-cuts"):
            path = folder / f"{name}-yaw-{view}.png"
            mask = silhouette(path)
            union = int(np.count_nonzero(baseline | mask))
            intersection = int(np.count_nonzero(baseline & mask))
            results.append({"view": view, "name": name,
                            "captureSha256": hashlib.sha256(path.read_bytes()).hexdigest(),
                            "foregroundPixels": int(mask.sum()),
                            "silhouetteEdgeTransitions": perimeter(mask),
                            "iouWithBaseline": intersection / union})
    return {"scope": "binary 1280x960 silhouette diagnostic; shape difference is not rock realism",
            "scriptSha256": hashlib.sha256(Path(__file__).read_bytes()).hexdigest(),
            "cases": results}


if __name__ == "__main__":
    if len(sys.argv) != 3:
        raise SystemExit("usage: AngularProfilesAudit.py <capture-directory> <output-json>")
    report = audit(Path(sys.argv[1]))
    Path(sys.argv[2]).write_text(json.dumps(report, indent=2) + "\n", encoding="utf-8")
    print(json.dumps(report["cases"], indent=2))
