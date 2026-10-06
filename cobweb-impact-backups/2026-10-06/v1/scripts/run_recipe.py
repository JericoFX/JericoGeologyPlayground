#!/usr/bin/env python3
"""Run a source-controlled impact recipe using the local compiled demo."""
import argparse
import json
from pathlib import Path
import subprocess

parser = argparse.ArgumentParser(description=__doc__)
parser.add_argument('recipe', type=Path)
parser.add_argument('--out', type=Path, required=True)
args = parser.parse_args()
recipe = json.loads(args.recipe.read_text())
required = {'seed', 'breakable', 'score', 'threshold', 'wind'}
if set(recipe) != required or not isinstance(recipe['breakable'], bool):
    parser.error('Recipe must contain only seed, breakable (boolean), score, threshold and wind')
root = Path(__file__).resolve().parents[1]
command = [str(root/'build'/'impact_demo'), '--out', str(args.out)]
for name in sorted(required):
    value = recipe[name]
    if name == 'breakable':
        value = int(value)
    command += ['--'+name, str(value)]
raise SystemExit(subprocess.run(command).returncode)
