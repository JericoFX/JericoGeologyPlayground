#!/usr/bin/env python3
"""Bounded orchestration; all geometry/validation stays in portable C++."""
import json,pathlib,subprocess,statistics,time,platform,argparse
p=argparse.ArgumentParser();p.add_argument('--binary',default='build/rocks_cli');p.add_argument('--out',default='out/evidence-v3');a=p.parse_args();binary=pathlib.Path(a.binary).resolve();out=pathlib.Path(a.out);out.mkdir(parents=True,exist_ok=True)
summary={'platform':platform.platform(),'compiler':subprocess.run(['g++','--version'],capture_output=True,text=True).stdout,'flags':'CMake Release -O3 -DNDEBUG C++20 no fast-math','rss':'unavailable; core capacities are not RSS','route':'poly-csg-v1 floating-point plane arrangement; grid independent; wear0','families':{},'mcDiagnostics':[]}
for family in ['fractured_wall','irregular_beds','overhang']:
 summary['families'][family]={}
 for detail in [0,1]:
  rp=pathlib.Path('recipes')/(family+('-noise' if detail else '')+'-v3.json');dest=out/family/str(detail);runs=[]
  for repeat in range(6):
   start=time.perf_counter();r=subprocess.run([str(binary),'generate','--recipe',str(rp),'--out',str(dest)],capture_output=True,text=True,check=True);wall=1000*(time.perf_counter()-start);m=json.loads((dest/'metrics.json').read_text())
   if repeat:runs.append({'wallMs':wall,'metrics':m})
  summary['families'][family][str(detail)]={'runs':runs,'wallMs':{'median':statistics.median(x['wallMs'] for x in runs),'min':min(x['wallMs'] for x in runs),'max':max(x['wallMs'] for x in runs)},'geometry':runs[-1]['metrics']['stages']['stratified']}
  print(f'{family} detail{detail}: accepted, median {summary["families"][family][str(detail)]["wallMs"]["median"]:.1f}ms',flush=True)
 # MC is retained as a comparison. Rejection is a reported result, not accepted output.
 base=json.loads((pathlib.Path('recipes')/(family+'-v3.json')).read_text())
 for cells in [64,128]:
  recipe={**base,'extractorVersion':'paris-mc-v1','grid':{'cells':cells}};rp=out/f'{family}-mc-{cells}.json';rp.write_text(json.dumps(recipe));r=subprocess.run([str(binary),'generate','--recipe',str(rp),'--out',str(out/f'{family}-mc-{cells}')],capture_output=True,text=True);summary['mcDiagnostics'].append({'family':family,'cells':cells,'returnCode':r.returncode,'stderr':r.stderr,'accepted':r.returncode==0})
summary['fieldFidelity']='Every direct-polyhedral triangle centroid tested on both sides against original field; sampling is not a universal Boolean/self-intersection proof.'
(out/'summary.json').write_text(json.dumps(summary,indent=2));print(out/'summary.json')
