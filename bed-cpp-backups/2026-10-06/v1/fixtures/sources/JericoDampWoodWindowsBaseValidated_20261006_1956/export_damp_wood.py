"""Portable GLB, OBJ, JSON and baked PBR for the two damp-wood families."""
import sys,json,zipfile
from pathlib import Path
ROOT=Path(__file__).resolve().parent;sys.path.insert(0,str(ROOT))
from damp_wood import build_damp_window
from export_assets import export_asset
from core import validate_scene,asset_hash
from evidence import freeze_file,freeze_manifest

def main():
 out=ROOT/'output/exports';out.mkdir(parents=True,exist_ok=True);records=[]
 for family in ('old','new'):
  for state in ('intact','broken','empty'):
   label='damp_wood_'+family+'_'+state;scene=build_damp_window(dict(family=family,glazing=state,seed=29));errors=validate_scene(scene)
   if errors:raise ValueError(errors)
   print('BEGIN '+label,flush=True)
   result=export_asset(scene,out/label,label,texture_size=1024,bake_samples=2);result.update(family=family,state=state,asset_hash=asset_hash(scene))
   files=[Path(result[k]) for k in ('glb','obj','mtl','metadata')]+[Path(p) for p in result['textures'].values()]
   path=out/(label+'-working.zip')
   with zipfile.ZipFile(path,'w',zipfile.ZIP_DEFLATED) as z:
    for f in files:z.write(f,f.name)
   if path.stat().st_size>=20_000_000:raise ValueError('Package exceeds 20 MB')
   result['package']=freeze_file(path,label+'-portable');records.append(result)
   (out/'export-records.json').write_text(json.dumps(records,indent=2));print('STATE_COMPLETE '+json.dumps(result),flush=True)
 print('FINAL '+str(freeze_manifest(records,out)),flush=True)
if __name__=='__main__':main()
