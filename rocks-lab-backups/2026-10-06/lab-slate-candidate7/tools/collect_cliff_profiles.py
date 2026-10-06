#!/usr/bin/env python3
"""Reproduce versioned C++ profile fixtures and numerical controls; no image input."""
import argparse,copy,hashlib,json,pathlib,subprocess
ROOT=pathlib.Path(__file__).resolve().parents[1]
def collect(binary,out):
 report={'method':'Actual exported C++ meshes and uint16 samples; fixed scalar mapping. Numerical diagnostics are not a naturalness score.','profiles':{}}
 for profile in ['massive_oblique','vertical_channels']:
  recipe=json.loads((ROOT/f'recipes/heightfield-{profile.replace("_","-")}-48-v11.json').read_text())
  variants={f'seed-{seed}':{**recipe,'seed':seed} for seed in [48,117,305]}
  for key,value in [('directionDegrees',30),('jointSpacing',.12),('jointDepth',0),('blockScale',.45),('fractures',.3),('irregularity',0),('relief',.45),('size',3),('backDepth',.5)]:variants[key]={**recipe,key:value}
  variants['mesh-resolution']={**recipe,'grid':{**recipe['grid'],'cells':64}}
  base=None;codes=None;items={}
  for name,r in variants.items():
   folder=out/profile/name;folder.mkdir(parents=True,exist_ok=True);source=folder/'input.json';source.write_text(json.dumps(r,indent=2))
   result=subprocess.run([str(binary),'generate','--recipe',str(source),'--out',str(folder)],capture_output=True,text=True)
   if result.returncode:raise RuntimeError(f'{profile}/{name}: {result.stderr}')
   positions=json.loads((folder/'viewer.json').read_text())['stages']['heightfield']['positions'];raw=(folder/'height.u16').read_bytes();front=(r['grid']['cells']+1)**2;z=positions[2:front*3:3]
   if base is None:base=positions;codes=raw
   metrics=json.loads((folder/'metrics.json').read_text())
   item={'recipe':r,'objSha256':hashlib.sha256((folder/'heightfield.obj').read_bytes()).hexdigest(),'samplesSha256':hashlib.sha256(raw).hexdigest(),'frontZMin':min(z),'frontZMax':max(z),'frontPeakToValleyMeters':max(z)-min(z),'changedSourceSamples':sum(raw[i:i+2]!=codes[i:i+2] for i in range(0,len(raw),2)),'metrics':metrics}
   if len(positions)==len(base):item['changedVertices']=sum(positions[i:i+3]!=base[i:i+3] for i in range(0,len(base),3))
   items[name]=item
  report['profiles'][profile]=items
 (out/'report.json').write_text(json.dumps(report,indent=2)+'\n');return report
if __name__=='__main__':
 parser=argparse.ArgumentParser();parser.add_argument('--binary',type=pathlib.Path,default=ROOT/'build/rocks_cli');parser.add_argument('--out',type=pathlib.Path,default=ROOT/'out/heightfield-v11-controls');args=parser.parse_args();collect(args.binary,args.out);print(args.out/'report.json')
