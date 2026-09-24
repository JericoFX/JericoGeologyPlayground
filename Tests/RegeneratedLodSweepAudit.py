"""Audit regenerated low candidates against geometric and high-to-low bake gates."""

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
    proof = json.loads((root / "regenerated-lod-sweep-proof.json").read_text())
    assert {case["family"] for case in proof["cases"]} == {9, 10, 12}
    counts = {"baked": 0, "partial": 0, "rejected": 0}
    for case in proof["cases"]:
        family = case["family"]
        expected = {9: [32, 24, 20], 10: [16, 12, 8, 6, 4],
                    12: [8, 7, 6, 5, 4]}[family]
        assert [item["lowResolution"] for item in case["candidates"]] == expected
        for candidate in case["candidates"]:
            status = candidate["status"]
            if status == "REJECTED":
                assert candidate["reason"]
                counts["rejected"] += 1
                continue
            assert status in {"BAKED", "BAKED_WITH_MISSES"}
            assert candidate["closed"]
            assert candidate["lowTriangles"] < case["currentLod0Triangles"]
            assert candidate["sampledSurfaceError"] <= case["maximumSampledError"]
            directory = root / f"rock-{family}" / f"low-resolution-{candidate['lowResolution']}"
            metadata = json.loads((directory / "bake.json").read_text())
            assert metadata["highHash"] == case["highHash"]
            assert metadata["lowHash"] == candidate["lowHash"]
            assert metadata["misses"] == candidate["misses"]
            assert metadata["lowStrategy"] == "recipe-regenerated"
            assert (directory / "low.obj").is_file()
            assert (directory / "high-recipe.json").is_file()
            for name in ("low-normal.png", "low-cavity.png", "low-convex-edge.png"):
                with Image.open(directory / name) as image:
                    assert image.size == (1280, 960)
            alpha = None
            signed_masks = {}
            for name in MAPS:
                with Image.open(directory / name) as image:
                    assert image.size == (2048, 2048)
                    rgba = np.asarray(image.convert("RGBA"))
                if alpha is None:
                    alpha = rgba[:, :, 3]
                    assert np.count_nonzero(alpha) > 0
                else:
                    assert np.array_equal(rgba[:, :, 3], alpha)
                if name in {"cavity.png", "convex-edge.png"}:
                    signed_masks[name] = rgba[:, :, 0] > 127
            assert not np.any(signed_masks["cavity.png"] & signed_masks["convex-edge.png"])
            assert (candidate["misses"] == 0) == (status == "BAKED")
            counts["baked" if status == "BAKED" else "partial"] += 1
    return {"counts": counts, "cases": proof["cases"]}


if __name__ == "__main__":
    result = audit(Path(sys.argv[1]))
    Path(sys.argv[2]).write_text(json.dumps(result, indent=2) + "\n")
    print(result["counts"])
