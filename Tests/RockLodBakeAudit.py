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
          require_specialist_mips: bool = False,
          expected_families: set[int] | None = None,
          cliff_drainage: bool = False, cliff_edge_damage: bool = False,
          cliff_rbrush_alpha: bool = False,
          layered_occlusion: bool = False, layered_material: bool = False,
          cliff_eroded_rim: bool = False,
          layered_horizon_fracture: bool = False) -> dict:
    proof = json.loads((root / "rock-lod-bakes-proof.json").read_text())
    result = {"families": [], "completeBakes": 0, "bakesWithMisses": 0, "rejectedLods": 0}
    assert proof.get("cliffDrainage", False) is cliff_drainage
    assert proof.get("cliffEdgeDamage", False) is cliff_edge_damage
    assert proof.get("cliffRBrushAlpha", False) is cliff_rbrush_alpha
    assert proof.get("cliffErodedRim", False) is cliff_eroded_rim
    assert proof.get("layeredOcclusion", False) is layered_occlusion
    assert proof.get("layeredMaterial", False) is layered_material
    assert proof.get("layeredHorizonFracture", False) is layered_horizon_fracture
    assert {case["family"] for case in proof["cases"]} == (
        {5, 8, 9, 10, 11, 12} if expected_families is None else expected_families)
    for case in proof["cases"]:
        family = case["family"]
        if layered_horizon_fracture:
            assert family == 11
            assert case["defaultMaterialAppearance"] == 2
            assert case["defaultMaterialDiffuse"] == "cliff_side-diff-1k.jpg"
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
            if layered_horizon_fracture:
                assert lod["misses"] == lod["backfaces"] == 0
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
                "recipe-regenerated" if family in {5, 11} or family == 12 and lod["lod"] == 2
                else "native-simplify")
            assert metadata["edgeRadius"] == lod["edgeRadius"]
            assert lod["edgeRadius"] == (0.1 if family == 8 else 0.02)
            assert (directory / "low.obj").is_file()
            assert (directory / "high-recipe.json").is_file()
            if cliff_eroded_rim:
                recipe = json.loads((directory / "high-recipe.json").read_text())
                assert recipe["version"] == 14
                assert any(layer["kind"] == 19 and layer["profile"] == 3 and
                           layer["alphaSource"] == 1 for layer in recipe["layers"])
            if layered_horizon_fracture:
                recipe = json.loads((directory / "high-recipe.json").read_text())
                assert recipe["version"] == 15
                assert any(layer["kind"] == 4 and layer["profile"] == 2
                           for layer in recipe["layers"])
            for view in ("high.png", "low.png", "low-normal.png",
                         "low-ao.png", "low-contact.png", "low-cavity.png",
                         "low-convex-edge.png"):
                with Image.open(directory / view) as image:
                    assert image.size == (1280, 960)
            if lod["lod"] == 2:
                for view in ("high-far.png", "low-normal-far.png"):
                    with Image.open(directory / view) as image:
                        assert image.size == (1280, 960)
            if cliff_rbrush_alpha:
                for view in ("high-material.png", "low-material.png"):
                    with Image.open(directory / view) as image:
                        assert image.size == (1280, 960)
            if layered_horizon_fracture:
                for view in ("high-material.png", "low-material.png"):
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
    options = sys.argv[3:]
    require_specialist_mips = "--require-specialist-mips" in options
    family = int(options[options.index("--family") + 1]) if "--family" in options else None
    cliff_drainage = "--cliff-drainage" in options
    cliff_edge_damage = "--cliff-edge-damage" in options
    cliff_eroded_rim = "--cliff-eroded-rim" in options
    cliff_rbrush_alpha = "--cliff-rbrush-alpha" in options or cliff_eroded_rim
    layered_occlusion = "--layered-occlusion" in options
    layered_material = "--layered-material" in options
    layered_horizon_fracture = "--layered-horizon-fracture" in options
    assert sum((cliff_drainage, cliff_edge_damage, cliff_rbrush_alpha, layered_occlusion)) <= 1
    output = audit(Path(sys.argv[1]),
                   "--require-auto-uv" in options or require_specialist_mips,
                   require_specialist_mips,
                   {family} if family is not None else None,
                   cliff_drainage, cliff_edge_damage, cliff_rbrush_alpha,
                   layered_occlusion, layered_material, cliff_eroded_rim,
                   layered_horizon_fracture)
    target = Path(sys.argv[2])
    target.write_text(json.dumps(output, indent=2) + "\n")
    print(f"PASS: {output['completeBakes']} complete, "
          f"{output['bakesWithMisses']} with misses, "
          f"{output['rejectedLods']} rejected LODs")
