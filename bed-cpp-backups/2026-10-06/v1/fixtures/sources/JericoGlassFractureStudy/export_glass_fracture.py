"""Build three exact-compatible glazing states with baked portable PBR."""
import sys,json,hashlib,zipfile
from pathlib import Path
ROOT=Path(__file__).resolve().parent;sys.path.insert(0,str(ROOT))
from glass_fracture import build_fracture
from export_assets import export_asset
from core import validate_scene,asset_hash
from evidence import freeze_file,freeze_manifest

def main():
 out=ROOT/'output/fracture/exports';out.mkdir(parents=True,exist_ok=True);records=[]
 recipe=json.loads((ROOT/'recipes/industrial_window_broken.json').read_text())
 for state in ['broken','intact','empty']:
  current=dict(recipe,glazing=state);scene=build_fracture(current);errors=validate_scene(scene)
  if errors:raise ValueError(errors)
  result=export_asset(scene,out/state,'glass_'+state,texture_size=512,bake_samples=2);result['asset_hash']=asset_hash(scene);result['state']=state
  files=[Path(result[k]) for k in ['glb','obj','mtl','metadata']]+[Path(p) for p in result['textures'].values()]
  path=out/('glass-'+state+'-working.zip')
  with zipfile.ZipFile(path,'w',zipfile.ZIP_DEFLATED) as z:
   for f in files:z.write(f,f.name)
  if path.stat().st_size>=20_000_000:raise ValueError('Package exceeds 20 MB')
  result['package']=freeze_file(path,'glass-'+state+'-portable');records.append(result)
  print('STATE_COMPLETE '+json.dumps(result),flush=True)
 (out/'export-records.json').write_text(json.dumps(records,indent=2));print('FINAL '+str(freeze_manifest(records,out)),flush=True)
if __name__=='__main__':main()
