#!/usr/bin/env python3
"""Measure exported geometry, never rendering or deforming it."""
import argparse,csv,json,math
from pathlib import Path
from collections import defaultdict,Counter
import numpy as np

def obj(path):
    v=[];f=[]
    for line in path.read_text().splitlines():
        s=line.split()
        if s and s[0]=='v':v.append(list(map(float,s[1:4])))
        if s and s[0]=='f':f.append([int(x.split('/')[0])-1 for x in s[1:]])
    return np.array(v),np.array(f)
def distribution(a):
    a=np.array(a);return dict(zip(['min','p05','p50','p95','max'],map(float,np.quantile(a,[0,.05,.5,.95,1]))))
def measure(run):
    rest,rf=obj(run/'tick_0/mesh.obj');top=list(csv.DictReader(open(run/'topology.csv')))
    e=np.array([[int(x['a']),int(x['b'])] for x in top]);lens=np.array([float(x['rest']) for x in top])
    initial=json.loads((run/'tick_0/frame.json').read_text());uv=np.array([p['uv'] for p in initial['particles']]);points=np.array([p['rest'] for p in initial['particles']]);j=defaultdict(list)
    for x in top:
        for p in ['a','b']:j[int(x[p]),int(x['strand'])].append(x)
    candidates=[]
    for (center,strand),es in sorted(j.items()):
        if len(es)==2 and strand>=0:
            a=next(int(es[0][x]) for x in ['a','b'] if int(es[0][x])!=center)
            c=next(int(es[1][x]) for x in ['a','b'] if int(es[1][x])!=center)
            if a!=c:candidates.append([a,center,c])
    grid=[]
    for y in np.linspace(0,1,41):
        for x in np.linspace(0,1,41):
            a=uv[e[:,0]];d=uv[e[:,1]]-a;t=np.clip(np.sum((np.array([x,y])-a)*d,axis=1)/np.sum(d*d,axis=1),0,1)
            grid.append(float(np.min(np.linalg.norm(a+d*t[:,None]-[x,y],axis=1))))
    result={'run':str(run),'particles':len(points),'edges':len(e),'same_strand_bend_candidates':len(candidates),'scalar_budget_with_512_contacts':len(e)+len(candidates)+512,'edge_rest_length_m':distribution(lens),'UV_distance_to_nearest_carrier':distribution(grid),'same_strand_degree_counts':dict(Counter(len(x) for x in j.values())),'ticks':[]}
    def angle(p):
        q=np.array(candidates);a=p[q[:,0]]-p[q[:,1]];b=p[q[:,2]]-p[q[:,1]];return np.degrees(np.arccos(np.clip(np.sum(a*b,axis=1)/(np.linalg.norm(a,axis=1)*np.linalg.norm(b,axis=1)),-1,1)))
    rest_angles=angle(points)
    for folder in sorted(run.glob('tick_*'),key=lambda p:int(p.name[5:])):
        v,f=obj(folder/'mesh.obj');frame=json.loads((folder/'frame.json').read_text());p=np.array([p['p'] for p in frame['particles']]);rat=np.linalg.norm(p[e[:,1]]-p[e[:,0]],axis=1)/lens
        x=rest[f[:,1]]-rest[f[:,0]];y=rest[f[:,2]]-rest[f[:,0]];xx=v[f[:,1]]-v[f[:,0]];yy=v[f[:,2]]-v[f[:,0]]
        area0=np.linalg.norm(np.cross(x,y),axis=1);area=np.linalg.norm(np.cross(xx,yy),axis=1);n=np.cross(xx,yy)/area[:,None]
        adj=defaultdict(list)
        for i,t in enumerate(f):
            for k in range(3):adj[tuple(sorted((int(t[k]),int(t[(k+1)%3]))))].append(i)
        pairs=np.array([a for a in adj.values() if len(a)==2]);dihedral=np.degrees(np.arccos(np.clip(np.sum(n[pairs[:,0]]*n[pairs[:,1]],axis=1),-1,1)))
        # Rest-material tangent basis removes rest triangle aspect ratio from deformation strain.
        L=np.linalg.norm(x,axis=1);b=np.sum(x*y,axis=1)/L;c=np.sqrt(np.maximum(0,np.sum(y*y,axis=1)-b*b))
        F=np.stack((xx/L[:,None],(yy-xx*(b/L)[:,None])/c[:,None]),axis=-1);s=np.linalg.svd(F,compute_uv=False)
        a=angle(p)
        result['ticks'].append({'tick':frame['tick'],'faces':len(f),'events':len(frame['events']),'area_ratio':distribution(area/area0),'area_below_25pct':int(np.sum(area/area0<.25)),'principal_stretch_min':distribution(s[:,1]),'principal_stretch_max':distribution(s[:,0]),'dihedral_degrees':distribution(dihedral),'dihedral_over_90':int(np.sum(dihedral>90)),'dihedral_over_135':int(np.sum(dihedral>135)),'adjacent_faces':len(pairs),'carrier_edge_length_ratio':distribution(rat),'carrier_edges_compressed_below_half':int(np.sum(rat<.5)),'same_strand_turn_degrees':distribution(180-a),'same_strand_turn_over_90':int(np.sum(a<90)),'same_strand_angle_error_RMS_deg':float(np.sqrt(np.mean((a-rest_angles)**2)))})
    return result
if __name__=='__main__':
    ap=argparse.ArgumentParser();ap.add_argument('run',type=Path);ap.add_argument('--output',type=Path);args=ap.parse_args();r=measure(args.run);txt=json.dumps(r,indent=2);print(txt)
    if args.output:args.output.write_text(txt+'\n')
