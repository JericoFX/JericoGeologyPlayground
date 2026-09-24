"""Check the C++ edge-radius sweep without selecting a production default."""

import hashlib
import json
import sys
from pathlib import Path

import numpy as np
from PIL import Image


def digest(path: Path) -> str:
    return hashlib.sha256(path.read_bytes()).hexdigest()


def audit(root: Path) -> dict:
    proof = json.loads((root / "edge-radius-sweep-proof.json").read_text())
    assert {entry["family"] for entry in proof["cases"]} == {5, 8, 10, 12}
    result = []
    for case in proof["cases"]:
        family = case["family"]
        assert [entry["metres"] for entry in case["radii"]] == [0.02, 0.1, 0.2]
        rows = []
        reference = None
        for entry in case["radii"]:
            radius = entry["metres"]
            directory = root / f"rock-{family}" / f"edge-radius-{round(radius * 100)}"
            metadata = json.loads((directory / "bake.json").read_text())
            assert metadata["highHash"] == case["highHash"]
            assert metadata["lowHash"] == case["lowHash"]
            assert metadata["edgeRadius"] == radius
            assert metadata["misses"] == entry["misses"]
            with Image.open(directory / "convex-edge.png") as image:
                assert image.size == (2048, 2048)
                pixels = np.asarray(image.convert("RGBA"))
            valid = pixels[:, :, 0][pixels[:, :, 3] > 0]
            bright = float((valid > 127).mean())
            with Image.open(directory / "low-convex-edge.png") as image:
                assert image.size == (1280, 960)
            identities = {name: digest(directory / name) for name in
                          ("normal.png", "ambient-occlusion.png", "contact-occlusion.png",
                           "cavity.png", "low.obj", "high-recipe.json")}
            if reference is None:
                reference = identities
            else:
                assert identities == reference, "radius changed an unrelated map or mesh"
            rows.append({"metres": radius, "brightFraction": round(bright, 5),
                         "misses": entry["misses"]})
        assert rows[0]["brightFraction"] <= rows[1]["brightFraction"] <= rows[2]["brightFraction"]
        result.append({"family": family, "radii": rows})
    return {"cases": result}


if __name__ == "__main__":
    result = audit(Path(sys.argv[1]))
    Path(sys.argv[2]).write_text(json.dumps(result, indent=2) + "\n")
    for case in result["cases"]:
        print(case["family"], " ".join(f"{r['metres']}: {r['brightFraction']:.3f}"
                                        for r in case["radii"]))
