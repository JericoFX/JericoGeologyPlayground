#!/usr/bin/env python3
"""Offline JSON reference exporter only; never used by native targets."""
import argparse, hashlib, json, sys
from pathlib import Path
p=argparse.ArgumentParser();p.add_argument('--source',type=Path,required=True);p.add_argument('--out',type=Path,required=True);a=p.parse_args()
source=a.source.resolve();out=a.out.resolve();out.mkdir(parents=True,exist_ok=True);sys.path.insert(0,str(source))
from damp_wood import build_damp_window
from core import validate_scene
cases=[]
for family in ('old','new'):
 for glazing in ('intact','broken','empty'):
  cases.append((f'{family}_{glazing}',dict(family=family,glazing=glazing)))
for label,recipe in [
 ('age_zero',dict(age=0,wetness=0,seed=0)),
 ('age_threshold',dict(age=.35,wetness=1,seed=59)),
 ('age_above_threshold',dict(age=.350001,wetness=.3,seed=59)),
 ('age_maximum',dict(age=1,wetness=1,seed=-15)),
 ('new_broken_small',dict(family='new',glazing='broken',width=.65,height=.8,thickness=.028,seed=17)),
 ('old_large',dict(family='old',width=2.4,height=3.2,thickness=.12,seed=2**40+17)),
]:cases.append((label,recipe))
manifest=[]
for name,recipe in cases:
 scene=build_damp_window(recipe);raw=json.dumps(scene,sort_keys=True,separators=(',',':'),allow_nan=False).encode();filename=name+'.json';(out/filename).write_bytes(raw)
 manifest.append(dict(file=filename,recipe=recipe,sha256=hashlib.sha256(raw).hexdigest(),source_validation_errors=validate_scene(scene),parts=len(scene['parts'])))
(out/'manifest.json').write_text(json.dumps(manifest,indent=2)+'\n')
(out/'oracle-provenance.json').write_text(json.dumps(dict(source_snapshot=source.name,source_sha256={name:hashlib.sha256((source/name).read_bytes()).hexdigest() for name in ('damp_wood.py','core.py','singlepane.py','glass_fracture.py','damage_math.py','crack_math.py')},recipe_count=len(manifest),status='Development reference; visual approval and exporter tangent issue are separate gates. No bpy/export behavior is used.',scope='Offline reference only; no Python runtime in native generation or tests'),indent=2)+'\n')
print('Wrote',len(manifest),'damp-wood source scenes');print('Source validation errors:',[(r['file'],r['source_validation_errors']) for r in manifest if r['source_validation_errors']])
