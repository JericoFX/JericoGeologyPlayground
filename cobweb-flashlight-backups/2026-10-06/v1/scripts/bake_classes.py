#!/usr/bin/env python3
"""Bake independent, light-invariant class assets using unmodified dense-v3r3 code."""
import json
from pathlib import Path
import subprocess
from contract import CLASSES,FIXED,asset_identity,require_new
ROOT=Path(__file__).resolve().parents[1]
GENERATOR=ROOT/'vendor/build/cobweb_lab'
if not GENERATOR.exists(): GENERATOR=ROOT/'vendor/cobweb_lab'
if not GENERATOR.exists(): raise FileNotFoundError('Run bash scripts/build_generator.sh first')
for c in CLASSES:
    out=ROOT/'assets'/c['id']
    require_new([out])
    subprocess.run([str(GENERATOR),'--preset','corridor','--seed','42','--density',str(c['density']),
      '--width',str(c['width']),'--broken','.18','--layers','2','--atlas',str(FIXED['atlas_size']),'--out',str(out)],check=True)
identities={c['id']:asset_identity(ROOT/'assets'/c['id']) for c in CLASSES}
assert len(set(v['mesh.obj'] for v in identities.values()))==1,'All class carrier meshes must match'
(ROOT/'assets/identities.json').write_text(json.dumps(identities,indent=2)+'\n')
print(json.dumps(identities,indent=2))
