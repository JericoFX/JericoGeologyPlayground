"""Compare two complete bake packages from the same source and binary."""

import hashlib
import json
import sys
from pathlib import Path


TIMING_KEYS = {"cpuMs", "bakeCpuMs"}


def without_timing(value):
    if isinstance(value, dict):
        return {key: without_timing(item) for key, item in value.items()
                if key not in TIMING_KEYS}
    if isinstance(value, list):
        return [without_timing(item) for item in value]
    return value


def files(root: Path) -> set[Path]:
    return {path.relative_to(root) for path in root.rglob("*") if path.is_file()}


def audit(first: Path, second: Path) -> dict:
    paths = files(first)
    assert paths and paths == files(second), "replay file sets differ"
    extensions = {}
    package_digest = hashlib.sha256()
    for relative in sorted(paths):
        left, right = first / relative, second / relative
        if relative.suffix == ".json":
            data = without_timing(json.loads(left.read_text()))
            assert data == without_timing(json.loads(right.read_text())), \
                f"metadata differs: {relative}"
            content = json.dumps(data, sort_keys=True, separators=(",", ":")).encode()
        else:
            content = left.read_bytes()
            assert hashlib.sha256(content).digest() == \
                   hashlib.sha256(right.read_bytes()).digest(), f"artifact differs: {relative}"
        package_digest.update(relative.as_posix().encode() + b"\0")
        package_digest.update(hashlib.sha256(content).digest())
        extensions[relative.suffix] = extensions.get(relative.suffix, 0) + 1
    return {"sameBinaryRequired": True, "excludedTimingFields": sorted(TIMING_KEYS),
            "identicalArtifacts": len(paths), "byExtension": extensions,
            "canonicalPackageSha256": package_digest.hexdigest(),
            "note": "Compare only runs from the same binary and inputs; CPU time is excluded."}


if __name__ == "__main__":
    result = audit(Path(sys.argv[1]), Path(sys.argv[2]))
    Path(sys.argv[3]).write_text(json.dumps(result, indent=2) + "\n")
    print(f"PASS: {result['identicalArtifacts']} replay artifacts match")
