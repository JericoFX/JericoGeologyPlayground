import argparse,json,subprocess
from pathlib import Path
ROOT=Path(__file__).resolve().parents[1]
p=argparse.ArgumentParser();p.add_argument('--mode',choices=['reference','native'],required=True);p.add_argument('--material',action='store_true');p.add_argument('--bake',type=int,default=1024);p.add_argument('--samples',type=int,default=32);p.add_argument('--scale',type=float,default=.5);p.add_argument('--only',default='');a=p.parse_args()
folder=ROOT/'output/family-visual';cases=json.loads((folder/'cases.json').read_text())
for c in cases:
 if a.only and c['label'] not in a.only.split(','):continue
 label=c['label'];d=folder/label;d.mkdir(parents=True,exist_ok=True);s=d/'native-scene.json'
 if a.mode=='native':
  cmd=[str(ROOT/'build/jerico_procedural'),'--family',c['family'],'--recipe',c['recipe'],'--out',str(s),'--evaluate','1']
  if a.material:cmd+=['--bake',str(a.bake),'--glb',str(d/'native.glb')]
  result=subprocess.run(cmd,text=True,capture_output=True);(d/'native-generation.log').write_text(result.stdout+result.stderr)
  if result.returncode:print('GENERATION FAILED',label,result.stderr,flush=True);continue
 out=d/(a.mode+('-material' if a.material else '-clay')+'.png');cmd=['blender','-b','--threads','4','--python-exit-code','1','--python',str(ROOT/'tools/render_family_compare.py'),'--','--mode',a.mode,'--family',c['family'],'--recipe',c['recipe'],'--out',str(out),'--samples',str(a.samples),'--scale',str(a.scale)]
 if not a.material:cmd+=['--clay']
 if a.mode=='native':cmd+=['--scene',str(s)]
 with out.with_suffix('.log').open('w') as log:result=subprocess.run(cmd,stdout=log,stderr=subprocess.STDOUT)
 print('RENDER',label,result.returncode,flush=True)
