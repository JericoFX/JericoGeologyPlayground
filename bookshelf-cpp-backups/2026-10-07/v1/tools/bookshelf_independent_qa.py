#!/usr/bin/env python3
"""Read-only independent bookshelf oracle (NumPy). Never generates asset geometry.
Checks actual mesh containment, SAT contacts, closed oriented float32 topology,
physical contact samples against support triangles, and declared frame joinery.
This proves geometric contacts, not structural safety or rigid-body stability.
"""
import argparse
from collections import defaultdict
import hashlib
import json
from pathlib import Path
import numpy as np

TOL = .001

def triangles(part):
    f=[]; ns=[]
    for k,face in enumerate(part['faces']):
        for j in range(1,len(face)-1):
            f.append([face[0],face[j],face[j+1]])
            if 'normals' in part:
                ns.append([part['normals'][k][0],part['normals'][k][j],part['normals'][k][j+1]])
    return np.asarray(f,dtype=np.int64), np.asarray(ns,dtype=float)

def minimum_surface_distance(points,tris):
    """Exact point-to-triangle distances, vectorized by candidate sample point."""
    best=float('inf'); witness=None
    a,b,c=tris[:,0],tris[:,1],tris[:,2]; ab,ac=b-a,c-a
    n=np.cross(ab,ac); nn=np.einsum('ij,ij->i',n,n)
    for p in points:
        ap=p-a; signed=np.einsum('ij,ij->i',ap,n)
        proj=p-(signed/np.maximum(nn,1e-60))[:,None]*n; rel=proj-a
        aa=np.einsum('ij,ij->i',ab,ab); bb=np.einsum('ij,ij->i',ab,ac); cc=np.einsum('ij,ij->i',ac,ac)
        dd=np.einsum('ij,ij->i',rel,ab); ee=np.einsum('ij,ij->i',rel,ac); det=aa*cc-bb*bb
        u=(dd*cc-ee*bb)/np.maximum(det,1e-60);v=(ee*aa-dd*bb)/np.maximum(det,1e-60)
        valid=(u>=-1e-10)&(v>=-1e-10)&(u+v<=1+1e-10)&(nn>1e-30)
        dist=np.where(valid,signed*signed/np.maximum(nn,1e-60),np.inf)
        for e0,e1 in ((a,b),(b,c),(c,a)):
            edge=e1-e0; t=np.clip(np.einsum('ij,ij->i',p-e0,edge)/np.maximum(np.einsum('ij,ij->i',edge,edge),1e-60),0,1)
            q=e0+t[:,None]*edge;diff=q-p;dist=np.minimum(dist,np.einsum('ij,ij->i',diff,diff))
        ix=int(np.argmin(dist))
        if dist[ix]<best:best=float(dist[ix]);witness={'point':p.tolist(),'triangle':tris[ix].tolist()}
    return float(np.sqrt(best)),witness

def segment_triangle_intersection(edges, tris):
    """Independent finite-segment Moller-Trumbore witness for intentional embeds."""
    a,b,c=tris[:,0],tris[:,1],tris[:,2]; e1,e2=b-a,c-a
    for start,end in edges:
        direction=end-start;h=np.cross(direction,e2);det=np.einsum('ij,ij->i',e1,h);valid=np.abs(det)>1e-13
        inv=np.divide(1.,det,out=np.zeros_like(det),where=valid);ss=start-a;u=inv*np.einsum('ij,ij->i',ss,h);q=np.cross(ss,e1);v=inv*(q@direction);t=inv*np.einsum('ij,ij->i',e2,q)
        hit=valid&(u>=-1e-8)&(v>=-1e-8)&(u+v<=1+1e-8)&(t>=-1e-8)&(t<=1+1e-8)
        if hit.any():
            ix=int(np.flatnonzero(hit)[0]);return {'point':(start+t[ix]*direction).tolist(),'triangle':tris[ix].tolist(),'method':'segment_triangle_intersection'}
    return None

def obb_corners(i):
    o=i['obb'];a=np.array(o['axes']);h=np.array(o['half']);c=np.array(o['center'])
    return np.array([c+(h*np.array([x,y,z]))@a for x in (-1,1) for y in (-1,1) for z in (-1,1)])

