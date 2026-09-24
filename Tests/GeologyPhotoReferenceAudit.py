"""Verify retained public-domain geology photographs against recorded originals."""

import hashlib
import json
import sys
from pathlib import Path
from urllib.parse import urlparse

from PIL import Image


def audit(root: Path) -> dict:
    manifest = json.loads((root / "manifest.json").read_text())
    cases = []
    assert len(manifest["references"]) == 4
    for item in manifest["references"]:
        path = root / item["file"]
        assert path.is_file()
        digest = hashlib.sha256(path.read_bytes()).hexdigest()
        assert digest == item["sha256"]
        with Image.open(path) as image:
            assert list(image.size) == item["pixels"]
            assert image.format == "JPEG"
        assert urlparse(item["sourcePage"]).hostname == "www.usgs.gov"
        assert urlparse(item["originalUrl"]).hostname == "d9-wret.s3.us-west-2.amazonaws.com"
        cases.append({"file": item["file"], "sha256": digest,
                      "pixels": item["pixels"], "sourcePage": item["sourcePage"]})
    return {"count": len(cases), "cases": cases,
            "limit": "Page rights were checked manually on USGS; this checks retained bytes and attribution URLs."}


if __name__ == "__main__":
    result = audit(Path(sys.argv[1]))
    Path(sys.argv[2]).write_text(json.dumps(result, indent=2) + "\n")
    print("PASS: four USGS geology reference photographs")
