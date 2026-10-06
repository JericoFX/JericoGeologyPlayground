#!/usr/bin/env bash
set -euo pipefail
cd "$(dirname "$0")/.."
./build/impact_tests
python3 -m unittest discover -s scripts -p 'test_*.py' -v
python3 -m unittest discover -s tests -p 'test_*.py' -v
