"""Audit the arch and square-tunnel high-to-low packages and UV/mip coverage."""

import json
import sys
from pathlib import Path

from AutoUvBakeAudit import audit as audit_uv


def audit(root: Path) -> dict:
    proof = json.loads((root / "cave-forms-proof.json").read_text())
    assert proof["imageSize"] == [1280, 960] and proof["textureResolution"] == 1024
    assert [form["form"] for form in proof["forms"]] == ["arch", "square"]
    result = {"forms": [], "completeBakes": 0, "bakesWithMisses": 0,
              "unpaintedUvSamples": 0, "fabricatedRegionIds": 0}
    for form in proof["forms"]:
        assert [lod["lod"] for lod in form["lods"]] == [0, 1, 2]
        previous = form["highTriangles"]
        entries = []
        for lod in form["lods"]:
            assert lod["triangles"] < previous
            assert lod["sampledSurfaceError"] <= 0.15
            assert lod["misses"] == lod["backfaces"] == 0
            previous = lod["triangles"]
            directory = root / form["form"] / f"lod-{lod['lod']}"
            uv = audit_uv(directory, require_specialist_mips=True)
            assert uv["unpaintedInteriorSamples"] == 0
            assert uv["fabricatedRegionIds"] == 0
            assert not any(uv["specialistAlphaMismatches"].values())
            assert not any(uv["specialistMipAlphaMismatches"].values())
            result["completeBakes"] += 1
            entries.append({"lod": lod["lod"], "triangles": lod["triangles"],
                            "sampledSurfaceError": lod["sampledSurfaceError"],
                            "uvIslands": lod["uvIslands"], "bakeCpuMs": lod["bakeCpuMs"]})
        result["forms"].append({"form": form["form"], "highTriangles": form["highTriangles"],
                                "lods": entries})
    return result


if __name__ == "__main__":
    result = audit(Path(sys.argv[1]))
    Path(sys.argv[2]).write_text(json.dumps(result, indent=2) + "\n")
    print(f"PASS: {result['completeBakes']} cave bakes, zero misses or UV holes")
