"""Check blind-arch visibility against a same-camera through-tunnel control."""

import json
import math
import sys
from pathlib import Path

import numpy as np
from PIL import Image


def pixels(path: Path) -> np.ndarray:
    result = np.asarray(Image.open(path).convert("RGB"), dtype=np.float32)
    assert result.shape == (960, 1280, 3), path
    return result


def audit(root: Path, through: Path) -> dict:
    proof = json.loads((root / "cave-forms-proof.json").read_text())
    control = json.loads((through / "cave-forms-proof.json").read_text())
    assert proof["blindChamber"] and proof["axialFeatureReduced"]
    assert abs(abs(proof["entranceCamera"]["yaw"]) - math.pi) < 1e-5
    arch, square = proof["forms"]
    assert [arch["form"], square["form"]] == ["arch", "square"]
    assert arch["highHash"] != control["forms"][0]["highHash"]
    assert square["highHash"] == control["forms"][1]["highHash"]
    assert [item["lowHash"] for item in square["lods"]] == [
        item["lowHash"] for item in control["forms"][1]["lods"]]
    entrance = pixels(root / "arch/high-entrance-material.png")
    square_entrance = pixels(root / "square/high-entrance-material.png")
    low_entrance = pixels(root / "arch/lod-2/low-entrance-material.png")
    interior = pixels(root / "arch/high-interior-material.png")
    low_interior = pixels(root / "arch/lod-2/low-interior-material.png")
    center = np.s_[350:650, 500:780]
    chamber_mean = float(entrance[center].mean())
    square_mean = float(square_entrance[center].mean())
    entrance_mae = float(np.abs(entrance - low_entrance).mean())
    interior_mae = float(np.abs(interior - low_interior).mean())
    assert chamber_mean > square_mean + 40
    assert entrance_mae < 15 and interior_mae < 15
    return {"sameCameraCenterRoi": {"archMeanRgb": chamber_mean,
                                     "squareMeanRgb": square_mean},
            "highVsLod2RgbMae": {"entrance": entrance_mae, "interior": interior_mae},
            "squareGeometryAndLodsUnchanged": True,
            "limit": "Static color and center-wall visibility do not prove natural-cave Beauty, "
                     "runtime lighting, or collision/navigation."}


if __name__ == "__main__":
    result = audit(Path(sys.argv[1]), Path(sys.argv[2]))
    Path(sys.argv[3]).write_text(json.dumps(result, indent=2) + "\n")
    print("PASS: blind end wall visible; square geometry unchanged")
