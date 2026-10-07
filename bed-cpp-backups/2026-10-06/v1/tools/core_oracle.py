"""Offline Python oracle only. It is never linked or invoked by the generator."""
import argparse,json,subprocess,sys,tempfile
from pathlib import Path
ROOT=Path(__file__).resolve().parents[1]
sys.path.insert(0,str(ROOT/'fixtures/sources/JericoGlassFractureStudy'))
from industrial import build_industrial

def compare(a,b,path='root',diffs=None):
 diffs=[] if diffs is None else diffs
 if path=='root.metadata.generator':return diffs
 if isinstance(a,bool) or isinstance(b,bool):
  if a!=b:diffs.append((path,a,b))
 elif isinstance(a,(int,float)) and isinstance(b,(int,float)):
  if abs(a-b)>2e-12:diffs.append((path,a,b))
 elif isinstance(a,dict) and isinstance(b,dict):
  if a.keys()!=b.keys():diffs.append((path+'.keys',sorted(a),sorted(b)))
  for k in a.keys()&b.keys():compare(a[k],b[k],path+'.'+k,diffs)
 elif isinstance(a,list) and isinstance(b,list):
  if len(a)!=len(b):diffs.append((path+'.length',len(a),len(b)))
  for i,(x,y) in enumerate(zip(a,b)):compare(x,y,path+f'[{i}]',diffs)
 elif a!=b:diffs.append((path,a,b))
 return diffs

def main():
 p=argparse.ArgumentParser();p.add_argument('--exe',default=str(ROOT/'build/jerico_procedural'));args=p.parse_args()
 cases=[]
 for seed in [0,17,29,107,2147483647]:
  for width,height in [(.65,.8),(1.16,1.25),(2.4,3.2)]:
   for glazing in ['intact','empty']:
    cases.append(dict(preset='industrial_window',kind='window',seed=seed,width=width,height=height,glazing=glazing))
 for i,r in enumerate(cases):
  with tempfile.TemporaryDirectory() as d:
   inp=Path(d)/'recipe.json';out=Path(d)/'scene.json';inp.write_text(json.dumps(r));subprocess.run([args.exe,'--family','industrial','--recipe',str(inp),'--out',str(out)],check=True,stdout=subprocess.DEVNULL)
   diffs=compare(build_industrial(r),json.loads(out.read_text()))
   if diffs:raise AssertionError(json.dumps({'case':r,'diffs':diffs[:20]},indent=2))
 print(f'{len(cases)} industrial fixed-window source scenes match: topology, positions, metric UVs, material groups, sockets and metadata at 2e-12')
if __name__=='__main__':main()
