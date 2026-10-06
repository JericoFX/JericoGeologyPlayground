#!/usr/bin/env python3
"""Read-only dock QA oracle. It does not create or modify mesh geometry.

Run from the project root:
 python3 tools/dock_independent_qa.py --scene output/preview/scene.json --glb output/preview/dock.glb
Optional --intact-scene and --repeat-scene verify support protection and determinism.
The checks establish authoring geometry, not engineering load capacity or simulation.
"""
import argparse
from collections import Counter
import hashlib
import json
import math
from pathlib import Path
import struct


def sub(a,b): return tuple(x-y for x,y in zip(a,b))
def dot(a,b): return sum(x*y for x,y in zip(a,b))
def cross(a,b): return (a[1]*b[2]-a[2]*b[1],a[2]*b[0]-a[0]*b[2],a[0]*b[1]-a[1]*b[0])
def norm(a): return math.sqrt(dot(a,a))
def finite(a): return all(math.isfinite(x) for x in a)
def unit(a):
    n=norm(a)
    return tuple(x/n for x in a) if n else (0,0,0)
def digest(a): return hashlib.sha256(json.dumps(a,sort_keys=True,separators=(',',':')).encode()).hexdigest()

def hull(points):
    points=sorted(set(points))
    def turn(a,b,c): return (b[0]-a[0])*(c[1]-a[1])-(b[1]-a[1])*(c[0]-a[0])
    lo=[];hi=[]
    for p in points:
        while len(lo)>1 and turn(lo[-2],lo[-1],p)<=0:lo.pop()
        lo.append(p)
    for p in reversed(points):
        while len(hi)>1 and turn(hi[-2],hi[-1],p)<=0:hi.pop()
        hi.append(p)
    return lo[:-1]+hi[:-1]

def band_section(poly,y,halfwidth=.035):
    values=[x for x,z in poly if abs(z-y)<=halfwidth]
    for z in (y-halfwidth,y,y+halfwidth):
        interval=section(poly,z)
        if interval:values.extend(interval)
    return (min(values),max(values)) if values else None

def section(poly,y):
    cuts=[]
    for a,b in zip(poly,poly[1:]+poly[:1]):
        if abs(a[1]-y)<1e-10:cuts.append(a[0])
        if (a[1]<y<b[1]) or (b[1]<y<a[1]):cuts.append(a[0]+(b[0]-a[0])*(y-a[1])/(b[1]-a[1]))
    return (min(cuts),max(cuts)) if cuts else None

def vertical_interval(part,x,y):
    heights=[]
    for face in part['faces']:
        for i in range(1,len(face)-1):
            a,b,c=[part['vertices'][j] for j in (face[0],face[i],face[i+1])]
            den=(b[1]-c[1])*(a[0]-c[0])+(c[0]-b[0])*(a[1]-c[1])
            if abs(den)<1e-15:continue
            u=((b[1]-c[1])*(x-c[0])+(c[0]-b[0])*(y-c[1]))/den
            v=((c[1]-a[1])*(x-c[0])+(a[0]-c[0])*(y-c[1]))/den
            w=1-u-v
            if min(u,v,w)>=-1e-8:heights.append(u*a[2]+v*b[2]+w*c[2])
    return (min(heights),max(heights)) if heights else None

def segment_triangle_hit(a,b,triangle):
    e1=sub(triangle[1],triangle[0]);e2=sub(triangle[2],triangle[0]);direction=sub(b,a)
    h=cross(direction,e2);det=dot(e1,h)
    if abs(det)<1e-12:return False
    inv=1/det;offset=sub(a,triangle[0]);u=inv*dot(offset,h)
    if not 0<=u<=1:return False
    k=cross(offset,e1);v=inv*dot(direction,k)
    if v<0 or u+v>1:return False
    return 0<=inv*dot(e2,k)<=1

def solids_contact(a,b):
    # Positive tests: a vertex is inside the parent, or boundaries intersect.
    # Timber sections contain no internal voids, making vertical intervals valid.
    for x,y,z in a['vertices']:
        interval=vertical_interval(b,x,y)
        if interval and interval[0]<z<interval[1]:return True
    def triangles(part):
        return [[part['vertices'][j] for j in (f[0],f[i],f[i+1])] for f in part['faces'] for i in range(1,len(f)-1)]
    def bounds(t):return [(min(v[i] for v in t),max(v[i] for v in t)) for i in range(3)]
    first=[(t,bounds(t)) for t in triangles(a)];second=[(t,bounds(t)) for t in triangles(b)]
    for t,ab in first:
        for u,bb in second:
            if any(x[0]>y[1] or y[0]>x[1] for x,y in zip(ab,bb)):continue
            if any(segment_triangle_hit(t[i],t[(i+1)%3],u) or segment_triangle_hit(u[i],u[(i+1)%3],t) for i in range(3)):return True
    # A small timber can be fully inside a larger one in either direction.
    for x,y,z in b['vertices']:
        interval=vertical_interval(a,x,y)
        if interval and interval[0]<z<interval[1]:return True
    return False

