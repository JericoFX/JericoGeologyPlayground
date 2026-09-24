"""Measure the current rock LOD UVs against the automatic-unwrap contract."""

import json
import sys
from pathlib import Path

import numpy as np


def obj_metrics(path: Path, resolution: int) -> dict:
    positions, uvs, faces, tex_faces = [], [], [], []
    for line in path.open(encoding="utf-8"):
        if line.startswith("v "):
            positions.append(tuple(map(float, line.split()[1:4])))
        elif line.startswith("vt "):
            uvs.append(tuple(map(float, line.split()[1:3])))
        elif line.startswith("f "):
            triples = [part.split("/") for part in line.split()[1:]]
            assert len(triples) == 3
            faces.append(tuple(int(part[0]) - 1 for part in triples))
            tex_faces.append(tuple(int(part[1]) - 1 for part in triples))
    p = np.asarray(positions, dtype=np.float64)[np.asarray(faces)]
    uv = np.asarray(uvs, dtype=np.float64)[np.asarray(tex_faces)]
    e1, e2 = p[:, 1] - p[:, 0], p[:, 2] - p[:, 0]
    du1, du2 = uv[:, 1] - uv[:, 0], uv[:, 2] - uv[:, 0]
    det = du1[:, 0] * du2[:, 1] - du1[:, 1] * du2[:, 0]
    mesh_area = np.linalg.norm(np.cross(e1, e2), axis=1) * .5
    uv_area = np.abs(det) * .5
    valid = (mesh_area > 1e-15) & (uv_area > 1e-15)
    assert valid.all() and np.isfinite(uv).all()
    assert uv.min() >= -1e-6 and uv.max() <= 1 + 1e-6
    dpdu = (e1 * du2[:, 1, None] - e2 * du1[:, 1, None]) / det[:, None]
    dpdv = (-e1 * du2[:, 0, None] + e2 * du1[:, 0, None]) / det[:, None]
    a = np.einsum("ij,ij->i", dpdu, dpdu)
    b = np.einsum("ij,ij->i", dpdu, dpdv)
    c = np.einsum("ij,ij->i", dpdv, dpdv)
    discriminant = np.sqrt((a - c) ** 2 + 4 * b ** 2)
    stretch = np.sqrt((a + c + discriminant) /
                      np.maximum(a + c - discriminant, 1e-30))
    density = resolution * np.sqrt(uv_area / mesh_area)
    return {
        "triangles": len(faces),
        "activeUvAreaFraction": float(uv_area.sum()),
        "texelDensityP5": float(np.percentile(density, 5)),
        "texelDensityP50": float(np.median(density)),
        "texelDensityP95": float(np.percentile(density, 95)),
        "stretchP50": float(np.median(stretch)),
        "stretchP95": float(np.percentile(stretch, 95)),
        "stretchMax": float(stretch.max()),
        "zeroUvAreaFaces": int((~valid).sum()),
    }


def audit(root: Path) -> dict:
    proof = json.loads((root / "rock-lod-bakes-proof.json").read_text())
    cases = []
    for family in proof["cases"]:
        for lod in family["lods"]:
            if lod["status"] == "REJECTED":
                continue
            directory = root / f"rock-{family['family']}" / f"lod-{lod['lod']}"
            bake = json.loads((directory / "bake.json").read_text())
            resolution, padding = bake["resolution"], bake["padding"]
            required_margin = max(4, 2 ** (4 - 1), 4)  # Four levels, one mip-safety pixel, BC block.
            entry = {"family": family["family"], "lod": lod["lod"],
                     "resolution": resolution, "actualPaddingPixels": padding,
                     "requiredPaddingPixelsAtFourMips": required_margin,
                     "mipMarginPass": padding >= required_margin,
                     **obj_metrics(directory / "low.obj", resolution)}
            assert entry["triangles"] == lod["lowTriangles"]
            cases.append(entry)
    return {
        "scope": "existing OBJ UV metrics; four-mip margin is a diagnostic target, not a shipped mip chain",
        "formula": "max(bakeDilation=4, 2^(mipCount-1)*minimumMipSafety=8, BCblockSafety=4)",
        "cases": cases,
    }


if __name__ == "__main__":
    result = audit(Path(sys.argv[1]))
    Path(sys.argv[2]).write_text(json.dumps(result, indent=2) + "\n")
    print(f"measured {len(result['cases'])} baked rock LOD UV sets")