def separation(a,b):
    oa,ob=a['obb'],b['obb'];aa,ab=np.array(oa['axes']),np.array(ob['axes']);ha,hb=np.array(oa['half']),np.array(ob['half']);delta=np.array(ob['center'])-np.array(oa['center'])
    axes=list(aa)+list(ab)
    for u in aa:
        for v in ab:
            n=np.cross(u,v);l=np.linalg.norm(n)
            if l>1e-8:axes.append(n/l)
    axes=np.array(axes)
    return float(np.max(np.abs(axes@delta)-np.abs(axes@aa.T)@ha-np.abs(axes@ab.T)@hb))

def review(scene,path):
    parts={p['id']:p for p in scene['parts']};items={i['id']:i for i in scene['metadata']['items']};itemparts=defaultdict(list);errors=[];warnings=[]
    pv={};pt={};all_vertices=0;all_tris=0;min_area=float('inf');min_volume=float('inf');max_normal_error=0.;min_normal_dot=1.;bounds=[];topology=[]
    for pid,p in parts.items():
        vs=np.asarray(p['vertices'],dtype=float);pv[pid]=vs;ti,ns=triangles(p);f32=vs.astype(np.float32).astype(float);ts=f32[ti];pt[pid]=ts
        if 'item_id' in p:itemparts[p['item_id']].append(pid)
        all_vertices+=len(vs);all_tris+=len(ti)
        if not np.isfinite(vs).all():errors.append({'kind':'nonfinite_vertex','part':pid})
        cross=np.cross(ts[:,1]-ts[:,0],ts[:,2]-ts[:,0]);area=np.linalg.norm(cross,axis=1);volume=float(np.einsum('ij,ij->i',ts[:,0],np.cross(ts[:,1],ts[:,2])).sum()/6)
        min_area=min(min_area,float(area.min())*.5);min_volume=min(min_volume,volume)
        edges=defaultdict(list)
        for face in p['faces']:
            for x,y in zip(face,face[1:]+face[:1]):edges[min(x,y),max(x,y)].append(x<y)
        invalid=sum(len(d)!=2 or sum(d)!=1 for d in edges.values())
        # Weld float32 coincident vertices and recheck edges to expose numeric collapse.
        _,remap=np.unique(f32,axis=0,return_inverse=True);collapse=int(sum(len(set(remap[t]))<3 for t in ti))
        if invalid or collapse or volume<=0 or np.any(area<=1e-16):errors.append({'kind':'mesh_topology','part':pid,'invalid_oriented_edges':invalid,'float32_collapsed_triangles':collapse,'volume_m3':volume,'minimum_double_area_m2':float(area.min())})
        if len(ns):
            lengths=np.linalg.norm(ns,axis=2);normal_error=float(np.abs(lengths-1).max());dot=np.einsum('tki,ti->tk',ns,cross/np.maximum(area[:,None],1e-30));worstdot=float(dot.min());max_normal_error=max(max_normal_error,normal_error);min_normal_dot=min(min_normal_dot,worstdot)
            if not np.isfinite(ns).all() or normal_error>2e-5 or worstdot<-.001:errors.append({'kind':'normal','part':pid,'length_error':normal_error,'minimum_face_dot':worstdot})
    for iid,i in items.items():
        if not itemparts[iid]:errors.append({'kind':'empty_item','item':iid});continue
        o=i['obb'];a=np.array(o['axes']);c=np.array(o['center']);h=np.array(o['half']);vertices=np.concatenate([pv[p] for p in itemparts[iid]])
        if np.max(np.abs(a@a.T-np.eye(3)))>1e-8 or np.linalg.det(a)<.999999:errors.append({'kind':'invalid_obb_basis','item':iid})
        worst={'excess_m':0.}
        for pid in itemparts[iid]:
            d=np.abs((pv[pid]-c)@a.T)-h;k=np.unravel_index(d.argmax(),d.shape)
            if d[k]>worst['excess_m']:worst={'item':iid,'part':pid,'axis':int(k[1]),'excess_m':float(d[k]),'point':pv[pid][k[0]].tolist()}
        bounds.append(worst)
        if worst['excess_m']>TOL+1e-7:errors.append({'kind':'mesh_outside_obb',**worst})
        declared=set(i['grab']['group_part_ids']);actual=set(itemparts[iid])
        if declared!=actual:errors.append({'kind':'grab_part_mismatch','item':iid,'missing':sorted(actual-declared),'extra':sorted(declared-actual)})
    collisions=[];iv=list(items.values())
    for k,a in enumerate(iv):
        for b in iv[k+1:]:
            if a['compartment']!=b['compartment']:continue
            gap=separation(a,b)
            if gap < -.0001:collisions.append({'a':a['id'],'b':b['id'],'minimum_penetration_m':-gap})
    errors.extend({'kind':'obb_penetration',**x} for x in collisions)
    contacts=[]
    for iid,i in items.items():
        vertices=np.concatenate([pv[p] for p in itemparts[iid]]);itris=np.concatenate([pt[p] for p in itemparts[iid]]);samples=np.concatenate([vertices,itris.mean(axis=1),(itris[:,0]+itris[:,1])*.5,(itris[:,1]+itris[:,2])*.5,(itris[:,2]+itris[:,0])*.5]);floor=vertices[:,2].min();queries=np.unique(samples[samples[:,2]<=floor+.00002],axis=0)
        targets=itemparts[i['support_id']] if i['support_id'] in items else [i['support_id']]
        if any(p not in pt for p in targets):errors.append({'kind':'missing_support','item':iid});continue
        ts=np.concatenate([pt[p] for p in targets]);distance,witness=minimum_surface_distance(queries,ts);contact={'item':iid,'support':i['support_id'],'kind':'bearing','distance_m':distance,'witness':witness};contacts.append(contact)
        if distance>TOL+1e-7:errors.append({**contact,'kind':'missing_real_bearing'})
        if 'lean_support_id' in i:
            target=i['lean_support_id'];lowx=vertices[:,0].min();queries=np.unique(samples[samples[:,0]<=lowx+.00005],axis=0);ts=np.concatenate([pt[p] for p in itemparts[target]]);distance,witness=minimum_surface_distance(queries,ts);contact={'item':iid,'support':target,'kind':'lean','distance_m':distance,'witness':witness};contacts.append(contact)
            if distance>TOL+1e-7:errors.append({**contact,'kind':'missing_real_lean_contact'})
        # Check shelf and stacking margins independently from production validation.
        corners=obb_corners(i);lo,hi=corners.min(axis=0),corners.max(axis=0);bay=scene['metadata']['compartments'][i['compartment']]
        if lo[0]<bay['x_min']-1e-6 or hi[0]>bay['x_max']+1e-6 or lo[2]<bay['bottom']-1e-6 or hi[2]>bay['top']-.005+1e-6:errors.append({'kind':'compartment_margin','item':iid,'min':lo.tolist(),'max':hi.tolist(),'bay':bay})
        if i['support_id'] in items:
            supports=obb_corners(items[i['support_id']]);slo,shi=supports.min(axis=0),supports.max(axis=0)
            if np.any(lo[:2]<slo[:2]-1e-7) or np.any(hi[:2]>shi[:2]+1e-7):errors.append({'kind':'stack_overhang','item':iid})
    # Witness-based contact test for intentional frame bearing. The candidates include
    # vertices, edges and centroids; a positive distance is flagged for review, not
    # assumed to prove disconnection if intersecting triangles lack a sampled witness.
    joints=[]
    pairs=[(j['supported'],j['support']) for j in scene['metadata'].get('joints',[])]
    for pid in parts:
        if '.cleat.' in pid:
            sx=-1 if pid.endswith('.L') else 1
            pairs.extend((pid,f'frame.post.{sx}.{sy}') for sy in (-1,1))
        if pid.startswith('back.crossbar.'):
            pairs.extend((pid,f'frame.post.{sx}.1') for sx in (-1,1))
    for pid,p in parts.items():
        if pid.endswith('.foot'):pairs.append((pid,pid[:-5]))
        if p.get('role')=='Stile' or pid.startswith('frame.post.') and not pid.endswith('.foot'):pairs.append((pid,'frame.cornice'))
    for a,b in pairs:
        ta,tb=pt[a],pt[b];alo,ahi=ta.reshape(-1,3).min(axis=0),ta.reshape(-1,3).max(axis=0);blo,bhi=tb.reshape(-1,3).min(axis=0),tb.reshape(-1,3).max(axis=0)
        if np.any(np.minimum(ahi,bhi)<np.maximum(alo,blo)-TOL):errors.append({'kind':'frame_joint_aabb_gap','a':a,'b':b});continue
        # Contact surfaces are concentrated inside the intersection of expanded AABBs.
        pts=np.concatenate([pv[a],ta.mean(axis=1),(ta[:,0]+ta[:,1])*.5,(ta[:,1]+ta[:,2])*.5,(ta[:,2]+ta[:,0])*.5]);mask=np.all((pts>=blo-TOL)&(pts<=bhi+TOL),axis=1);queries=np.unique(pts[mask],axis=0)
        if len(queries)>300:queries=queries[np.linspace(0,len(queries)-1,300,dtype=int)]
        if len(queries):distance,witness=minimum_surface_distance(queries,tb)
        else:distance,witness=float('inf'),None
        if distance>TOL:
            pts=np.concatenate([pv[b],tb.mean(axis=1),(tb[:,0]+tb[:,1])*.5,(tb[:,1]+tb[:,2])*.5,(tb[:,2]+tb[:,0])*.5]);mask=np.all((pts>=alo-TOL)&(pts<=ahi+TOL),axis=1);queries=np.unique(pts[mask],axis=0)
            if len(queries)>300:queries=queries[np.linspace(0,len(queries)-1,300,dtype=int)]
            if len(queries):
                reverse_distance,reverse_witness=minimum_surface_distance(queries,ta)
                if reverse_distance<distance:distance,witness=reverse_distance,reverse_witness
        if distance>TOL:
            edges=np.concatenate([ta[:,[0,1]],ta[:,[1,2]],ta[:,[2,0]]]);em=(edges.min(axis=1)<=bhi+1e-7).all(axis=1)&(edges.max(axis=1)>=blo-1e-7).all(axis=1);hit=segment_triangle_intersection(edges[em],tb)
            if hit is not None:distance,witness=0.,hit
        result={'a':a,'b':b,'sample_distance_m':distance,'witness':witness};joints.append(result)
        if distance>TOL:warnings.append({'kind':'frame_contact_needs_intersection_review',**result})
    return {'source':str(path),'source_sha256':hashlib.sha256(Path(path).read_bytes()).hexdigest(),'status':'FAIL' if errors else 'PASS','tolerance_m':TOL,'parts':len(parts),'items':len(items),'vertices':all_vertices,'triangles':all_tris,'topology':{'closed_oriented_meshes_checked':len(parts),'float32_checked':True,'minimum_triangle_area_m2':min_area,'minimum_signed_volume_m3':min_volume,'maximum_unit_normal_error':max_normal_error,'minimum_normal_face_dot':min_normal_dot},'bounds':sorted(bounds,key=lambda x:x['excess_m'],reverse=True),'item_contacts':contacts,'frame_contacts':joints,'errors':errors,'warnings':warnings,'limitations':['Geometric contact only; no load/stability simulation or engine physics.','AABB/OBB mesh containment tolerance is 1 mm; precision preserved in all reported witnesses.','Frame sample distance is sufficient evidence of proximity; unsampled triangle intersection needs separate review.']}

def main():
    parser=argparse.ArgumentParser();parser.add_argument('scene');parser.add_argument('--out',required=True);args=parser.parse_args();path=Path(args.scene);r=review(json.loads(path.read_text()),path);Path(args.out).write_text(json.dumps(r,indent=2));print(json.dumps({k:r[k] for k in ('status','parts','items','triangles','topology')}));print('errors:',len(r['errors']),'warnings:',len(r['warnings']));print(json.dumps(r['errors'][:12],indent=2));return 1 if r['errors'] else 0
if __name__=='__main__':raise SystemExit(main())
