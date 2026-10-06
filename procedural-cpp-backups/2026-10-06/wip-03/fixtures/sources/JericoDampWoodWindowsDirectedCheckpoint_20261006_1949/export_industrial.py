"""Export validated milestone props to content-addressed portable PBR files."""
import sys,json,hashlib,tempfile
from pathlib import Path
ROOT=Path(__file__).resolve().parent;sys.path.insert(0,str(ROOT))
from industrial import build_industrial
from core import validate_scene,asset_hash
from export_assets import export_asset,_freeze

def main():
 out=ROOT/'output/exports_verified';out.mkdir(parents=True,exist_ok=True);records=[]
 names=['industrial_solid','industrial_louvered','industrial_vented','industrial_window_intact','industrial_window_broken','industrial_window_empty','industrial_damage_left','industrial_edge_damage','industrial_edges_cracks']
 for name in names:
  recipe=json.loads((ROOT/'recipes'/(name+'.json')).read_text());scene=build_industrial(recipe)
  errors=validate_scene(scene)
  if errors:raise ValueError(errors)
  print('BEGIN '+name,flush=True)
  record=export_asset(scene,out/name,name,texture_size=1024,bake_samples=2);record['source_asset_hash']=asset_hash(scene);records.append(record)
  print('COMPLETE '+json.dumps(record),flush=True)
 path=out/'industrial-export-manifest-working.json';path.write_text(json.dumps(records,indent=2)+'\n');print('MANIFEST '+str(_freeze(path,out,'industrial-export-manifest')),flush=True)
if __name__=='__main__':main()
