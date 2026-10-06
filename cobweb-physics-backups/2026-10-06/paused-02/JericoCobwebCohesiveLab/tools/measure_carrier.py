#!/usr/bin/env python3
"""Read-only material distortion metrics; Python standard library only."""
import argparse,json,math,pathlib,collections

def load(path):
 v=[];f=[]
 for line in path.read_text().splitlines():
  a=line.split()
  if a and a[0]=='v':v.append(tuple(map(float,a[1:4])))
  if a and a[0]=='f':f.append(tuple(int(x.split('/')[0])-1 for x in a[1:]))
 return v,f

def sub(a,b):return tuple(x-y for x,y in zip(a,b))
def mul(a,s):return tuple(x*s for x in a)
def dot(a,b):return sum(x*y for x,y in zip(a,b))
def cross(a,b):return(a[1]*b[2]-a[2]*b[1],a[2]*b[0]-a[0]*b[2],a[0]*b[1]-a[1]*b[0])
def norm(a):return math.sqrt(dot(a,a))
def measure(rest,deformed):
 a,rf=load(rest);v,f=load(deformed);assert rf==f,'Use matching pre-tear topology for distortion comparisons'
 normals=[];areas=[];stretch=[];adj=collections.defaultdict(list)
 for i,t in enumerate(f):
  x,y=sub(a[t[1]],a[t[0]]),sub(a[t[2]],a[t[0]]);xx,yy=sub(v[t[1]],v[t[0]]),sub(v[t[2]],v[t[0]])
  r0=norm(cross(x,y));n=cross(xx,yy);r=norm(n);normals.append(mul(n,1/r));areas.append(r/r0)
  L=norm(x);b=dot(x,y)/L;h=math.sqrt(max(0,dot(y,y)-b*b));c1=mul(xx,1/L);c2=mul(sub(yy,mul(xx,b/L)),1/h);A,B,C=dot(c1,c1),dot(c1,c2),dot(c2,c2);stretch.append(math.sqrt(max(0,(A+C+math.sqrt((A-C)**2+4*B*B))/2)))
  for k in range(3):adj[tuple(sorted((t[k],t[(k+1)%3])))].append(i)
 angles=[math.degrees(math.acos(max(-1,min(1,dot(normals[p[0]],normals[p[1]]))))) for p in adj.values() if len(p)==2]
 return {'triangles':len(f),'adjacent_faces':len(angles),'folds_gt_90':sum(x>90 for x in angles),'folds_gt_135':sum(x>135 for x in angles),'max_fold_degrees':max(angles),'area_below_25pct':sum(x<.25 for x in areas),'max_principal_stretch':max(stretch),'min_area_ratio':min(areas),'max_area_ratio':max(areas)}
if __name__=='__main__':
 p=argparse.ArgumentParser();p.add_argument('rest',type=pathlib.Path);p.add_argument('deformed',type=pathlib.Path);p.add_argument('--out',type=pathlib.Path);o=p.parse_args();s=json.dumps(measure(o.rest,o.deformed),indent=2);print(s)
 if o.out:o.out.write_text(s+'\n')
