"""Compare paired native curvature bakes on identical geometry and UV inputs."""

import hashlib
import json
import sys
from pathlib import Path

import numpy as np
from PIL import Image


UNCHANGED = ("normal", "ambient-occlusion", "contact-occlusion",
             "crease-distance-mask", "thickness", "region-id", "signed-height")
CURVATURE = ("signed-curvature", "cavity", "convex-edge")
EXPECTED_CASES = 17


def digest(path: Path) -> str:
    return hashlib.sha256(path.read_bytes()).hexdigest()


def image(path: Path) -> np.ndarray:
    return np.asarray(Image.open(path).convert("RGBA"))


def total_variation(gray: np.ndarray, valid: np.ndarray) -> float:
    horizontal = valid[:, 1:] & valid[:, :-1]
    vertical = valid[1:, :] & valid[:-1, :]
    difference_x = np.abs(gray[:, 1:].astype(np.int16) - gray[:, :-1].astype(np.int16))
    difference_y = np.abs(gray[1:, :].astype(np.int16) - gray[:-1, :].astype(np.int16))
    count = int(horizontal.sum() + vertical.sum())
    return float((difference_x[horizontal].sum() + difference_y[vertical].sum()) / (255 * count))


def sign_flip_fraction(before: np.ndarray, after: np.ndarray) -> float:
    if before.shape != after.shape or not np.array_equal(before[:, :, 3], after[:, :, 3]):
        raise ValueError("signed-curvature mip coverage changed")
    valid = before[:, :, 3] > 0
    old = before[:, :, 0]
    new = after[:, :, 0]
    return float(np.mean(((old[valid] > 129) & (new[valid] < 127)) |
                         ((old[valid] < 127) & (new[valid] > 129))))


def compare(base: Path, candidate: Path) -> dict:
    original = json.loads((base / "bake.json").read_text(encoding="utf-8"))
    modified = json.loads((candidate / "bake.json").read_text(encoding="utf-8"))
    for key in ("highHash", "lowHash", "lowTriangles", "resolution", "misses",
                "normalMipLevels", "specialistMipLevels"):
        if original[key] != modified[key]:
            raise ValueError(f"changed {key}: {base}")
    if modified.get("curvatureRelaxation") != 0.5 or digest(base / "low.obj") != digest(candidate / "low.obj"):
        raise ValueError(f"candidate geometry, UV, or relaxation differs: {base}")
    for name in UNCHANGED:
        for suffix in ("", "-mip-1", "-mip-2", "-mip-3"):
            if digest(base / f"{name}{suffix}.png") != digest(candidate / f"{name}{suffix}.png"):
                raise ValueError(f"unrelated map changed: {name}{suffix} {base}")

    changes = {}
    for name in CURVATURE:
        before = image(base / f"{name}.png")
        after = image(candidate / f"{name}.png")
        if before.shape != after.shape or not np.array_equal(before[:, :, 3], after[:, :, 3]):
            raise ValueError(f"curvature coverage changed: {name} {base}")
        valid = before[:, :, 3] > 0
        old = before[:, :, 0]
        new = after[:, :, 0]
        difference = np.abs(old[valid].astype(np.int16) - new[valid].astype(np.int16)) / 255
        changes[name] = {
            "changedFraction": float(np.mean(old[valid] != new[valid])),
            "meanAbsoluteDifference": float(np.mean(difference)),
            "p95AbsoluteDifference": float(np.percentile(difference, 95)),
            "baseSpatialVariation": total_variation(old, valid),
            "candidateSpatialVariation": total_variation(new, valid),
            "baseFractionAboveHalf": float(np.mean(old[valid] > 128)),
            "candidateFractionAboveHalf": float(np.mean(new[valid] > 128)),
        }
        if name == "signed-curvature":
            changes[name]["signFlipFraction"] = sign_flip_fraction(before, after)
            changes[name]["mipSignFlipFraction"] = {
                str(level): sign_flip_fraction(
                    image(base / f"signed-curvature-mip-{level}.png"),
                    image(candidate / f"signed-curvature-mip-{level}.png"))
                for level in range(1, 4)
            }
    return {"family": base.parent.name, "lod": base.name,
            "highHash": original["highHash"], "lowHash": original["lowHash"],
            "baseBakeCpuMs": original["cpuMs"], "candidateBakeCpuMs": modified["cpuMs"],
            "maps": changes}


if __name__ == "__main__":
    if len(sys.argv) not in (4, 5) or (len(sys.argv) == 5 and sys.argv[4] != "--require-no-sign-flips"):
        raise SystemExit("usage: CurvatureRelaxationAudit.py <baseline-root> <candidate-root> "
                         "<output-json> [--require-no-sign-flips]")
    baseline = Path(sys.argv[1])
    candidate_root = Path(sys.argv[2])
    folders = sorted(baseline.glob("rock-*/lod-*/bake.json"))
    candidate_folders = sorted(candidate_root.glob("rock-*/lod-*/bake.json"))
    if len(folders) != EXPECTED_CASES or {
        path.parent.relative_to(baseline) for path in folders
    } != {
        path.parent.relative_to(candidate_root) for path in candidate_folders
    }:
        raise SystemExit(f"expected the same {EXPECTED_CASES} paired bakes")
    cases = [compare(path.parent, candidate_root / path.parent.relative_to(baseline))
             for path in folders]
    report = {
        "scope": "paired source-field relaxation diagnostic; identical high/low geometry and UV; "
                 "texture-space variation is not perceptual Beauty or moving-runtime acceptance",
        "scriptSha256": digest(Path(__file__)),
        "cases": cases,
    }
    Path(sys.argv[3]).write_text(json.dumps(report, indent=2) + "\n", encoding="utf-8")
    if len(sys.argv) == 5 and any(
        case["maps"]["signed-curvature"]["signFlipFraction"] > 0 or
        any(value > 0 for value in case["maps"]["signed-curvature"]["mipSignFlipFraction"].values())
        for case in cases
    ):
        raise SystemExit("curvature relaxation reversed source bake polarity")
    print(f"PASS: compared {len(cases)} paired bakes")
