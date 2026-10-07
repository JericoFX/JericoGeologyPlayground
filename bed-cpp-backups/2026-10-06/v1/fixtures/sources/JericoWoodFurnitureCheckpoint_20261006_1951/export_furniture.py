import sys,json,argparse
from pathlib import Path
ROOT=Path(__file__).resolve().parent;sys.path.insert(0,str(ROOT))
from furniture import build_furniture,validate_furniture,asset_hash,sweep_clearance
from export_assets import export_asset,_freeze
p=argparse.ArgumentParser();p.add_argument('--preset',choices=['cabinet','workbench'],required=True);p.add_argument('--texture-size',type=int,default=2048);a=p.parse_args(sys.argv[sys.argv.index('--')+1:])
out=ROOT/'output/exports'/a.preset;out.mkdir(parents=True,exist_ok=True)
s=build_furniture(dict(preset=a.preset));errors=validate_furniture(s)
if errors:raise ValueError(errors)
record=export_asset(s,out,a.preset,texture_size=a.texture_size,bake_samples=2);record['source_hash']=asset_hash(s);record['clearance']=sweep_clearance(s)
f=out/'export-record-working.json';f.write_text(json.dumps(record,indent=2)+'\n');print('EXPORT_RECORD '+str(_freeze(f,out,'export-record')),flush=True)
