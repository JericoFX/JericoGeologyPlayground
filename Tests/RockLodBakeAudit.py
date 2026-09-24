"""Audit native high-to-low rock LOD packages without treating rejected LODs as passes."""

import json
import sys
from pathlib import Path

import numpy as np
from PIL import Image
from AutoUvBakeAudit import audit as audit_uv


MAPS = (
    "normal.png", "ambient-occlusion.png", "contact-occlusion.png",
    "signed-curvature.png", "cavity.png", "convex-edge.png",
    "crease-distance-mask.png", "thickness.png", "region-id.png",
    "signed-height.png",
)


def audit(root: Path, require_automatic_uv: bool = False,
          require_specialist_mips: bool = False) -> dict:
    proof = json.loads((root / "rock-lod-bakes-proof.json").read_text())
    result = {"families": [], "completeBakes": 0, "bakesWithMisses": 0, "rejectedLods": 0}
    assert {case["family"] for case in proof["cases"]} == {5, 8, 9, 10, 11, 12}
    for case in proof["cases"]:
        family = case["family"]
        assert [lod["lod"] for lod in case["lods"]] == [0, 1, 2]
        if family == 12:
            assert case["lods"][2]["status"] == "BAKED", \
                "crater LOD2 must be a complete reduced high-to-low bake"
        if family == 10:
            assert case["lods"][2]["status"] == "BAKED", \
                "angular LOD2 must not lose projections across semantic regions"
        previous = None
        entries = []
        for lod in case["lods"]:
            status = lod["status"]
            if status == "REJECTED":
                assert lod["reason"]
                result["rejectedLods"] += 1
                entries.append({"lod": lod["lod"], "status": status, "reason": lod["reason"]})
                continue
            assert status in {"BAKED", "BAKED_WITH_MISSES"}
            assert lod["sampledSurfaceError"] <= lod["errorBudget"]
            assert lod["lowTriangles"] < lod["highTriangles"]
            if previous is not None:
                assert lod["lowTriangles"] < previous
            previous = lod["lowTriangles"]
            directory = root / f"rock-{family}" / f"lod-{lod['lod']}"
            metadata = json.loads((directory / "bake.json").read_text())
            if require_automatic_uv:
                assert metadata["automaticUvIslandCount"] > 0
                assert metadata["automaticUvMarginPixels"] >= 8
                assert metadata["normalMipLevels"] == 4
                assert len(json.loads((directory / "uv-chart-islands.json").read_text())) == lod["lowTriangles"]
                for level, size in enumerate((1024, 512, 256), start=1):
                    with Image.open(directory / f"normal-mip-{level}.png") as mip:
                        assert mip.size == (size, size)
            assert metadata["highHash"] == lod["highHash"]
            assert metadata["lowHash"] == lod["lowHash"]
            assert metadata["misses"] == lod["misses"]
            assert metadata["lowTriangles"] == lod["lowTriangles"]
            assert metadata["lowStrategy"] == (
                "recipe-regenerated" if family == 5 or family == 12 and lod["lod"] == 2
                else "native-simplify")
            assert metadata["edgeRadius"] == lod["edgeRadius"]
            assert lod["edgeRadius"] == (0.1 if family == 8 else 0.02)
            assert (directory / "low.obj").is_file()
            assert (directory / "high-recipe.json").is_file()
            for view in ("high.png", "low.png", "low-normal.png",
                         "low-ao.png", "low-contact.png", "low-cavity.png",
                         "low-convex-edge.png"):
                with Image.open(directory / view) as image:
                    assert image.size == (1280, 960)
            if lod["lod"] == 2:
                for view in ("high-far.png", "low-normal-far.png"):
                    with Image.open(directory / view) as image:
                        assert image.size == (1280, 960)
            alpha = None
            metrics = {}
            curvature_masks = {}
            for name in MAPS:
                with Image.open(directory / name) as image:
                    assert image.size == (2048, 2048)
                    data = np.asarray(image.convert("RGBA"))
                current_alpha = data[:, :, 3]
                if alpha is None:
                    alpha = current_alpha
                else:
                    assert np.array_equal(alpha, current_alpha), name
                valid = data[:, :, 0][current_alpha > 0]
                assert valid.size > 0
                if name in {"cavity.png", "convex-edge.png"}:
                    curvature_masks[name] = valid > 127
                metrics[name] = {"mean": round(float(valid.mean()), 2),
                                 "brightFraction": round(float((valid > 127).mean()), 4)}
            assert not np.any(curvature_masks["cavity.png"] &
                              curvature_masks["convex-edge.png"]), \
                "concave and convex masks overlap at a texel"
            mip_audit = None
            if require_specialist_mips:
                assert metadata["specialistMipLevels"] == metadata["normalMipLevels"] == 4
                mip_audit = audit_uv(directory, True)
                assert mip_audit["unpaintedInteriorSamples"] == 0
                assert mip_audit["fabricatedRegionIds"] == 0
                assert not any(mip_audit["specialistMipAlphaMismatches"].values())
            assert (lod["misses"] == 0) == (status == "BAKED")
            result["completeBakes" if status == "BAKED" else "bakesWithMisses"] += 1
            entries.append({"lod": lod["lod"], "status": status,
                            "lowTriangles": lod["lowTriangles"],
                            "misses": lod["misses"], "maps": metrics,
                            "specialistMips": None if mip_audit is None else {
                                "unpaintedInteriorSamples": mip_audit["unpaintedInteriorSamples"],
                                "fabricatedRegionIds": mip_audit["fabricatedRegionIds"],
                                "maskPeakByLevel": mip_audit["maskPeakByLevel"]}})
        result["families"].append({"family": family, "lods": entries})
    return result


if __name__ == "__main__":
    require_specialist_mips = "--require-specialist-mips" in sys.argv[3:]
    output = audit(Path(sys.argv[1]),
                   "--require-auto-uv" in sys.argv[3:] or require_specialist_mips,
                   require_specialist_mips)
    target = Path(sys.argv[2])
    target.write_text(json.dumps(output, indent=2) + "\n")
    print(f"PASS: {output['completeBakes']} complete, "
          f"{output['bakesWithMisses']} with misses, "
          f"{output['rejectedLods']} rejected LODs")
