#!/usr/bin/env bash
set -euo pipefail
cd "$(dirname "$0")/.."
bash scripts/build.sh
build/core_tests
python3 -m unittest discover -s tests -v
python3 scripts/test_render_contract.py --blender
