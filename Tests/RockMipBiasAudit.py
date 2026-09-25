"""Gate a normal-only mip bias experiment against its unchanged bake and renderer control."""

import hashlib
import json
import sys
from pathlib import Path

from RockReprojectionAudit import audit


MAPS = ("normal", "ambient-occlusion", "contact-occlusion", "signed-curvature",
        "cavity", "convex-edge", "crease-distance-mask", "thickness", "region-id",
        "signed-height")
EXPECTED_BIASES = {"low-bias-half": -0.5, "low-bias-one": -1.0}


def digest(path: Path) -> bytes:
    return hashlib.sha256(path.read_bytes()).digest()


def verify(candidate: Path, previous: Path, reference: Path) -> dict:
    proof = json.loads((candidate / "rock-temporal-proof.json").read_text())
    assert proof["normalMipBiasVariants"] == EXPECTED_BIASES
    assert proof["nearCameraDistance"] == proof["cameraDistance"] * 0.5
    for name in MAPS:
        filename = name + ".png"
        assert digest(candidate / "bake" / filename) == digest(reference / filename), filename
    controls = sorted(path.name for path in previous.glob("*.png"))
    assert len(controls) == 60, len(controls)  # Material, normal, depth and triangle ID.
    for name in controls:
        assert digest(candidate / name) == digest(previous / name), name
    measured = audit(candidate)
    near_measured = audit(candidate, near=True)
    assert all(name in measured["variants"] and name in measured["shared"]
               for name in EXPECTED_BIASES)
    baseline = measured["shared"]["low-mips"]
    candidates = {}
    for name, bias in EXPECTED_BIASES.items():
        result = measured["shared"][name]
        candidates[name] = {
            "bias": bias,
            "sharedPixels": result["pixels"],
            "normalSpatialDegrees": result["normalSpatialDegrees"],
            "normalTemporalResidualDegrees": result["normalTemporalResidualDegrees"],
            "perTransitionNormalResidualDegrees": result["perTransitionNormalResidualDegrees"],
            "meanSpatialImproves": result["normalSpatialDegrees"]["mean"] <
                                   baseline["normalSpatialDegrees"]["mean"],
            "meanTemporalResidualImproves": result["normalTemporalResidualDegrees"]["mean"] <
                                            baseline["normalTemporalResidualDegrees"]["mean"]}
    near_candidates = {}
    near_baseline = near_measured["shared"]["near-low-mips"]
    for name, bias in EXPECTED_BIASES.items():
        result = near_measured["shared"]["near-" + name]
        near_candidates[name] = {
            "bias": bias,
            "sharedPixels": result["pixels"],
            "normalSpatialDegrees": result["normalSpatialDegrees"],
            "normalTemporalResidualDegrees": result["normalTemporalResidualDegrees"],
            "perTransitionNormalResidualDegrees": result["perTransitionNormalResidualDegrees"],
            "meanSpatialImproves": result["normalSpatialDegrees"]["mean"] <
                                   near_baseline["normalSpatialDegrees"]["mean"],
            "meanTemporalResidualImproves": result["normalTemporalResidualDegrees"]["mean"] <
                                            near_baseline["normalTemporalResidualDegrees"]["mean"]}
    return {"family": measured["family"], "tenBaseMapsExact": True,
            "sixtyControlCapturesExact": True, "farCandidates": candidates,
            "nearCandidates": near_candidates,
            "farControls": {name: measured["shared"][name] for name in
                            ("low-mips", "low-no-normal-mips")},
            "nearControls": {name: near_measured["shared"]["near-" + name] for name in
                             ("low-mips", "low-no-normal-mips")},
            "limit": "This test records tradeoffs; an improved scalar metric is not Beauty acceptance."}


if __name__ == "__main__":
    result = verify(Path(sys.argv[1]), Path(sys.argv[2]), Path(sys.argv[3]))
    Path(sys.argv[4]).write_text(json.dumps(result, indent=2) + "\n")
    print(f"PASS: family {result['family']} normal-only mip bias candidates audited")