class Checks:
    def __init__(self):self.errors=[];self.warnings=[];self.metrics={}
    def check(self,ok,message):
        if not ok:self.errors.append(message)
    def finish(self):return {'status':'PASS' if not self.errors else 'FAIL','errors':self.errors,'warnings':self.warnings,'metrics':self.metrics}

def topology(vertices,faces,label,out):
    out.check(all(len(p)==3 and finite(p) for p in vertices),label+': finite 3D vertices')
    edge=Counter();balance=Counter();volume=0.0;tris=0
    for f in faces:
        if len(f)<3 or any(type(i)!=int or not 0<=i<len(vertices) for i in f):
            out.errors.append(label+': invalid face indices');continue
        out.check(len(set(f))==len(f),label+': duplicate vertex in face')
        for a,b in zip(f,f[1:]+f[:1]):
            k=tuple(sorted((a,b)));edge[k]+=1;balance[k]+=1 if a<b else -1
        a=vertices[f[0]]
        for j in range(1,len(f)-1):
            b,c=vertices[f[j]],vertices[f[j+1]];ng=cross(sub(b,a),sub(c,a));tris+=1
            out.check(norm(ng)>1e-13,label+': degenerate triangle')
            volume+=dot(a,cross(b,c))/6
    out.check(all(v==2 for v in edge.values()),label+': not a closed 2-manifold by edge incidence')
    out.check(all(v==0 for v in balance.values()),label+': inconsistent winding at edges')
    out.check(volume>1e-12,label+': nonpositive signed volume')
    return tris

