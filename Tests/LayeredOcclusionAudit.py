"""Compare one layered-rock bake pair with identical high/low geometry and UVs."""

import json
import sys
from pathlib import Path

import numpy as np
from PIL import Image


def audit(baseline: Path, candidate: Path) -> dict:
    left_proof = json.loads((baseline / "rock-lod-bakes-proof.json").read_text())
    right_proof = json.loads((candidate / "rock-lod-bakes-proof.json").read_text())
    assert left_proof.get("layeredOcclusion", False) is False
    assert right_proof["layeredOcclusion"] is True
    assert [case["family"] for case in left_proof["cases"]] == [11]
    assert [case["family"] for case in right_proof["cases"]] == [11]
    result = {"family": 11, "lods": []}
    for lod in range(3):
        left = baseline / "rock-11" / f"lod-{lod}"
        right = candidate / "rock-11" / f"lod-{lod}"
        old = json.loads((left / "bake.json").read_text())
        new = json.loads((right / "bake.json").read_text())
        assert (old["highHash"], old["lowHash"]) == (new["highHash"], new["lowHash"])
        assert old["misses"] == new["misses"] == 0
        assert old["contactRadius"] == 0.125 and old["ambientRadius"] == 0.5
        assert new["contactRadius"] == 0.08 and new["ambientRadius"] == 0.8
        allowed = {"bake.json"}
        for name in ("ambient-occlusion", "contact-occlusion"):
            allowed.update({f"{name}.png", *(f"{name}-mip-{level}.png" for level in range(1, 4))})
        allowed.update({"low-ao.png", "low-contact.png"})
        if lod == 2:
            allowed.update({f"low-{kind}-far-{mode}.png"
                            for kind in ("ao", "contact") for mode in ("base", "mips")})
        names = {p.name for p in left.iterdir()} | {p.name for p in right.iterdir()}
        assert all((left / name).is_file() and (right / name).is_file() for name in names)
        changed = {name for name in names if (left / name).read_bytes() != (right / name).read_bytes()}
        assert changed == allowed, f"LOD{lod} unexpected changed outputs: {changed ^ allowed}"
        row = {"lod": lod, "highHash": old["highHash"], "lowHash": old["lowHash"], "maps": {}}
        for name in ("ambient-occlusion", "contact-occlusion"):
            old_map = np.asarray(Image.open(left / f"{name}.png").convert("RGBA"))
            new_map = np.asarray(Image.open(right / f"{name}.png").convert("RGBA"))
            assert np.array_equal(old_map[:, :, 3], new_map[:, :, 3])
            covered = old_map[:, :, 3] > 0
            before = old_map[:, :, 0][covered].astype(np.int16)
            after = new_map[:, :, 0][covered].astype(np.int16)
            assert before.size > 0 and np.any(before != after), f"LOD{lod} {name} did not respond"
            row["maps"][name] = {
                "coveredTexels": int(before.size),
                "meanBaseline": round(float(before.mean()) / 255, 5),
                "meanCandidate": round(float(after.mean()) / 255, 5),
                "changedFraction": round(float(np.mean(before != after)), 5),
                "p95AbsoluteChange": round(float(np.percentile(np.abs(after - before), 95)) / 255, 5),
            }
        result["lods"].append(row)
    return result


if __name__ == "__main__":
    report = audit(Path(sys.argv[1]), Path(sys.argv[2]))
    Path(sys.argv[3]).write_text(json.dumps(report, indent=2) + "\n")
    print("PASS: three matched layered LODs; only ambient and contact bake outputs changed")
