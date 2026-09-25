"""Check matched original-resolution material-preview captures for layered bake variants."""

import hashlib
import json
import sys
from pathlib import Path

import numpy as np
from PIL import Image


def audit(baseline: Path, candidate: Path) -> dict:
    references = json.loads((baseline.parent / "cc0-references" / "manifest.json").read_text())
    diffuse = next(entry for entry in references
                   if entry["asset"] == "rock_06" and entry["map"] == "Diffuse")
    texture = baseline.parent / "cc0-references" / "rock_06-diff-1k.jpg"
    assert diffuse["license"] == "CC0"
    assert texture.stat().st_size == diffuse["size"]
    assert hashlib.sha256(texture.read_bytes()).hexdigest() == diffuse["sha256"]
    proofs = [json.loads((root / "rock-lod-bakes-proof.json").read_text())
              for root in (baseline, candidate)]
    assert all(proof["layeredMaterial"] is True for proof in proofs)
    assert [proof["layeredOcclusion"] for proof in proofs] == [False, True]
    assert all([case["family"] for case in proof["cases"]] == [11] for proof in proofs)
    result = {"diffuseSha256": diffuse["sha256"], "lods": []}
    for lod in range(3):
        folders = [root / "rock-11" / f"lod-{lod}" for root in (baseline, candidate)]
        metadata = [json.loads((folder / "bake.json").read_text()) for folder in folders]
        assert metadata[0]["highHash"] == metadata[1]["highHash"]
        assert metadata[0]["lowHash"] == metadata[1]["lowHash"]
        assert metadata[0]["misses"] == metadata[1]["misses"] == 0
        assert (folders[0] / "high-material.png").read_bytes() == \
            (folders[1] / "high-material.png").read_bytes()
        views = ["low-material.png"] + (["low-material-far.png"] if lod == 2 else [])
        row = {"lod": lod, "views": {}}
        for name in views:
            pixels = [np.asarray(Image.open(folder / name).convert("RGB"), dtype=np.int16)
                      for folder in folders]
            assert pixels[0].shape == pixels[1].shape == (960, 1280, 3)
            background = np.array([14, 17, 22], dtype=np.int16)
            masks = [np.max(np.abs(pix - background), axis=2) > 24 for pix in pixels]
            union = masks[0] | masks[1]
            intersection = masks[0] & masks[1]
            assert union.sum() > 10000
            iou = float(intersection.sum() / union.sum())
            assert iou > 0.98, f"LOD{lod} material variants changed silhouette"
            rgb_mae = float(np.abs(pixels[0][intersection] - pixels[1][intersection]).mean())
            assert rgb_mae > 0.05, f"LOD{lod} material variant is visually identical"
            row["views"][name] = {"foregroundPixels": int(intersection.sum()),
                                   "silhouetteIoU": round(iou, 5),
                                   "foregroundRgbMae": round(rgb_mae, 5)}
            high = np.asarray(Image.open(folders[0] / "high-material.png").convert("RGB"),
                              dtype=np.int16)
            if name == "low-material.png":
                high_foreground = np.max(np.abs(high - background), axis=2) > 24
                common = high_foreground & intersection
                assert common.sum() > 10000
                row["views"][name]["highToLowRgbMaeBaseline"] = round(
                    float(np.abs(high[common] - pixels[0][common]).mean()), 5)
                row["views"][name]["highToLowRgbMaeCandidate"] = round(
                    float(np.abs(high[common] - pixels[1][common]).mean()), 5)
        result["lods"].append(row)
    return result


if __name__ == "__main__":
    report = audit(Path(sys.argv[1]), Path(sys.argv[2]))
    Path(sys.argv[3]).write_text(json.dumps(report, indent=2) + "\n")
    print("PASS: three matched layered LOD material captures, including far LOD2")
