#!/usr/bin/env python3
"""Offline reference oracle only. Never called by the C++ asset generator.

Compares every authoring field (geometry, triangulation, UVs, materials, IDs,
controls, support measurements and fracture metadata) with the frozen source.
Only metadata.generator is omitted because a native port has new provenance.
"""
import argparse
import json
import math
from pathlib import Path
import subprocess
import sys
import tempfile

ROOT = Path(__file__).resolve().parents[1]
SOURCE = ROOT / "fixtures/sources/JericoGlassFractureStudy"
sys.dont_write_bytecode = True
sys.path.insert(0, str(SOURCE))
from glass_fracture import build_fracture  # noqa: E402

TOLERANCE = 2e-12
BASE = dict(preset="industrial_window", kind="window", width=1.16,
            height=1.25, thickness=.052, damage="aged", finish="weathered_gray",
            glazing="broken", seed=29, angle=0)


def cases(sweep=False):
    result = []
    def add(name, updates, defaults=False):
        recipe = dict(preset="industrial_window", glazing="broken") if defaults else dict(BASE)
        recipe.update(updates)
        result.append((name, recipe))
    for state in ("broken", "intact", "empty"):
        add("default_" + state, dict(glazing=state), True)
        add("explicit_" + state, dict(glazing=state))
    for seed in (107, 5, 15, 35, 49, 53, 62, 69, 81, 87, -29, 2**40 + 17):
        add("seed_" + str(seed), dict(seed=seed, energy=1 if seed in (15, 35) else .66))
    add("seed_int64_max", dict(seed=2**63-1))
    add("seed_int64_min", dict(seed=-2**63))
    add("narrow_offcenter_fallback", dict(width=.65, height=.8, energy=.1,
                                          ray_count=12, impact=[.3, .25]))
    add("wide_max_rays", dict(width=2.4, height=.8, energy=1,
                              ray_count=40, impact=[-.3, .75]))
    add("tall_min_rays", dict(width=.65, height=3.2, energy=.1,
                              ray_count=12, impact=[-.3, .25]))
    add("max_dimensions", dict(width=2.4, height=3.2, thickness=.12,
                               energy=1, ray_count=40, impact=[.3, .75]))
    if sweep:
        for seed in range(10):
            for width, height in ((.65,.8), (1.16,1.25), (2.4,.8), (.65,3.2)):
                for energy in (.1, .66, 1):
                    add(f"sweep_{seed}_{width}_{height}_{energy}", dict(
                        seed=seed, width=width, height=height, energy=energy,
                        ray_count=(12,18,40)[seed % 3],
                        impact=([-.3,.25],[0,.5],[.3,.75])[seed % 3]))
    return result


def compare(expected, actual, path="$", stats=None):
    if stats is None:
        stats = dict(numbers=0, max_abs_error=0.0, max_error_path="")
    if isinstance(expected, dict):
        if not isinstance(actual, dict):
            raise AssertionError(f"{path}: expected object")
        ignored = {"generator"} if path == "$.metadata" else set()
        if set(expected) - ignored != set(actual) - ignored:
            raise AssertionError(f"{path}: keys differ: expected-only {set(expected)-set(actual)-ignored}; actual-only {set(actual)-set(expected)-ignored}")
        for key in expected:
            if key not in ignored:
                compare(expected[key], actual[key], f"{path}.{key}", stats)
    elif isinstance(expected, list):
        if not isinstance(actual, list) or len(actual) != len(expected):
            raise AssertionError(f"{path}: array length/type differs")
        for index, (a, b) in enumerate(zip(expected, actual)):
            compare(a, b, f"{path}[{index}]", stats)
    elif isinstance(expected, bool) or expected is None or isinstance(expected, str):
        if type(expected) is not type(actual) or expected != actual:
            raise AssertionError(f"{path}: {expected!r} != {actual!r}")
    else:
        if isinstance(actual, bool) or not isinstance(actual, (int,float)) or not math.isfinite(actual):
            raise AssertionError(f"{path}: invalid numeric value {actual!r}")
        # Indices and integer controls must be numerically exact, not merely close.
        if isinstance(expected, int) and isinstance(actual, int):
            error = abs(expected - actual)
            if error:
                raise AssertionError(f"{path}: integer {expected} != {actual}")
        else:
            error = abs(expected - actual)
        stats["numbers"] += 1
        if error > stats["max_abs_error"]:
            stats.update(max_abs_error=error, max_error_path=path)
        if error > TOLERANCE:
            raise AssertionError(f"{path}: absolute error {error:.17g}; {expected:.17g} != {actual:.17g}")
    return stats


def reference(sweep):
    result = []
    for name, recipe in cases(sweep):
        record = dict(name=name, recipe=recipe)
        try:
            record["scene"] = build_fracture(recipe)
        except ValueError as error:
            record["error"] = str(error)
        result.append(record)
    return result


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--exe", type=Path, help="Native jerico_procedural executable")
    parser.add_argument("--write-reference", type=Path)
    parser.add_argument("--sweep", action="store_true", help="Add 120 deterministic edge combinations")
    args = parser.parse_args()
    if not args.exe and not args.write_reference:
        parser.error("provide --exe and/or --write-reference")
    references = reference(args.sweep)
    if args.write_reference:
        args.write_reference.write_text(json.dumps(references, allow_nan=False))
    if not args.exe:
        print(json.dumps(dict(reference_cases=len(references), source=str(SOURCE))))
        return
    aggregate = dict(cases=len(references), passed=0, expected_rejections=0,
                     fallback_cases=0, numbers=0, max_abs_error=0.0,
                     max_error_path="", failures=[])
    with tempfile.TemporaryDirectory(prefix="jerico-fracture-oracle-") as folder:
        folder = Path(folder)
        for record in references:
            recipe, output = folder / "recipe.json", folder / "scene.json"
            recipe.write_text(json.dumps(record["recipe"], allow_nan=False))
            output.unlink(missing_ok=True)
            run = subprocess.run([str(args.exe.resolve()), "--family", "fracture",
                                  "--recipe", str(recipe), "--out", str(output)],
                                 text=True, capture_output=True)
            try:
                if "error" in record:
                    if run.returncode == 0:
                        raise AssertionError("native accepted source-rejected configuration")
                    if record["error"] not in run.stderr + run.stdout:
                        raise AssertionError(f"different rejection: {run.stderr or run.stdout}; expected {record['error']}")
                    aggregate["expected_rejections"] += 1
                else:
                    if run.returncode:
                        raise AssertionError(f"native failed ({run.returncode}): {run.stderr or run.stdout}")
                    actual = json.loads(output.read_text())
                    stats = compare(record["scene"], actual)
                    aggregate["numbers"] += stats["numbers"]
                    if stats["max_abs_error"] > aggregate["max_abs_error"]:
                        aggregate["max_abs_error"] = stats["max_abs_error"]
                        aggregate["max_error_path"] = record["name"] + ":" + stats["max_error_path"]
                    aggregate["fallback_cases"] += int("safety_fallback" in record["scene"]["metadata"].get("glass_fracture", {}))
                aggregate["passed"] += 1
            except (AssertionError, OSError, ValueError) as error:
                aggregate["failures"].append(dict(name=record["name"], error=str(error)))
    print(json.dumps(aggregate, indent=2))
    if aggregate["failures"]:
        raise SystemExit(1)


if __name__ == "__main__":
    main()