def check_scene(scene,out):
    parts=scene['parts'];byid={p['id']:p for p in parts};recipe=scene['recipe'];meta=scene['metadata']
    out.check(len(byid)==len(parts),'duplicate part ids')
    triangles=0;bearing_gaps=[]
    for p in parts:
        name=p['id'];vertices=p['vertices'];faces=p['faces']
        triangles+=topology(vertices,faces,name,out)
        for key,size in [('uvs',2),('texture_uvs',2),('normals',3)]:
            if key not in p:
                out.errors.append(name+': missing '+key);continue
            out.check(len(p[key])==len(faces),name+': '+key+' face count')
            for f,values in zip(faces,p[key]):
                out.check(len(f)==len(values) and all(len(x)==size and finite(x) for x in values),name+': invalid '+key+' loops')
                if key=='normals':
                    out.check(all(abs(norm(n)-1)<1e-6 for n in values),name+': nonunit normals')
                    a,b,c=(vertices[i] for i in f[:3]);ng=cross(sub(b,a),sub(c,a))
                    out.check(all(dot(n,ng)>0 for n in values),name+': inward normal hemisphere')
        for otherid in p.get('bearing_part_ids',[]):
            out.check(otherid in byid,name+': missing bearing '+otherid)
            if otherid not in byid:continue
            other=byid[otherid]
            gaps=[max(0,min(v[i] for v in vertices)-max(v[i] for v in other['vertices']),min(v[i] for v in other['vertices'])-max(v[i] for v in vertices)) for i in range(3)]
            bearing_gaps.append({'part':name,'bearing':otherid,'gap_m':max(gaps)})
            out.check(max(gaps)<=.001,f'{name}: declared bearing {otherid} exceeds 1 mm bounding-box contact tolerance {gaps}')
    out.metrics.update(parts=len(parts),source_vertices=sum(len(p['vertices']) for p in parts),triangles=triangles,source_geometry_sha256=digest([{k:p[k] for k in ('id','vertices','faces')} for p in parts]))
    out.metrics['declared_bearing_max_aabb_gap_m']=max((x['gap_m'] for x in bearing_gaps),default=0)
    small_gaps=[x for x in bearing_gaps if 1e-5<x['gap_m']<=.001]
    if small_gaps:out.warnings.append('Sub-millimetre declared bearing clearances within authoring tolerance: '+json.dumps(small_gaps))
    profiles=[p['timber_profile'] for p in parts if p['role']=='DeckBoard']
    out.metrics['distinct_board_profiles']=len({digest(p) for p in profiles})
    if profiles:out.check(len({digest(p) for p in profiles})>=min(3,len(profiles)),'insufficient independent board geometry profiles')
    piles=[p for p in parts if p['role']=='Pile']
    out.metrics['distinct_pile_radii']=len({p.get('radius_m') for p in piles})
    if len(piles)>1:out.check(out.metrics['distinct_pile_radii']>1,'identical pile radii')
    fractured=[p for p in parts if p.get('fractured',False)]
    damage=recipe['damage'];expected=set(meta.get('damaged_board_ids',[]));actual={p.get('source_board_id') for p in fractured if p['role']=='DeckBoard'}
    out.check(actual==expected,'damaged board IDs disagree with fractured parts')
    for p in fractured:
        out.check(p['role'] in ('DeckBoard','Splinter') and not p.get('protected_support',False),p['id']+': protected/nondeck part fractured')
        bearing=byid.get(p.get('bearing_part_ids',[''])[0],{})
        if p['role']=='Splinter':
            out.check(p.get('attached_fragment',False) and bearing.get('fractured',False) and bearing.get('role')=='DeckBoard',p['id']+': splinter has no fractured parent board')
            if bearing:out.check(solids_contact(p,bearing),p['id']+': attached splinter is geometrically disconnected from parent')
            d=bearing.get('route_distance_m',math.inf)
        else:d=p.get('route_distance_m',math.inf)
        out.check(damage.get('enabled',False) and damage['start_m']<=d<=damage['end_m'],p['id']+': fracture outside authored region')
        # Deck board length axes must follow the same orientation as local beam width.
        if p['role']=='DeckBoard' and 'timber_frame' in bearing:
            out.check(dot(p['timber_frame']['u'],bearing['timber_frame']['v'])>.7,p['id']+': reversed damaged fragment / cut outside board')
    if not damage.get('enabled',False) or damage.get('severity',1)==0:
        out.check(not fractured,'damage disabled/zero severity but fractured parts remain')
    for source in actual:
        if not isinstance(source,str):
            out.errors.append('fractured board has no source_board_id');continue
        socket=next((s for s in scene.get('sockets',[]) if s.get('id')==source+'.center.socket'),None)
        out.check(socket is not None and not socket.get('enabled',True),str(source)+': missing disabled damaged socket')
    out.metrics['fractured_parts']=len(fractured);out.metrics['fractured_board_parts']=sum(p['role']=='DeckBoard' for p in fractured);out.metrics['attached_splinters']=sum(p['role']=='Splinter' for p in fractured)
    nail_gaps=[]
    for p in parts:
        if p['role']!='Nail' or '.nail.' not in p['id']:continue
        source=p['id'].split('.nail.')[0]
        boards=[byid[source]] if source in byid else [byid[k] for k in (source+'.left',source+'.right') if k in byid]
        x=sum(v[0] for v in p['vertices'])/len(p['vertices']);y=sum(v[1] for v in p['vertices'])/len(p['vertices'])
        surfaces=[r[1] for board in boards if (r:=vertical_interval(board,x,y)) is not None]
        if surfaces:nail_gaps.append({'id':p['id'],'gap_m':min(v[2] for v in p['vertices'])-max(surfaces)})
    if nail_gaps:
        worst=max(nail_gaps,key=lambda p:p['gap_m']);out.metrics['nail_gap_max_m']=worst['gap_m'];out.metrics['nail_centers_floating_over_1mm']=sum(p['gap_m']>.001 for p in nail_gaps)
        out.check(worst['gap_m']<=.001,f"{worst['id']}: nail floats {worst['gap_m']:.6f} m above board at shaft center")
    if 'stairs' in meta:
        st=meta['stairs'];treads=sorted((p for p in parts if p['role']=='StairTread'),key=lambda p:int(p['id'].split('.')[-1]));rise=st['rise_m'];previous=st['top_height_m'];levels=[];spreads=[]
        out.check(len(treads)==st['count'],'stair count mismatch')
        for p in treads:
            f=p['timber_frame'];out.check(abs(f['u'][2])<1e-9 and abs(f['v'][2])<1e-9 and abs(f['w'][2]-1)<1e-9,p['id']+': tread top plane is not horizontal')
            rings=len(p['timber_profile']);ring_size=(len(p['vertices'])-2)//rings
            if ring_size==11:
                z=[p['vertices'][j*11+k][2] for j in range(rings) for k in (5,6,7,8,9)]
                spread=max(z)-min(z);level=sum(z)/len(z);spreads.append(spread);levels.append(level)
                out.check(spread<1e-8,p['id']+f': top geometry has {spread:.6f} m vertical spread')
                out.check(abs(level-p['tread_top_m'])<1e-8,p['id']+': top geometry disagrees with contract')
                out.check(abs(previous-level-rise)<1e-8,p['id']+': geometric rise not uniform');previous=level
            else:out.warnings.append(p['id']+': unknown profile topology; top-ring height check skipped')
        out.check(abs(previous-st['bottom_height_m'])<1e-8,'final stair height differs from bottom')
        out.metrics['stairs']={'count':len(treads),'rise_m':rise,'top_plane_spread_max_m':max(spreads,default=0),'geometric_top_levels_m':levels}
    if 'branch_attachment' in meta:
        branch=meta['branch_attachment'];origin=branch['start'];along=unit(sub(branch['end'],origin));across=(along[1],-along[0],0);width=recipe['branch']['width'];z=origin[2]
        blockers=[]
        for p in piles:
            c=p['timber_frame']['origin'];rel=sub(c,origin);x=dot(rel,along);y=dot(rel,across);radius=p.get('radius_m',p['timber_width']/2)
            # Interior entrance corridor excludes its boundary posts. The first
            # half metre on the main side must remain walkable at deck level.
            if -min(recipe['width']*.45,.5)-radius<x<.3+radius and abs(y)+radius<width*.4 and max(v[2] for v in p['vertices'])>z+.04:
                blockers.append({'id':p['id'],'along_m':x,'across_m':y,'radius_m':radius,'top_m':max(v[2] for v in p['vertices'])})
        out.check(not blockers,'pile heads obstruct the main/branch entrance: '+', '.join(p['id'] for p in blockers))
        out.metrics['branch_entry_obstructions']=blockers
        main=[hull([(dot(sub(v,origin),along),dot(sub(v,origin),across)) for v in p['vertices']]) for p in parts if p['role']=='DeckBoard']
        first=byid.get('branch.board.0');gaps=[]
        if first:
            poly=hull([(dot(sub(v,origin),along),dot(sub(v,origin),across)) for v in first['vertices']])
            for y in (-width*.3,0,width*.3):
                b=section(poly,y);m=[r for h in main if (r:=band_section(h,y)) is not None]
                if b and m:
                    near=max(r[1] for r in m);gaps.append({'across_m':y,'gap_m':b[0]-near})
            out.metrics['main_branch_measured_edge_gaps']=gaps
            out.check(all(-.02<g['gap_m']<.10 for g in gaps),'main/branch deck has excessive overlap or transition gap')
    out.warnings.append('Bearing bounds are checked with an explicit 1 mm authoring tolerance; this is not proof of exact contact area, joinery strength or load-bearing capacity. Attached splinters additionally require actual geometry contact.')

