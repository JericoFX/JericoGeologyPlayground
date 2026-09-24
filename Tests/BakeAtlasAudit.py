"""Report the effective triangle-chart sampling footprint of existing bakes."""

import json
import math
import sys
from pathlib import Path


def audit(path: Path) -> dict:
    bake = json.loads(path.read_text(encoding="utf-8"))
    resolution = int(bake["resolution"])
    triangles = int(bake["lowTriangles"])
    padding = int(bake["padding"])
    if resolution <= 0 or triangles <= 0 or padding < 0:
        raise ValueError(f"invalid atlas dimensions: {path}")
    columns = math.ceil(math.sqrt(triangles))
    tile = resolution // columns
    span = tile - 2 * padding
    if span <= 0:
        raise ValueError(f"no triangle interior: {path}")
    return {
        "path": str(path),
        "resolution": resolution,
        "lowTriangles": triangles,
        "columns": columns,
        "tileTexels": tile,
        "paddingTexels": padding,
        "triangleEdgeSpanTexels": span,
        "meanCoveredSamplesPerTriangle": bake["samples"] / triangles,
        "nominalInteriorTriangleTexelsByMip": [
            0.5 * (span / (2**level)) ** 2 for level in range(4)
        ],
        "note": "Nominal geometric footprint only; no mip images or filtered render were produced.",
    }


if __name__ == "__main__":
    if len(sys.argv) != 3:
        raise SystemExit("usage: BakeAtlasAudit.py <bake-directory> <output-json>")
    source = Path(sys.argv[1])
    result = {"families": [audit(path) for path in sorted(source.glob("normal-bake-*/bake.json"))]}
    if not result["families"]:
        raise SystemExit(f"no bake.json files in {source}")
    Path(sys.argv[2]).write_text(json.dumps(result, indent=2) + "\n", encoding="utf-8")
    print(json.dumps(result, indent=2))
