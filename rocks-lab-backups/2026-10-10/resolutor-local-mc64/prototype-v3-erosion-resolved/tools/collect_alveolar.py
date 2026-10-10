#!/usr/bin/env python3
"""Isolated candidate8 evidence; never writes preserved candidates or historical outputs."""
from pathlib import Path
import argparse,copy,hashlib,json,math,subprocess,tempfile
ROOT=Path(__file__).resolve().parents[1]
BASE=ROOT.parent
OUT=ROOT/'out/alveolar-v14'
SEEDS=(48,117,305)

def sha(path):
 h=hashlib.sha256()
 with Path(path).open('rb') as f:
  for b in iter(lambda:f.read(1024*1024),b''):h.update(b)
 return h.hexdigest()

def geometric_error(a,b):
 if len(a)!=len(b) or len(a)%3:raise ValueError('Unmatched vertex counts')
 total=0.;maximum=0.
 for k in range(0,len(a),3):
  if a[k:k+2]!=b[k:k+2]:raise ValueError('Unmatched XY positions')
  d=abs(a[k+2]-b[k+2]);maximum=max(maximum,d);total+=d*d
 return {'maxMeters':maximum,'rmsMeters':math.sqrt(total/(len(a)//3)),'vertices':len(a)//3}

def save(path,value):path.parent.mkdir(parents=True,exist_ok=True);path.write_text(json.dumps(value,indent=2)+'\n')
def run(recipe,dest,binary=None):
 dest.mkdir(parents=True,exist_ok=True);path=dest/'input.json';save(path,recipe)
 p=subprocess.run([str(binary or ROOT/'build/rocks_cli'),'generate','--recipe',str(path),'--out',str(dest)],capture_output=True,text=True)
 if p.returncode:raise RuntimeError(str(dest)+'\n'+p.stdout+p.stderr)
 path.unlink()
 return {f:sha(dest/f) for f in ('height.u16','heightfield.obj','viewer.json','recipe.json')}

def recipes(seed):
 a=json.loads((ROOT/'recipes/alveolar_blocks-48-v14.json').read_text());a['seed']=seed
 l=json.loads((BASE/'lab-families-expansion/recipes'/f'limestone_dissolution-{seed}-v13.json').read_text())
 h={k:copy.deepcopy(a[k]) for k in ('schemaVersion','extractorVersion','family','units','seed','size','relief','backDepth','directionDegrees','irregularity','grid')}
 h.update(generatorVersion='heightfield-cliff-v11',profile='massive_oblique',blockScale=.288,jointSpacing=.16,jointDepth=.15,fractures=.175)
 return {'support':h,'limestone':l,'alveolar':a}

def main_outputs():
 records=[]
 for seed in SEEDS:
  for variant,r in recipes(seed).items():
   name=f'{variant}-{seed}';dest=OUT/name;save(ROOT/'recipes'/f'{name}-comparison.json',r)
   existing=all((dest/f).exists() for f in ('height.u16','heightfield.obj','viewer.json','recipe.json','metrics.json'))
   if existing and json.loads((dest/'recipe.json').read_text())==r:hashes={f:sha(dest/f) for f in ('height.u16','heightfield.obj','viewer.json','recipe.json')}
   else:hashes=run(r,dest,None if variant=='alveolar' else BASE/'lab-families-expansion/build/rocks_cli')
   record={'name':name,'hashes':hashes,'metrics':json.loads((dest/'metrics.json').read_text()),'binarySource':'candidate8' if variant=='alveolar' else 'preserved_candidate6'}
   if variant=='alveolar':record['diagnostics']=json.loads((dest/'diagnostics.json').read_text())
   records.append(record);print('MAIN_READY',name,flush=True)
 save(ROOT/'evidence/alveolar-generation.json',records)

def validation():
 records=[]
 for seed in SEEDS:
  base=OUT/f'alveolar-{seed}';r=recipes(seed)['alveolar'];original=json.loads((base/'viewer.json').read_text())['stages']['heightfield'];diagnostic=json.loads((base/'diagnostics.json').read_text())
  with tempfile.TemporaryDirectory(prefix='alveolar-validation-',dir=ROOT) as d:
   d=Path(d);replay=run(r,d/'replay');exact=all(replay[f]==sha(base/f) for f in replay)
   if not exact:raise RuntimeError('Replay mismatch '+str(seed))
   rr=copy.deepcopy(r);rr['grid']['width']=rr['grid']['height']=2049;run(rr,d/'refined')
   refined=json.loads((d/'refined/viewer.json').read_text())['stages']['heightfield'];rd=json.loads((d/'refined/diagnostics.json').read_text());error=geometric_error(original['positions'],refined['positions']);same_topology=original['indices']==refined['indices'];same_events=diagnostic['events']==rd['events'];same_guide=sha(base/'guide-mask.f64')==sha(d/'refined/guide-mask.f64')
   if not(same_topology and same_events and same_guide):raise RuntimeError('Refinement changed topology/events/guide')
   audit=subprocess.run([str(ROOT/'build/audit_intersections'),str(base/'heightfield.obj')],text=True,capture_output=True);(ROOT/'evidence'/f'intersections-{seed}.log').write_text(audit.stdout+audit.stderr)
   if audit.returncode:raise RuntimeError('Independent intersection audit failed '+str(seed))
   records.append({'seed':seed,'exactReplay':exact,'eventsAndGuideExactAcrossSourceResolution':same_events and same_guide,'topologyExactAcrossSourceResolution':same_topology,'convergence1025to2049':error,'rmsTargetPass':error['rmsMeters']<.00005,'maxTargetPass':error['maxMeters']<.0005,'independentIntersectionAuditPass':True,'refinedDiagnostics':{k:v for k,v in rd.items() if k!='events'}})
  print('VALIDATED',seed,error,flush=True)
 save(ROOT/'evidence/alveolar-validation.json',records)
 legacy=[]
 for name in ('heightfield-cliff-48-v10.json','heightfield-massive-oblique-48-v11.json','limestone_dissolution-48-v13.json','slate_foliated-48-v13.json'):
  r=json.loads((ROOT/'recipes'/name).read_text())
  with tempfile.TemporaryDirectory(prefix='alveolar-legacy-',dir=ROOT) as d:
   d=Path(d);before=run(r,d/'baseline',BASE/'lab-slate-candidate7/build/rocks_cli');after=run(r,d/'candidate')
   if before!=after:raise RuntimeError('Inherited hash regression '+name)
   legacy.append({'recipe':name,'allHashesExact':True,'hashes':before})
  print('LEGACY_EXACT',name,flush=True)
 save(ROOT/'evidence/inherited-version-hashes.json',legacy)
if __name__=='__main__':
 p=argparse.ArgumentParser();p.add_argument('--phase',choices=['main','validation','all'],default='all');a=p.parse_args()
 if a.phase in ('main','all'):main_outputs()
 if a.phase in ('validation','all'):validation()