def check_glb(path,out):
    data=Path(path).read_bytes();magic,version,length=struct.unpack_from('<III',data);out.check((magic,version,length)==(0x46546c67,2,len(data)),'invalid GLB header')
    count,kind=struct.unpack_from('<II',data,12);out.check(kind==0x4e4f534a,'missing GLB JSON');d=json.loads(data[20:20+count]);binsize,kind=struct.unpack_from('<II',data,20+count);binary=memoryview(data)[28+count:];out.check(kind==0x004e4942 and binsize==len(binary),'invalid GLB BIN chunk')
    cache={}
    def accessor(index):
        if index in cache:return cache[index]
        a=d['accessors'][index];view=d['bufferViews'][a['bufferView']];code,size={5126:('f',4),5125:('I',4),5123:('H',2),5121:('B',1)}[a['componentType']];components={'SCALAR':1,'VEC2':2,'VEC3':3,'VEC4':4}[a['type']];stride=view.get('byteStride',components*size);start=view.get('byteOffset',0)+a.get('byteOffset',0)
        out.check(a.get('byteOffset',0)+(a['count']-1)*stride+components*size<=view['byteLength'],'accessor exceeds buffer view')
        rows=[struct.unpack_from('<'+code*components,binary,start+i*stride) for i in range(a['count'])];out.check(all(finite(r) for r in rows),'nonfinite GLB accessor');cache[index]=rows;return rows
    for i in range(len(d['accessors'])):accessor(i)
    triangles=vertices=0
    for mesh in d['meshes']:
        for prim in mesh['primitives']:
            attrs=prim['attributes'];out.check(all(k in attrs for k in ('POSITION','NORMAL','TANGENT','TEXCOORD_0','TEXCOORD_1')),mesh['name']+': missing GLB attributes')
            ps,ns,ts,uv,metric=[accessor(attrs[k]) for k in ('POSITION','NORMAL','TANGENT','TEXCOORD_0','TEXCOORD_1')];indices=[v[0] for v in accessor(prim['indices'])]
            out.check(len(ps)==len(ns)==len(ts)==len(uv)==len(metric),mesh['name']+': GLB attribute count mismatch')
            out.check(len(indices)%3==0 and all(type(i)==int and 0<=i<len(ps) for i in indices),mesh['name']+': GLB triangle indices invalid')
            out.check(all(abs(norm(n)-1)<2e-5 and abs(norm(t[:3])-1)<2e-5 and abs(dot(n,t[:3]))<2e-5 and t[3] in (-1,1) for n,t in zip(ns,ts)),mesh['name']+': invalid GLB normal/tangent frame')
            out.check(all(0<=v<=1 for r in uv for v in r),mesh['name']+': GLB atlas UV out of range')
            for i in range(0,len(indices),3):
                ia,ib,ic=indices[i:i+3];ng=cross(sub(ps[ib],ps[ia]),sub(ps[ic],ps[ia]));out.check(norm(ng)>0,mesh['name']+': degenerate GLB triangle');out.check(all(dot(ns[j],ng)>0 for j in (ia,ib,ic)),mesh['name']+': GLB normal hemisphere')
                a,b=sub(uv[ib],uv[ia]),sub(uv[ic],uv[ia]);out.check(abs(a[0]*b[1]-a[1]*b[0])>0,mesh['name']+': degenerate GLB UV triangle')
            # Export splits vertices by face loop. Weld only exact float32
            # positions to recover source topological identity for this asset.
            points=[];lookup={};mapping=[]
            for p in ps:
                if p not in lookup:lookup[p]=len(points);points.append(p)
                mapping.append(lookup[p])
            faces=[[mapping[j] for j in indices[i:i+3]] for i in range(0,len(indices),3)];topology(points,faces,mesh['name']+' GLB',out)
            vertices+=len(ps);triangles+=len(indices)//3
    out.metrics['glb']={'sha256':hashlib.sha256(data).hexdigest(),'bytes':len(data),'meshes':len(d['meshes']),'vertices':vertices,'triangles':triangles,'all_accessors_finite':all(finite(row) for rows in cache.values() for row in rows)}

