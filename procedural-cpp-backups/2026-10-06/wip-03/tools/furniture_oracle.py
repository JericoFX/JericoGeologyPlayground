#!/usr/bin/env python3
"""Offline test-data authoring only. Never imported or invoked by native targets.

Runs the SHA-frozen pre-port reference to materialize comparison scenes and swept
clearance reports. It adds no geometry, materials, or production implementation.
"""
import argparse
import hashlib
import json
from pathlib import Path
import shutil
import sys

parser = argparse.ArgumentParser()
parser.add_argument("--source", type=Path, required=True)
parser.add_argument("--out", type=Path, required=True)
args = parser.parse_args()
source = args.source.resolve()
output = args.out.resolve()
output.mkdir(parents=True, exist_ok=True)
sys.path.insert(0, str(source))
from furniture import build_furniture, sweep_clearance, validate_furniture

sources = {name: hashlib.sha256((source / name).read_bytes()).hexdigest()
           for name in ("furniture.py", "mesh_core.py")}
manifest = json.loads((source / "fixtures/manifest.json").read_text())
for entry in manifest:
    path = source / "fixtures" / entry["file"]
    if hashlib.sha256(path.read_bytes()).hexdigest() != entry["file_sha256"]:
        raise SystemExit(f"Frozen fixture digest mismatch: {path}")
    shutil.copyfile(path, output / entry["file"])
    scene = json.loads(path.read_text())
    entry["clearance"] = sweep_clearance(scene)

recipes = [
    (f"cabinet_{n}_varied", dict(drawer_count=n, width=.88, height=1.64,
                                 depth=.62, seed=19 + n, open_fraction=.71))
    for n in (2, 4, 6)
] + [
    ("cabinet_single_open", dict(drawer_count=1, open_fraction=1, seed=-7)),
    ("cabinet_short_eight", dict(height=1.42, drawer_count=8, seed=2026)),
    ("cabinet_minimum", dict(width=.55, depth=.4, height=.65, drawer_count=2,
                             wear=0, seed=0)),
    ("cabinet_maximum", dict(width=2.4, depth=.95, height=2.1, drawer_count=8,
                             wear=1, seed=2**40 + 31)),
    ("workbench_no_optional", dict(preset="workbench", drawer=False, shelf=False,
                                   seed=5)),
    ("workbench_no_drawer", dict(preset="workbench", drawer=False, shelf=True,
                                 seed=22)),
    ("workbench_no_shelf", dict(preset="workbench", drawer=True, shelf=False,
                                open_fraction=1, seed=61)),
    ("workbench_minimum", dict(preset="workbench", width=.55, depth=.4,
                               height=.65, open_fraction=1, seed=11)),
    ("workbench_maximum", dict(preset="workbench", width=2.4, depth=.95,
                               height=2.1, wear=1, seed=911)),
]
for name, recipe in recipes:
    scene = build_furniture(recipe)
    errors = validate_furniture(scene)
    if errors:
        raise SystemExit(f"Source fixture fails validation: {name}: {errors}")
    raw = json.dumps(scene, sort_keys=True, separators=(",", ":"), allow_nan=False).encode()
    filename = name + ".json"
    (output / filename).write_bytes(raw)
    manifest.append(dict(file=filename, recipe=scene["recipe"],
                         file_sha256=hashlib.sha256(raw).hexdigest(),
                         nodes=len(scene["nodes"]), parts=len(scene["parts"]),
                         clearance=sweep_clearance(scene)))
(output / "manifest.json").write_text(json.dumps(manifest, indent=2) + "\n")
(output / "oracle-provenance.json").write_text(json.dumps(dict(
    source_snapshot=source.name, source_sha256=sources,
    scope="Offline reference fixtures only; no Python dependency in native generation or tests",
    recipe_count=len(manifest)), indent=2) + "\n")
print(f"Wrote {len(manifest)} source fixtures, with SHA-256 and continuous-clearance reports")
