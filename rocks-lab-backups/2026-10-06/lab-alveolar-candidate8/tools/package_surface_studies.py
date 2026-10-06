#!/usr/bin/env python3
from pathlib import Path
import zipfile,hashlib,json,argparse
ROOT=Path(__file__).resolve().parents[1]
def package(dest,include_renders=False):
 files=[]
 for p in ROOT.rglob('*'):
  if not p.is_file():continue
  rel=p.relative_to(ROOT);parts=rel.parts
  if any(k in parts for k in ['__pycache__','.git','build','out']):continue
  if parts[0]=='renders' and not include_renders and p.suffix not in ('.json','.md'):continue
  if p.suffix=='.obj':continue
  files.append(p)
 with zipfile.ZipFile(dest,'w',zipfile.ZIP_DEFLATED,6) as z:
  for p in sorted(files):z.write(p,p.relative_to(ROOT))
 with zipfile.ZipFile(dest) as z:assert z.testzip() is None
 return {'path':str(dest),'bytes':dest.stat().st_size,'sha256':hashlib.sha256(dest.read_bytes()).hexdigest(),'files':len(files)}
if __name__=='__main__':
 a=argparse.ArgumentParser();a.add_argument('destination',type=Path);a.add_argument('--renders',action='store_true');args=a.parse_args();args.destination.parent.mkdir(parents=True,exist_ok=True);print(json.dumps(package(args.destination,args.renders),indent=2))