def compare(scene,intact,repeat,out):
    fields=('id','vertices','faces','uvs','timber_frame','timber_profile','fractured','protected_support')
    def geom(p):return {k:p[k] for k in fields if k in p}
    if repeat:
        out.check([geom(p) for p in scene['parts']]==[geom(p) for p in repeat['parts']],'repeated generation changes native geometry')
        out.check(scene['sockets']==repeat['sockets'],'repeated generation changes sockets')
        out.metrics['deterministic_repeat_checked']=True
    if intact:
        baseline={p['id']:p for p in intact['parts']};changed=[];checked=0
        for p in scene['parts']:
            if p.get('fractured',False) or (p['role']=='DeckBoard' and p['id'] in scene['metadata'].get('damaged_board_ids',[])):continue
            if p['id'] in baseline:
                checked+=1
                if geom(p)!=geom(baseline[p['id']]):changed.append(p['id'])
        out.check(not changed,'regional damage changes unrelated/protected geometry: '+', '.join(changed));out.metrics['unchanged_outside_damage_parts']=checked
        intact_sockets={s['id']:s for s in intact.get('sockets',[])};invalidated=0
        for socket in scene.get('sockets',[]):
            if socket.get('invalidated_by')=='regional_break':
                invalidated+=1;original=intact_sockets.get(socket['id'])
                out.check(original is not None and original.get('enabled',False),socket['id']+': invalidation does not preserve an enabled intact socket identity')
        out.metrics['invalidated_socket_identities_checked']=invalidated

def main():
    parser=argparse.ArgumentParser(description=__doc__);parser.add_argument('--scene',type=Path,required=True);parser.add_argument('--glb',type=Path);parser.add_argument('--intact-scene',type=Path);parser.add_argument('--repeat-scene',type=Path);parser.add_argument('--report',type=Path);args=parser.parse_args()
    load=lambda p:json.loads(p.read_text()) if p else None
    scene=load(args.scene);out=Checks();check_scene(scene,out)
    if args.glb:check_glb(args.glb,out)
    compare(scene,load(args.intact_scene),load(args.repeat_scene),out)
    report=out.finish();text=json.dumps(report,indent=2,ensure_ascii=False)
    if args.report:args.report.write_text(text+'\n')
    print(text);return int(bool(out.errors))
if __name__=='__main__':raise SystemExit(main())
