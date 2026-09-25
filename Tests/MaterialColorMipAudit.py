"""Compare the same layered material with and without linear-light diffuse mips."""

import json
import sys
from pathlib import Path

import numpy as np
from PIL import Image


def audit(unfiltered: Path, filtered: Path) -> dict:
    result = {"views": []}
    proofs = [json.loads((root / "rock-lod-bakes-proof.json").read_text())
              for root in (unfiltered, filtered)]
    assert all(proof["layeredMaterial"] for proof in proofs)
    assert [proof["layeredMaterialNoMips"] for proof in proofs] == [True, False]
    assert all(not proof["layeredOcclusion"] for proof in proofs)
    background = np.array([14, 17, 22], dtype=np.int16)
    for lod, name in ((0, "low-material.png"), (2, "low-material-far.png")):
        folders = [root / "rock-11" / f"lod-{lod}" for root in (unfiltered, filtered)]
        metadata = [json.loads((folder / "bake.json").read_text()) for folder in folders]
        assert metadata[0]["highHash"] == metadata[1]["highHash"]
        assert metadata[0]["lowHash"] == metadata[1]["lowHash"]
        maps = ("normal", "ambient-occlusion", "contact-occlusion", "signed-curvature",
                "cavity", "convex-edge", "crease-distance-mask", "thickness",
                "region-id", "signed-height")
        baked_files = ["low.obj"] + [f"{name}.png" for name in maps] + [
            f"{name}-mip-{level}.png" for name in maps for level in range(1, 4)]
        for baked_map in baked_files:
            assert (folders[0] / baked_map).read_bytes() == (folders[1] / baked_map).read_bytes()
        pixels = [np.asarray(Image.open(folder / name).convert("RGB"), dtype=np.int16)
                  for folder in folders]
        assert pixels[0].shape == pixels[1].shape == (960, 1280, 3)
        masks = [np.max(np.abs(pix - background), axis=2) > 24 for pix in pixels]
        union = masks[0] | masks[1]
        common = masks[0] & masks[1]
        iou = float(common.sum() / union.sum())
        assert common.sum() > 10000 and iou > 0.999
        neighbor = common[:, :-1] & common[:, 1:]
        energy = [float(np.abs(pix[:, :-1] - pix[:, 1:])[neighbor].mean())
                  for pix in pixels]
        assert energy[1] < 0.8 * energy[0], "diffuse mips did not reduce visible alias detail"
        result["views"].append({"lod": lod, "capture": name,
                                "foregroundPixels": int(common.sum()),
                                "silhouetteIoU": round(iou, 5),
                                "neighborRgbDeltaUnfiltered": round(energy[0], 5),
                                "neighborRgbDeltaFiltered": round(energy[1], 5),
                                "foregroundRgbMae": round(
                                    float(np.abs(pixels[0][common] - pixels[1][common]).mean()), 5)})
    return result


if __name__ == "__main__":
    report = audit(Path(sys.argv[1]), Path(sys.argv[2]))
    Path(sys.argv[3]).write_text(json.dumps(report, indent=2) + "\n")
    print("PASS: matched color-mip views reduce high-frequency image variation")
