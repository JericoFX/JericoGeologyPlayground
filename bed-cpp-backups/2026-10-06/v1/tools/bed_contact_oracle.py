"""Independent mesh-triangle support QA; no generation or geometry repair."""
import json,sys,math
from pathlib import Path
import numpy as np
from scipy.spatial import cKDTree
s=json.loads(Path(sys.argv[1]).read_text());parts={p['id']:p for p in s['parts']};m=parts.get('mattress');report={'native_scene':True,'errors':[]}
if m:
 v=np.array(m['vertices']);half=len(v)//2
 fs=np.array([f for f in m['faces'] if len(f)==3 and all(i<half for i in f)])
 triangles=v[fs];tree=cKDTree(triangles[:,:,:2].mean(axis=1))
 def support(q):
  _,ix=tree.query(q[:2],k=min(30,len(triangles)))
  for i in np.atleast_1d(ix):
   t=triangles[i];a=t[1,:2]-t[0,:2];b=t[2,:2]-t[0,:2];p=np.array(q[:2])-t[0,:2];det=a[0]*b[1]-a[1]*b[0]
   if abs(det)<1e-16:continue
   u=(p[0]*b[1]-p[1]*b[0])/det;w=(a[0]*p[1]-a[1]*p[0])/det
   if u>=-1e-8 and w>=-1e-8 and u+w<=1+1e-8:return t[0,2]+u*(t[1,2]-t[0,2])+w*(t[2,2]-t[0,2])
  return None
 for name in ['pillow','blanket']:
  p=parts.get(name)
  if not p:continue
  points=p['vertices'][len(p['vertices'])//2:];gaps=[]
  for q in points:
   h=support(q)
   if h is not None:gaps.append(q[2]-h)
  assert gaps,name+' no support overlap'
  report[name]={'tested_lower_vertices':len(points),'overlap_samples':len(gaps),'minimum_gap_m':min(gaps),'maximum_gap_m':max(gaps),'near_contact_samples':sum(abs(g)<.003 for g in gaps),'penetrations_beyond_0_5mm':sum(g<-.0005 for g in gaps)}
  if min(gaps)<-.0005:report['errors'].append(name+' penetrates actual mattress mesh')
  if min(gaps)>.003:report['errors'].append(name+' floats above actual mattress mesh')
 # Slat contact is actual bottom geometry, not a metadata value.
 bottom=v[half:];slats=[p for key,p in parts.items() if key.startswith('support.slat.')];contact=[]
 for p in slats:
  sv=np.array(p['vertices']);lo=sv.min(axis=0);hi=sv.max(axis=0)
  pts=bottom[(bottom[:,0]>=lo[0])&(bottom[:,0]<=hi[0])&(bottom[:,1]>=lo[1])&(bottom[:,1]<=hi[1])]
  if len(pts):contact.append(float((pts[:,2]-hi[2]).min()))
 report['mattress_slat_contact']={'slats_tested':len(contact),'minimum_gap_m':min(contact),'maximum_nearest_gap_m':max(contact)}
 if min(contact)<-.0001 or max(contact)>.003:report['errors'].append('mattress support gap')
print(json.dumps(report,indent=2,default=lambda x:x.item()));sys.exit(bool(report['errors']))
