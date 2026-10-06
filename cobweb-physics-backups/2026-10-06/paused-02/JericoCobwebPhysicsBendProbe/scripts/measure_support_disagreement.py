"""Read-only material-space crossings of physical edges without a shared particle."""
import csv,json
from pathlib import Path
import numpy as np
root=Path(__file__).resolve().parents[1]
top=list(csv.DictReader(open(root/'outputs/off/topology.csv')))
pairs=np.array([[int(e['a']),int(e['b'])] for e in top]);frame=json.loads((root/'outputs/off/tick_0/frame.json').read_text());uv=np.array([p['uv'] for p in frame['particles']]);rest=np.array([p['rest'] for p in frame['particles']]);found=[]
cross=lambda a,b:a[0]*b[1]-a[1]*b[0]
for i,(a,b) in enumerate(pairs):
 for j in range(i+1,len(pairs)):
  c,d=pairs[j]
  if len({a,b,c,d})<4:continue
  u=uv[b]-uv[a];v=uv[d]-uv[c];den=cross(u,v)
  if abs(den)<1e-12:continue
  t=cross(uv[c]-uv[a],v)/den;s=cross(uv[c]-uv[a],u)/den
  if 1e-7<t<1-1e-7 and 1e-7<s<1-1e-7:found.append((i,j,t,s))
result={'nonjunction_material_crossings':len(found),'tick_150':{}}
for name in ['off','soft']:
 frame=json.loads((root/f'outputs/{name}/tick_150/frame.json').read_text());pos=np.array([p['p'] for p in frame['particles']]);delta=pos-rest;gap=[]
 for i,j,t,s in found:
  a,b=pairs[i];c,d=pairs[j];gap.append(float(np.linalg.norm(delta[a]*(1-t)+delta[b]*t-delta[c]*(1-s)-delta[d]*s)))
 result['tick_150'][name]={'displacement_disagreement_m':dict(zip(['min','p50','p95','max'],map(float,np.quantile(gap,[0,.5,.95,1])))),'over_5cm':sum(g>.05 for g in gap),'over_10cm':sum(g>.1 for g in gap)}
print(json.dumps(result,indent=2));(root/'evidence/support-disagreement.json').write_text(json.dumps(result,indent=2)+'\n')
