#!/usr/bin/env python3
"""Generate actual C++ surface-family outputs and matched recipe controls."""
from pathlib import Path
import json,subprocess,hashlib,argparse,copy
ROOT=Path(__file__).resolve().parents[1]
PROFILES=('shale_laminated','slate_foliated','limestone_dissolution')
def main_recipe(profile,seed):
 r={'schemaVersion':3,'generatorVersion':'surface-families-v13','extractorVersion':'heightfield-solid-v2','family':'heightfield_cliff','units':'m','profile':profile,'seed':seed,'size':.64,'relief':.12,'backDepth':.075,'directionDegrees':0,'featureDepth':.85,'fractures':.7,'irregularity':.65,'grid':{'cells':256,'width':1025,'height':1025}}
 if profile=='shale_laminated':r.update(directionDegrees=4,layerSpacingMeters=.014,plateLengthMeters=.14,thicknessVariation=.8)
 elif profile=='slate_foliated':r.update(directionDegrees=-58,layerSpacingMeters=.075,plateLengthMeters=.28,thicknessVariation=.4)
 else:r.update(size=2.,relief=.55,backDepth=.3,directionDegrees=0,channelSpacingMeters=.32,channelWidthMeters=.075,pocketRadiusMeters=.12,pocketCount=3)
 return r

def legacy_recipe(r):
 p=r['profile'];name={'shale_laminated':'heightfield-cliff-48-v10.json','slate_foliated':'heightfield-massive-oblique-48-v11.json','limestone_dissolution':'heightfield-vertical-channels-48-v11.json'}[p]
 old=json.loads((ROOT/'recipes'/name).read_text())
 for k in ('seed','size','relief','backDepth','grid'):old[k]=r[k]
 return old

def generate(r,name,replay=True):
 dest=ROOT/'out/surface-v13'/name;dest.mkdir(parents=True,exist_ok=True)
 recipe=ROOT/'recipes'/f'{name}-v13.json';recipe.write_text(json.dumps(r,indent=2)+'\n')
 run=subprocess.run([str(ROOT/'build/rocks_cli'),'generate','--recipe',str(recipe),'--out',str(dest)],text=True,capture_output=True)
 if run.returncode:raise RuntimeError(name+': '+run.stderr)
 files=['heightfield.obj','height.u16','viewer.json','recipe.json']
 hashes={p:hashlib.sha256((dest/p).read_bytes()).hexdigest() for p in files}
 if replay:
  again=ROOT/'out/surface-v13-replay'/name
  run=subprocess.run([str(ROOT/'build/rocks_cli'),'generate','--recipe',str(recipe),'--out',str(again)],text=True,capture_output=True,cwd='/tmp')
  if run.returncode:raise RuntimeError(run.stderr)
  for p,h in hashes.items():assert hashlib.sha256((again/p).read_bytes()).hexdigest()==h,(name,p)
 return {'name':name,'recipe':r,'hashes':hashes,'metrics':json.loads((dest/'metrics.json').read_text()),'exactReplay':replay}

if __name__=='__main__':
 p=argparse.ArgumentParser();p.add_argument('--seeds',nargs='+',type=int,default=[48,117,305]);p.add_argument('--no-replay',action='store_true');a=p.parse_args();records=[]
 for seed in a.seeds:
  for profile in PROFILES:
   r=main_recipe(profile,seed)
   for name,recipe in [(f'{profile}-{seed}',r),(f'control-{profile}-{seed}',{**r,'featureDepth':0}),(f'legacy-{profile}-{seed}',legacy_recipe(r))]:
    print(name,flush=True);records.append(generate(recipe,name,not a.no_replay))
 out=ROOT/'evidence'/('generation-'+ '-'.join(map(str,a.seeds))+'.json');out.write_text(json.dumps(records,indent=2)+'\n');print(out)
