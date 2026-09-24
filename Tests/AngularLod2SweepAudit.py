"""Audit strict angular LOD2 candidates without accepting incomplete map packages."""

import json
import sys
from pathlib import Path

import numpy as np
from PIL import Image


MAPS = (
    "normal.png", "ambient-occlusion.png", "contact-occlusion.png",
    "signed-curvature.png", "cavity.png", "convex-edge.png",
    "crease-distance-mask.png", "thickness.png", "region-id.png",
    "signed-height.png",
)


def audit(root: Path) -> dict:
    proof = json.loads((root / "angular-lod2-sweep-proof.json").read_text())
    assert proof["lod1Misses"] == 0
    assert [round(c["errorBudget"], 5) for c in proof["candidates"]] == [
        .008, .00825, .0085, .00875, .009, .00925]
    selected = []
    for candidate in proof["candidates"]:
        if candidate["status"] == "REJECTED":
            assert candidate["reason"]
            continue
        assert candidate["lowTriangles"] < proof["lod1Triangles"]
        assert candidate["sampledSurfaceError"] <= candidate["errorBudget"]
        directory = root / f"error-{round(candidate['errorBudget'] * 100000)}"
        metadata = json.loads((directory / "bake.json").read_text())
        assert metadata["highHash"] == proof["highHash"]
        assert metadata["lowHash"] == candidate["lowHash"]
        assert metadata["misses"] == candidate["misses"]
        assert metadata["projectionRadius"] == .15
        assert metadata["lowTriangles"] == candidate["lowTriangles"]
        assert (directory / "low.obj").is_file()
        assert (directory / "high-recipe.json").is_file()
        for name in MAPS:
            image = np.asarray(Image.open(directory / name).convert("RGBA"))
            assert image.shape == (2048, 2048, 4)
            assert np.count_nonzero(image[:, :, 3]) > 0
        for name in ("low-normal.png", "low-ao.png", "low-contact.png",
                     "low-cavity.png", "low-convex-edge.png"):
            assert Image.open(directory / name).size == (1280, 960)
        if candidate["misses"] == 0:
            selected.append({"errorBudget": candidate["errorBudget"],
                             "lowTriangles": candidate["lowTriangles"],
                             "lowHash": candidate["lowHash"]})
    assert selected
    best = min(selected, key=lambda item: item["lowTriangles"])
    assert round(best["errorBudget"], 5) == .008
    return {"zeroMissCandidates": selected, "smallestZeroMissCandidate": best}


if __name__ == "__main__":
    result = audit(Path(sys.argv[1]))
    Path(sys.argv[2]).write_text(json.dumps(result, indent=2) + "\n")
    print("PASS: strict angular LOD2 sweep and ten maps per eligible candidate")
