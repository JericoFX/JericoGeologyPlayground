#!/usr/bin/env bash
set -euo pipefail
cd "$(dirname "$0")/.."
python -m unittest discover -s tests -v
blender -b --python-exit-code 1 --python-expr "import sys,unittest; sys.path.insert(0,'.'); r=unittest.TextTestRunner(verbosity=2).run(unittest.defaultTestLoader.discover('tests')); sys.exit(not r.wasSuccessful())"
