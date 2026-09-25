"""Compare every retained cave image and mesh across two same-binary runs."""

import hashlib
import json
import sys
from pathlib import Path


def artifacts(root: Path) -> dict[str, Path]:
    return {str(path.relative_to(root)).replace("\\", "/"): path
            for path in root.rglob("*") if path.is_file() and path.suffix in {".png", ".obj"}}


def digest(path: Path) -> str:
    return hashlib.sha256(path.read_bytes()).hexdigest()


if __name__ == "__main__":
    first, second, output = map(Path, sys.argv[1:4])
    left, right = artifacts(first), artifacts(second)
    assert left.keys() == right.keys() and left, "Cave artifact sets differ or are empty"
    hashes = {name: {"first": digest(left[name]), "second": digest(right[name])}
              for name in sorted(left)}
    mismatched = [name for name, pair in hashes.items() if pair["first"] != pair["second"]]
    result = {"comparedPngObj": len(hashes), "mismatched": mismatched, "sha256": hashes}
    output.write_text(json.dumps(result, indent=2) + "\n")
    assert not mismatched, "Cave artifact bytes changed between same-binary runs"
    print(f"PASS: {len(hashes)} identical PNG/OBJ cave artifacts")
