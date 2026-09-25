"""Require visible baked AO in the selected cave PBR comparison."""

import json
import sys
from pathlib import Path

import numpy as np
from PIL import Image
from AutoUvBakeAudit import audit as audit_uv


root = Path(sys.argv[1])
proof = json.loads((root / "cave-forms-proof.json").read_text())
assert proof["selectedRelief"]
report = {}
for form in ("arch", "square"):
    directory = root / form / "lod-0"
    ao = np.asarray(Image.open(directory / "ambient-occlusion.png").convert("RGBA"))
    painted = ao[:, :, 3] > 0
    values = ao[:, :, 0][painted]
    mean_ao = float(values.mean())
    p90_ao = float(np.percentile(values, 90))
    assert mean_ao > 5 and p90_ao > 16, (form, mean_ao, p90_ao)
    without = np.asarray(Image.open(directory / "low-pbr-no-ao.png").convert("RGB"),
                         dtype=np.float32)
    with_ao = np.asarray(Image.open(directory / "low-pbr-ao.png").convert("RGB"),
                        dtype=np.float32)
    surface = without.mean(axis=2) > 30
    delta = (without - with_ao).mean(axis=2)[surface]
    darkening = float(delta.mean())
    visibly_changed = float((delta > 5).mean())
    assert darkening > 1.5 and visibly_changed > 0.05, (form, darkening, visibly_changed)
    high_ao = np.asarray(Image.open(root / form / "high-pbr-bake" /
                                    "ambient-occlusion.png").convert("RGBA"))
    high_uv = audit_uv(root / form / "high-pbr-bake", require_specialist_mips=True)
    assert high_uv["unpaintedInteriorSamples"] == 0
    assert high_uv["fabricatedRegionIds"] == 0
    high_values = high_ao[:, :, 0][high_ao[:, :, 3] > 0]
    assert float(high_values.mean()) > 5 and float(np.percentile(high_values, 90)) > 16
    high_without = np.asarray(Image.open(root / form / "high-pbr-no-ao.png").convert("RGB"),
                              dtype=np.float32)
    high_with = np.asarray(Image.open(root / form / "high-pbr-ao.png").convert("RGB"),
                           dtype=np.float32)
    high_delta = (high_without - high_with).mean(axis=2)[high_without.mean(axis=2) > 30]
    assert float(high_delta.mean()) > 1.5 and float((high_delta > 5).mean()) > 0.05
    report[form] = {"meanAoByte": mean_ao, "p90AoByte": p90_ao,
                    "pbrDarkeningByte": darkening,
                    "surfacePixelsDarkenedOver5Fraction": visibly_changed,
                    "highMeanAoByte": float(high_values.mean()),
                    "highPbrDarkeningByte": float(high_delta.mean())}
Path(sys.argv[2]).write_text(json.dumps(report, indent=2) + "\n")
print("PASS: selected arch and square have visible baked AO in PBR")
