"""Gate opt-in normal moments against byte-identical controls and both camera distances."""

import hashlib
import json
import sys
from pathlib import Path

import numpy as np
from PIL import Image

from RockReprojectionAudit import audit


def digest(path: Path) -> bytes:
    return hashlib.sha256(path.read_bytes()).digest()


def verify(candidate: Path, previous: Path) -> dict:
    proof = json.loads((candidate / "rock-temporal-proof.json").read_text())
    assert proof["normalMomentVariants"] == ["low-moments"]
    assert proof["normalMomentMipLevels"] == proof["normalMipLevels"] == 4
    assert proof["nearCameraDistance"] == proof["cameraDistance"] * 0.5
    unchanged = sorted((previous / "bake").glob("*.png"))
    assert len(unchanged) >= 30
    for path in unchanged:
        assert digest(candidate / "bake" / path.name) == digest(path), path.name
    controls = []
    for prefix in ("", "near-"):
        for variant in ("high", "low-mips", "low-no-normal-mips"):
            for mode in ("material", "normal", "depth", "primitive"):
                for index in range(5):
                    name = f"{prefix}{variant}-{mode}-{index}.png"
                    assert digest(candidate / name) == digest(previous / name), name
                    controls.append(name)
    assert len(controls) == 120
    shortening = []
    for level in range(1, 4):
        moment = np.asarray(Image.open(candidate / "bake" / f"normal-moment-mip-{level}.png")
                            .convert("RGBA"), dtype=np.uint8)
        regular = np.asarray(Image.open(candidate / "bake" / f"normal-mip-{level}.png")
                             .convert("RGBA"), dtype=np.uint8)
        assert moment.shape == regular.shape
        assert np.array_equal(moment[..., 3], regular[..., 3])
        painted = moment[..., 3] > 0
        assert painted.any()
        lengths = np.linalg.norm(moment[painted, :3].astype(np.float32) * (2 / 255) - 1,
                                 axis=1)
        shortening.append({"level": level, "meanClampedLength": float(np.minimum(lengths, 1).mean()),
                           "fractionBelow0_99": float((lengths < 0.99).mean()),
                           "fractionAbove1_001FromUnorm": float((lengths > 1.001).mean())})
    assert any(entry["fractionBelow0_99"] > 0 for entry in shortening)
    far, near = audit(candidate), audit(candidate, near=True)
    for result, prefix in ((far, ""), (near, "near-")):
        assert prefix + "low-moments" in result["shared"]
        assert result["shared"][prefix + "low-moments"]["pixels"] > 10000
    assert digest(candidate / "low-moments-material-2.png") != digest(
        candidate / "low-mips-material-2.png")
    return {"family": proof["family"], "unchangedBakePng": len(unchanged),
            "unchangedControlCaptures": len(controls), "momentShortening": shortening,
            "far": far, "near": near,
            "limit": "Normal-length roughness widening is an opt-in GGX approximation; "
                     "these micro-orbits are not scene Beauty acceptance."}


if __name__ == "__main__":
    result = verify(Path(sys.argv[1]), Path(sys.argv[2]))
    Path(sys.argv[3]).write_text(json.dumps(result, indent=2) + "\n")
    print(f"PASS: family {result['family']} moment mips and 120 controls audited")
