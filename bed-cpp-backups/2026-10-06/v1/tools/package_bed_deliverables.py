"""Lossless packaging only; never generates or alters geometry/images."""
from pathlib import Path
import json,zipfile,hashlib
root=Path(__file__).resolve().parents[1];delivery=root/'delivery';delivery.mkdir(exist_ok=True)
def archive(path,entries):
 with zipfile.ZipFile(path,'w',zipfile.ZIP_DEFLATED,compresslevel=9) as z:
  for source,name in entries:z.write(source,name)
 with zipfile.ZipFile(path) as z:assert z.testzip() is None
 return path.stat().st_size
records=[]
for kind in ['GLB','OBJ']:
 allentries=[];groups={}
 for preset in ['iron_only','bedding','complete']:
  d=root/'output'/preset;entries=[]
  if kind=='GLB':entries=[(d/'bed.glb',f'{preset}/bed.glb')]
  else:entries=[(d/name,f'{preset}/{name}') for name in ['bed.obj','bed.mtl','bed.README.txt']]+[(p,f'{preset}/bed_textures/{p.name}') for p in sorted((d/'bed_textures').glob('*.png'))]
  entries += [(root/'recipes/bed'/f'{preset}.json',f'{preset}/recipe.json'),(d/'report.json',f'{preset}/report.json')]
  groups[preset]=entries;allentries+=entries
 allentries += [(root/'README_BED.md','README.md')]
 path=delivery/f'JericoBedCpp-{kind}-3presets-512px.zip';size=archive(path,allentries)
 if size>20_000_000:
  path.unlink()
  for preset,entries in groups.items():
   path=delivery/f'JericoBedCpp-{kind}-{preset}-512px.zip';size=archive(path,entries+[(root/'README_BED.md','README.md')]);assert size<20_000_000;records.append({'file':path.name,'bytes':size,'sha256':hashlib.sha256(path.read_bytes()).hexdigest()})
 else:records.append({'file':path.name,'bytes':size,'sha256':hashlib.sha256(path.read_bytes()).hexdigest()})
(delivery/'asset-package-sizes.json').write_text(json.dumps(records,indent=2));print(json.dumps(records,indent=2))
