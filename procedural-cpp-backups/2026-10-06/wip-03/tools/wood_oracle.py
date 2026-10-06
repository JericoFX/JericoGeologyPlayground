#!/usr/bin/env python3
"""Offline fixture exporter from frozen Python source; never used by native targets."""
import argparse, hashlib, json, sys
from pathlib import Path
p=argparse.ArgumentParser();p.add_argument('--source',type=Path,required=True);p.add_argument('--out',type=Path,required=True);a=p.parse_args()
source=a.source.resolve();out=a.out.resolve();out.mkdir(parents=True,exist_ok=True);sys.path.insert(0,str(source))
from presets import build_preset
from singlepane import build_singlepane
from core import validate_scene
cases=[]
for kind in ('door','window'):
 for preset in ('mine_timber','reinforced_timber'):
  for damage in ('aged','deteriorated','broken'):
   cases.append((f'{preset}_{kind}_{damage}', 'wood',dict(kind=kind,preset=preset,damage=damage,seed=19,angle=0)))
for kind in ('door','window'):
 for label,recipe in [('small',dict(width=.65,height=.8,thickness=.028,seed=-9,handle='pull',angle=64)),('large',dict(width=2.4,height=3.2,thickness=.12,seed=2**40+17,handle='lever',angle=77))]:
  cases.append((f'wood_{kind}_{label}','wood',dict(recipe,kind=kind,preset='reinforced_timber')))
for glazing in ('intact','broken','empty'):
 for label,recipe in [('base',dict(seed=17)),('small',dict(width=.65,height=.8,thickness=.028,seed=2)),('large',dict(width=2.4,height=3.2,thickness=.12,seed=-71))]:
  cases.append((f'singlepane_{glazing}_{label}','singlepane',dict(recipe,preset='single_pane_fixed',glazing=glazing)))
manifest=[]
for name,family,recipe in cases:
 scene=(build_preset if family=='wood' else build_singlepane)(recipe)
 raw=json.dumps(scene,sort_keys=True,separators=(',',':'),allow_nan=False).encode();filename=name+'.json';(out/filename).write_bytes(raw)
 manifest.append(dict(file=filename,recipe=recipe,family=family,sha256=hashlib.sha256(raw).hexdigest(),source_validation_errors=validate_scene(scene),parts=len(scene['parts'])))
(out/'manifest.json').write_text(json.dumps(manifest,indent=2)+'\n')
(out/'oracle-provenance.json').write_text(json.dumps(dict(source_snapshot=source.name,source_sha256={name:hashlib.sha256((source/name).read_bytes()).hexdigest() for name in ('core.py','presets.py','singlepane.py')},recipe_count=len(manifest),scope='Offline reference only; no Python runtime in native generation or tests'),indent=2)+'\n')
print('Wrote',len(manifest),'frozen wood/singlepane reference scenes')
print('Source validation errors:',[(r['file'],r['source_validation_errors']) for r in manifest if r['source_validation_errors']])
