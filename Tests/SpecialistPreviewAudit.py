"""Verify that far Vulkan views actually sample exported specialist mip chains."""

import hashlib
import json
import sys
from pathlib import Path

import numpy as np
from PIL import Image


def inspect_pair(directory: Path, stem: str) -> dict:
    base_path = directory / f"{stem}-far-base.png"
    mip_path = directory / f"{stem}-far-mips.png"
    with Image.open(base_path) as image:
        base = np.asarray(image.convert("RGB"), dtype=np.int16)
    with Image.open(mip_path) as image:
        mip = np.asarray(image.convert("RGB"), dtype=np.int16)
    if base.shape != (960, 1280, 3) or mip.shape != base.shape:
        raise AssertionError(f"unexpected preview dimensions: {directory / stem}")
    difference = np.abs(base - mip)
    return {
        "baseSha256": hashlib.sha256(base_path.read_bytes()).hexdigest(),
        "mipSha256": hashlib.sha256(mip_path.read_bytes()).hexdigest(),
        "changedPixels": int(np.count_nonzero(np.any(difference != 0, axis=2))),
        "meanRgbAbsoluteDifference": float(difference.mean()),
    }


def source_range(directory: Path, name: str) -> int:
    with Image.open(directory / f"{name}.png") as image:
        source = np.asarray(image.convert("RGB"), dtype=np.int16)[:, :, 0]
    return int(source.max() - source.min())


def audit(root: Path) -> dict:
    proof = json.loads((root / "rock-lod-bakes-proof.json").read_text())
    resolution = int(proof["textureResolution"])
    expected_mip_bytes = sum(4 * (resolution >> level) ** 2 for level in range(1, 4))
    cases = []
    for case in proof["cases"]:
        family = case["family"]
        lod_index = 1 if family == 9 else 2
        lod = next(item for item in case["lods"] if item["lod"] == lod_index)
        if lod["status"] != "BAKED":
            raise AssertionError(f"missing complete diagnostic mip proof: {family}/{lod_index}")
        directory = root / f"rock-{family}" / f"lod-{lod_index}"
        maps = {}
        for name, source_name in (("ao", "ambient-occlusion"),
                                  ("contact", "contact-occlusion"),
                                  ("cavity", "cavity"),
                                  ("convex-edge", "convex-edge")):
            mip_proof = lod.get("diagnosticMipMaps", {}).get(name, {})
            if mip_proof.get("levels") != 4 or mip_proof.get("bytesAddedEstimate") != expected_mip_bytes:
                raise AssertionError(f"incomplete {name} mip upload proof: {family}/{lod_index}")
            result = inspect_pair(directory, f"low-{name}")
            result["sourceRange"] = source_range(directory, source_name)
            if result["sourceRange"] > 1 and result["changedPixels"] <= 100:
                raise AssertionError(f"far {name} did not visibly use mips: {family}/{lod_index}")
            maps[name] = result
        cases.append({"family": family, "lod": lod_index, "maps": maps})
    if len(cases) != 6:
        raise AssertionError("all six families need a far diagnostic preview")
    return {"scope": "same-camera 1280x960 Vulkan base-only versus four-level specialist map previews",
            "cases": cases}


if __name__ == "__main__":
    if len(sys.argv) != 3:
        raise SystemExit("usage: SpecialistPreviewAudit.py <package> <output-json>")
    result = audit(Path(sys.argv[1]))
    Path(sys.argv[2]).write_text(json.dumps(result, indent=2) + "\n")
    print(f"PASS: {len(result['cases'])} families have filtered far specialist previews")
