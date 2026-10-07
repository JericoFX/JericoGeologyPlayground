"""Deterministic, dependency-free mine door/window mesh and interaction authoring.

All coordinates and UV distances are meters. Mesh vertices are node-local;
rotation_z is radians. Width and height describe the frame's CLEAR opening.
This is an authoring contract, not an engine/physics integration.
"""
from __future__ import annotations
import collections
import copy
import hashlib
import json
import math
import random

TAU=2*math.pi
DEFAULTS={'kind':'door','width':1.0,'height':2.1,'thickness':.052,'damage':'aged','handle':'lever','seed':17,'angle':0.0}


def _sub(a,b): return [a[i]-b[i] for i in range(3)]
def _add(a,b): return [a[i]+b[i] for i in range(3)]
def _mul(a,s): return [v*s for v in a]
def _dot(a,b): return sum(x*y for x,y in zip(a,b))
def _cross(a,b): return [a[1]*b[2]-a[2]*b[1],a[2]*b[0]-a[0]*b[2],a[0]*b[1]-a[1]*b[0]]
def _norm(a):
    length=math.sqrt(_dot(a,a))
    return _mul(a,1/length)


def normalize_recipe(recipe):
    if not isinstance(recipe,dict): raise TypeError('Recipe must be an object')
    r=dict(DEFAULTS)
    if recipe.get('kind')=='window': r.update(width=1.4,height=1.25,thickness=.045)
    unknown=set(recipe)-set(r)
    if unknown: raise ValueError('Unknown recipe fields: '+', '.join(sorted(unknown)))
    r.update(recipe)
    for key,options in [('kind',('door','window')),('damage',('aged','deteriorated','broken')),('handle',('lever','pull'))]:
        if r[key] not in options: raise ValueError(key+' must be one of '+str(options))
    for key,limits in [('width',(.65,2.4)),('height',(.8,3.2)),('thickness',(.028,.12)),('angle',(0,105 if r['kind']=='door' else 85))]:
        value=r[key]
        if isinstance(value,bool) or not isinstance(value,(float,int)) or not math.isfinite(value): raise ValueError(key+' must be finite and numeric')
        if not limits[0]<=value<=limits[1]: raise ValueError(key+' outside supported range '+str(limits))
        r[key]=float(value)
    if isinstance(r['seed'],bool) or not isinstance(r['seed'],int): raise ValueError('seed must be an integer')
    return r


def asset_hash(scene):
    return hashlib.sha256(json.dumps(scene,sort_keys=True,separators=(',',':'),allow_nan=False).encode()).hexdigest()


def world_point(scene,node_id,point):
    nodes={n['id']:n for n in scene['nodes']}
    p=list(point); seen=set()
    while node_id is not None:
        if node_id in seen: raise ValueError('Cyclic node hierarchy')
        seen.add(node_id)
        n=nodes[node_id]; a=n['rotation_z']; c,s=math.cos(a),math.sin(a)
        p=[p[0]*c-p[1]*s+n['translation'][0],p[0]*s+p[1]*c+n['translation'][1],p[2]+n['translation'][2]]
        node_id=n['parent']
    return p


def _uvs(vertices,faces):
    result=[]
    for face in faces:
        a,b,c=[vertices[i] for i in face[:3]]
        normal=_norm(_cross(_sub(b,a),_sub(c,a)))
        reference=[1,0,0] if abs(normal[0])<.9 else [0,1,0]
        u=_norm(_sub(reference,_mul(normal,_dot(reference,normal)))); v=_cross(normal,u)
        result.append([[_dot(p,u),_dot(p,v)] for p in [vertices[i] for i in face]])
    return result


def _triangulate_profile(profile):
    """Ear-clip a simple counterclockwise XZ outline, including chipped corners."""
    remaining=list(range(len(profile))); result=[]
    def cross(a,b,c):
        return (b[0]-a[0])*(c[1]-a[1])-(b[1]-a[1])*(c[0]-a[0])
    while len(remaining)>3:
        found=False
        for k,mid in enumerate(remaining):
            prev=remaining[k-1]; nxt=remaining[(k+1)%len(remaining)]
            a,b,c=profile[prev],profile[mid],profile[nxt]
            if cross(a,b,c)<=1e-14: continue
            if any(cross(a,b,profile[j])>=-1e-14 and cross(b,c,profile[j])>=-1e-14 and cross(c,a,profile[j])>=-1e-14 for j in remaining if j not in (prev,mid,nxt)): continue
            result.append([prev,mid,nxt]); remaining.pop(k); found=True; break
        if not found: raise ValueError('Profile cannot be triangulated: degenerate or self intersecting')
    result.append(remaining)
    return result


class Builder:
    def __init__(self,recipe):
        self.r=recipe
        self.scene={'schema_version':1,'units':'m','basis':{'handedness':'right','up':'+Z','front':'-Y'},'recipe':recipe,'nodes':[],'parts':[],'sockets':[],'grips':[],'metadata':{'generator':'JericoDoorsWindowsLab/core.py','source_geometry':'Original procedural geometry; no imported assets','uv_convention':'Orthonormal metric planar projection per surface. Coplanar triangulated caps share a common basis. Coordinates are meters and intentionally not packed into 0..1.','socket_coverage_convention':'coverage.footprint and component footprint describe full mounting plate width/height on supporting wood, not the lateral or swept extent of the lever. max_projection bounds static outward depth only. support_part_ids names actual supporting solids. Handle/frame and handle/handle motion clearance is not certified.','grip_supports':{},'clearance':{},'missing_parts':[],'orientation_convention':'Socket/grip basis columns x,y,z are orthonormal right-handed axes in node-local coordinates. Socket normal equals basis.z (outward mounting normal, or hinge pin insertion axis); grip basis.x follows grasp bar and basis.z is outward surface normal. Grip approach is -basis.z. HandleBack rotation_z is pi. Transform axes by owning node rotation for world space.'}}
        self.rng=random.Random(recipe['seed'])
    def node(self,id,translation,role,parent='frame',angle=0):
        self.scene['nodes'].append({'id':id,'parent':parent,'translation':translation,'rotation_z':angle,'role':role})
    def mesh(self,id,node,vertices,faces,material='wood',bevel=.0015,role='Structure'):
        p={'id':id,'node':node,'material':material,'vertices':vertices,'faces':faces,'uvs':_uvs(vertices,faces),'bevel':bevel,'role':role}
        self.scene['parts'].append(p)
        return p
    def prism(self,id,node,profile,y0,y1,material='wood',bevel=.0015,role='Structure'):
        count=len(profile)
        verts=[[x,y0,z] for x,z in profile]+[[x,y1,z] for x,z in profile]
        caps=_triangulate_profile(profile)
        faces=caps+[[count+i for i in reversed(triangle)] for triangle in caps]+[[i,count+i,count+(i+1)%count,(i+1)%count] for i in range(count)]
        return self.mesh(id,node,verts,faces,material,bevel,role)
    def box(self,id,node,x0,x1,y0,y1,z0,z1,material='wood',bevel=.0015,role='Structure'):
        return self.prism(id,node,[(x0,z0),(x1,z0),(x1,z1),(x0,z1)],y0,y1,material,bevel,role)
    def chipped_board(self,id,node,x0,x1,y0,y1,z0,z1,severity=1,role='Plank'):
        # Convex irregular silhouette guarantees sound cap triangulation and bounded chips.
        w=x1-x0; h=z1-z0
        c=min(.012*severity,w*.16,h*.08)
        cuts=[c*self.rng.uniform(.4,1) for _ in range(8)]
        p=[(x0+cuts[0],z0),(x1-cuts[1],z0),(x1,z0+cuts[2]),(x1,z1-cuts[3]),(x1-cuts[4],z1),(x0+cuts[5],z1),(x0,z1-cuts[6]),(x0,z0+cuts[7])]
        if severity>=1.5 and role=='Plank':
            p=p[:1]+[(x0+w*.24,z0),(x0+w*.29,z0+c*1.3),(x0+w*.34,z0),(x0+w*.67,z0),(x0+w*.72,z0+c*2.1),(x0+w*.79,z0)]+p[1:]
        return self.prism(id,node,p,y0,y1,'wood',.0009,role)
    def cylinder(self,id,node,a,b,radius,material='iron',sides=12,bevel=.0006,role='Hardware'):
        direction=_norm(_sub(b,a)); ref=[0,0,1] if abs(direction[2])<.9 else [1,0,0]
        u=_norm(_cross(ref,direction)); v=_cross(direction,u)
        ring=[_add(_mul(u,radius*math.cos((i-.5)*TAU/sides)),_mul(v,radius*math.sin((i-.5)*TAU/sides))) for i in range(sides)]
        vertices=[_add(p,r) for p in [a,b] for r in ring]
        faces=[list(reversed(range(sides))),list(range(sides,2*sides))]+[[i,(i+1)%sides,sides+(i+1)%sides,sides+i] for i in range(sides)]
        return self.mesh(id,node,vertices,faces,material,bevel,role)
    def socket(self,id,node,role,interface,position,coverage,enabled=True):
        normal=[0,1,0] if role=='HandleBack' else ([0,0,1] if role=='Hinge' else [0,-1,0])
        x=[-1,0,0] if role=='HandleBack' else [1,0,0]
        basis={'x':x,'y':_cross(normal,x),'z':normal}
        s={'id':id,'node':node,'role':role,'interface':interface,'translation':position,'rotation_z':math.pi if role=='HandleBack' else 0.0,'normal':normal,'basis':basis,'enabled':enabled,'coverage':coverage}
        self.scene['sockets'].append(s)
        return s
    def rivet(self,id,node,x,y,z,front=True,radius=.009):
        direction=-1 if front else 1
        self.cylinder(id,node,[x,y,z],[x,y+direction*.005,z],radius,'iron',10,.001,'Rivet')
    def handle(self,id,node,x,y,z,front,enabled=True,style=None,support_id=None,support_width=.22,lever_direction=-1):
        style=style or self.r['handle']; sign=-1 if front else 1; role='HandleFront' if front else 'HandleBack'
        socket=self.socket(id+'.socket',node,role,'mount.handle.v1',[x,y,z],{'footprint':[support_width,.27],'support_part_ids':[support_id] if support_id else [],'max_projection':.14,'support_thickness':self.r['thickness'],'capabilities':['hand_grasp','two_bolt_mount'],'styles':['lever','pull']},enabled)
        grasp_x=[lever_direction,0,0] if style=='lever' else [0,0,1]; grasp_z=[0,sign,0]
        grip={'basis':{'x':grasp_x,'y':_cross(grasp_z,grasp_x),'z':grasp_z},'id':id+'.grip','node':node,'socket_id':socket['id'],'position':[x,y+sign*.07,z],'approach':[0,-sign,0],'reach_radius':.28,'enabled':enabled}
        self.scene['grips'].append(grip)
        if not enabled:
            self.scene['metadata']['missing_parts'].append(id+'.assembly')
            return
        self.box(id+'.plate',node,x-.025,x+.025,min(y,y+sign*.006),max(y,y+sign*.006),z-.11,z+.11,'iron',.003,'HandlePlate')
        for idx,dz in enumerate([-.083,.083]): self.rivet(id+'.bolt.'+str(idx),node,x,y+sign*.006,z+dz,front,.007)
        if style=='lever':
            self.cylinder(id+'.spindle',node,[x,y+sign*.006,z],[x,y+sign*.065,z],.016,'iron')
            part=self.cylinder(id+'.lever',node,[x,y+sign*.065,z],[x+lever_direction*.145,y+sign*.065,z],.012,'iron',12,.001,'GraspSurface')
        else:
            for idx,dz in enumerate([-.08,.08]):
                self.cylinder(id+'.mount.'+str(idx),node,[x,y+sign*.006,z+dz],[x,y+sign*.06,z+dz],.011,'iron')
                self.cylinder(id+'.elbow.'+str(idx),node,[x,y+sign*.06,z+dz],[x,y+sign*.08,z+dz*.75],.011,'iron')
            part=self.cylinder(id+'.pull',node,[x,y+sign*.08,z-.06],[x,y+sign*.08,z+.06],.011,'iron',12,.001,'GraspSurface')
        # Center of the outward lateral face survives small bevels at cylinder edges.
        centers=[[sum(part['vertices'][i][axis] for i in face)/len(face) for axis in range(3)] for face in part['faces'] if len(face)==4]
        grip['position']=max(centers,key=lambda center:center[1]*sign)
        self.scene['metadata']['grip_supports'][grip['id']]=part['id']


def _frame(b):
    w,h,t=[b.r[k] for k in ['width','height','thickness']]
    f=.115 if b.r['kind']=='door' else .09
    ymin=-t/2-.010; ymax=t/2+.042
    b.node('frame',[0,0,0],'Frame',None)
    for side,x0,x1 in [('left',-w/2-f,-w/2),('right',w/2,w/2+f)]:
        b.chipped_board('frame.'+side,'frame',x0,x1,ymin,ymax,0,h,1.1,'Frame')
        for index,z in enumerate([.14,h*.51,h-.10]): b.rivet('frame.'+side+'.bolt.'+str(index),'frame',(x0+x1)/2,ymin,z,True,.014)
        # Localized edge weathering tucked against joinery, not random floating noise.
        b.box('frame.'+side+'.grime','frame',x0+.005,x0+.013,ymin-.001,ymin+.001,.045,h-.03,'grime',.0003,'DirtSeam')
    b.chipped_board('frame.head','frame',-w/2-f,w/2+f,ymin,ymax,h,h+f,1.2,'Frame')
    if b.r['kind']=='window': b.chipped_board('frame.sill','frame',-w/2-f-.025,w/2+f+.025,ymin-.027,ymax,-f,0,1.3,'Frame')
    b.socket('frame.opening','frame','FrameOpening','opening.rect.v1',[0,0,0],{'opening':[w,h,t],'capabilities':['hinged_leaf'],'max_leaf_count':1 if b.r['kind']=='door' else 2})
    b.scene['metadata']['frame_envelopes']=[{'id':'frame.left','rect':[[-w/2-f,ymin],[-w/2,ymin],[-w/2,ymax],[-w/2-f,ymax]],'z':[0,h]},{'id':'frame.right','rect':[[w/2,ymin],[w/2+f,ymin],[w/2+f,ymax],[w/2,ymax]],'z':[0,h]},{'id':'frame.head','rect':[[-w/2-f,ymin],[w/2+f,ymin],[w/2+f,ymax],[-w/2-f,ymax]],'z':[h,h+f]}]
    if b.r['kind']=='window': b.scene['metadata']['frame_envelopes'].append({'id':'frame.sill','rect':[[-w/2-f,ymin-.027],[w/2+f,ymin-.027],[w/2+f,ymax],[-w/2-f,ymax]],'z':[-f,0]})


def _hinge(b,node,prefix,x,z,front_y,mirror=1,strap_length=.34):
    # Long pointed strap on leaf with rivets; fixed barrel and frame strap share pivot.
    thickness=.006
    profile=[(x,z-.022),(x+mirror*(strap_length-.035),z-.022),(x+mirror*strap_length,z),(x+mirror*(strap_length-.035),z+.022),(x,z+.022)]
    if mirror<0: profile.reverse()
    b.prism(prefix+'.strap',node,profile,front_y-.009,front_y-.003,'iron',.001,'HingeStrap')
    for i,fraction in enumerate([.15,.55,.86]): b.rivet(prefix+'.rivet.'+str(i),node,x+mirror*strap_length*fraction,front_y-.009,z,True)
    pivot=world_point(b.scene,node,[0,0,z])
    # Barrel is stationary on the common hinge axis and safely ahead of frame.
    for i,(a,c) in enumerate([(-.033,-.013),(-.011,.011),(.013,.033)]):
        b.cylinder(prefix+'.barrel.'+str(i),'frame',[pivot[0],pivot[1],z+a],[pivot[0],pivot[1],z+c],.014,'rust' if i==1 else 'iron',12,.001,'HingeBarrel')
    frame_x=pivot[0]-mirror*.068
    b.box(prefix+'.frame_plate','frame',min(frame_x,pivot[0]),max(frame_x,pivot[0]),pivot[1]+.004,pivot[1]+.011,z-.023,z+.023,'iron',.001,'HingePlate')
    b.socket(prefix+'.socket',node,'Hinge','hinge.pin.v1',[0,0,z],{'footprint':[.035,.07],'pin_diameter':.028,'capabilities':['rotate_z'],'range_degrees':[0,105 if b.r['kind']=='door' else 85]})


def _door(b):
    w,h,t=[b.r[k] for k in ['width','height','thickness']]; p=.024
    # Rear brace increases sweep radius. Solve the radial corner clearance
    # inequality, with a 3 mm margin, rather than using a fixed gap on thick leaves.
    swept_depth=p+t+.047
    g=max(.014,(2*w-math.sqrt(4*w*w-12*swept_depth*swept_depth))/6+.003)
    lw=w-2*g
    b.node('door.leaf',[-w/2+g,-t/2-p,0],'Leaf',angle=-math.radians(b.r['angle']))
    node='door.leaf'; y0=p; y1=p+t; broken=b.r['damage']=='broken'; severity={'aged':1,'deteriorated':2.1,'broken':3}[b.r['damage']]
    n=6; pitch=lw/n; gap=.0035
    weights=[b.rng.uniform(.93,1.07) for _ in range(n)]; total=sum(weights); boundaries=[0.0]
    for weight in weights: boundaries.append(boundaries[-1]+lw*weight/total)
    for i in range(n):
        id='door.plank.'+str(i)
        if broken and i==4:
            b.scene['metadata']['missing_parts'].append(id); continue
        bottom=g+(min(.09,h*.05) if b.r['damage']=='deteriorated' and i==2 else 0)
        if broken and i==3: bottom=h*.39
        b.chipped_board(id,node,boundaries[i]+gap/2,boundaries[i+1]-gap/2,y0,y1,bottom,h-g,severity)
        # Thin dark cracks in the plank surface remain inset in the wood thickness.
        if i%2==0:
            crackx=boundaries[i]+.42*(boundaries[i+1]-boundaries[i])
            b.box(id+'.check',node,crackx,crackx+.0015,y0-.0004,y0+.0003,max(bottom+.07,h*.16),h*.72,'grime',.0001,'WoodCheck')
    for i,z in enumerate([h*.18,h*.5,h*.82]):
        b.chipped_board('door.rail.'+str(i),node,.018,lw-.018,y1-.001,y1+.026,z-.059,z+.059,severity,'CrossRail')
        for j in range(6): b.rivet('door.rail.'+str(i)+'.bolt.'+str(j),node,(j+.5)*pitch,y1+.026,z,False,.007)
    # Back diagonal brace, an original extruded polygon.
    xa,xb=.055,lw-.055; za,zb=h*.22,h*.79; dz=.037
    b.prism('door.diagonal',node,[(xa,za-dz),(xb,zb-dz),(xb,zb+dz),(xa,za+dz)],y1+.026,y1+.047,'wood',.002,'DiagonalBrace')
    for i,z in enumerate([h*.13,h*.51,h*.88]): _hinge(b,node,'door.hinge.'+str(i),0,z,y0,strap_length=min(.40,lw*.48))
    hx=(boundaries[4]+boundaries[5])/2; hz=h*.48
    support_width=boundaries[5]-boundaries[4]-gap-.010
    b.handle('door.handle.front',node,hx,y0,hz,True,not broken,support_id='door.plank.4',support_width=support_width)
    b.handle('door.handle.back',node,hx,y1,hz,False,not broken,support_id='door.plank.4',support_width=support_width)
    b.socket('door.lock',node,'Lock','lock.mortise.v1',[hx,y0,hz-.16],{'footprint':[.047,.079],'capabilities':['keyed'],'support_thickness':t},not broken)
    if not broken:
        b.prism('door.lock.escutcheon',node,[(hx-.022,hz-.205),(hx+.022,hz-.205),(hx+.025,hz-.14),(hx,hz-.12),(hx-.025,hz-.14)],y0-.006,y0-.001,'iron',.001,'Escutcheon')
        b.cylinder('door.lock.keyhole.round',node,[hx,y0-.0065,hz-.155],[hx,y0-.007,hz-.155],.006,'grime',12,0,'Keyhole')
        b.prism('door.lock.keyhole.slot',node,[(hx-.0035,hz-.181),(hx+.0035,hz-.181),(hx+.002,hz-.156),(hx-.002,hz-.156)],y0-.0072,y0-.0068,'grime',0,'Keyhole')
    b.socket('door.leaf.mount',node,'Leaf','leaf.hinged.v1',[lw/2,(y0+y1)/2,h/2],{'footprint':[lw,h-2*g],'capabilities':['hinged_leaf'],'damage':b.r['damage']})
    b.scene['metadata']['leaf_envelopes']=[{'node':node,'rect':[[0,y0],[lw,y0],[lw,y1+.047],[0,y1+.047]],'z':[g,h-g],'angle_sign':-1,'maximum_degrees':105}]
    b.scene['metadata']['dimensions']={'clear_opening':[w,h,t],'leaf_width':lw,'frame_member':.115,'edge_gap':g,'pivot_front_offset':p}


def _window(b):
    w,h,t=[b.r[k] for k in ['width','height','thickness']]; g=.014; p=.024
    pivot_span=w-2*g; depth=p+t
    # Keep both rotating inner corners on their own side of the center plane.
    center_gap=max(.022,pivot_span-math.sqrt(pivot_span*pivot_span-4*depth*depth)+.006)
    lw=w/2-g-center_gap/2; stile=min(.062,lw*.20)
    broken=b.r['damage']=='broken'; severity={'aged':1,'deteriorated':2,'broken':3}[b.r['damage']]
    envelopes=[]
    for side,mirror in [('left',1),('right',-1)]:
        node='window.'+side; px=(-w/2+g) if mirror==1 else (w/2-g)
        b.node(node,[px,-t/2-p,0],'Leaf',angle=-mirror*math.radians(b.r['angle']))
        def xx(a,c): return sorted([mirror*a,mirror*c])
        for tag,xa,xb,za,zb in [('outer',0,stile,g,h-g),('inner',lw-stile,lw,g,h-g),('top',stile,lw-stile,h-g-stile,h-g),('bottom',stile,lw-stile,g,g+stile)]:
            id=node+'.stile.'+tag
            if broken and side=='left' and tag=='inner': b.scene['metadata']['missing_parts'].append(id); continue
            a,c=xx(xa,xb)
            b.chipped_board(id,node,a,c,p,p+t,za,zb,severity,'CasementFrame')
        # True 2 x 3 mullioned glazed openings per leaf.
        mx=lw/2; mw=.018; rows=3
        a,c=xx(mx-mw/2,mx+mw/2)
        b.box(node+'.mullion.vertical',node,a,c,p+.009,p+t-.008,g+stile,h-g-stile,'wood',.001,'Mullion')
        iz0=g+stile; iz1=h-g-stile; row=(iz1-iz0)/rows
        for j in [1,2]:
            z=iz0+row*j; a,c=xx(stile,lw-stile)
            b.box(node+'.mullion.horizontal.'+str(j),node,a,c,p+.009,p+t-.008,z-.009,z+.009,'wood',.001,'Mullion')
        for col,(xa,xb) in enumerate([(stile,mx-mw/2),(mx+mw/2,lw-stile)]):
            for j in range(rows):
                pane=node+'.glass.'+str(col)+'.'+str(j); missing=broken and side=='left' and (j==1 or (col==1 and j==0))
                a,c=xx(xa+.002,xb-.002); za=iz0+row*j+.012; zb=iz0+row*(j+1)-.012
                b.socket(pane+'.socket',node,'Glass','pane.rect.v1',[(a+c)/2,p+t*.5,(za+zb)/2],{'footprint':[c-a,zb-za],'thickness':.004,'capabilities':['glazing'],'max_thickness':.006},not missing)
                if missing: b.scene['metadata']['missing_parts'].append(pane); continue
                b.box(pane,node,a,c,p+t*.5-.002,p+t*.5+.002,za,zb,'glass',.0002,'GlassPane')
                # Built-up edge dirt is physically attached to pane edge.
                b.box(pane+'.grime',node,a,a+.006,p+t*.5-.0025,p+t*.5-.002,za,zb,'grime',.0001,'GlazingGrime')
        for i,z in enumerate([h*.2,h*.8]): _hinge(b,node,node+'.hinge.'+str(i),0,z,p,mirror,min(.21,lw*.6))
        hx=mirror*(lw-stile/2); hz=h*.48
        b.handle(node+'.handle',node,hx,p,hz,True,not (broken and side=='left'),support_id=node+'.stile.inner',support_width=stile-.004,lever_direction=-mirror)
        b.socket(node+'.latch',node,'Lock','latch.casement.v1',[hx,p,hz-.15],{'footprint':[.035,.07],'capabilities':['latch'],'support_thickness':t},not (broken and side=='left'))
        if not (broken and side=='left'):
            a,c=xx(lw-stile,lw+.005)
            b.box(node+'.latch.plate',node,a,c,p-.009,p-.004,hz-.165,hz-.14,'iron',.001,'Latch')
            b.rivet(node+'.latch.rivet',node,hx,p-.01,hz-.152,True,.007)
        a,c=xx(0,lw)
        b.socket(node+'.mount',node,'Leaf','leaf.casement.v1',[(a+c)/2,p+t/2,h/2],{'footprint':[lw,h-2*g],'capabilities':['hinged_leaf'],'damage':b.r['damage']})
        envelopes.append({'node':node,'rect':[[a,p],[c,p],[c,p+t],[a,p+t]],'z':[g,h-g],'angle_sign':-mirror,'maximum_degrees':85})
    b.scene['metadata']['leaf_envelopes']=envelopes
    b.scene['metadata']['dimensions']={'clear_opening':[w,h,t],'leaf_width':lw,'frame_member':.09,'edge_gap':g,'center_gap':center_gap,'pivot_front_offset':p}


def build_asset(recipe):
    r=normalize_recipe(recipe); b=Builder(r); _frame(b)
    if r['kind']=='door': _door(b)
    else: _window(b)
    b.scene['metadata']['clearance']=clearance_sweep(b.scene,step_degrees=.5)
    b.scene['metadata']['clearance']['scope']='Conservative leaf wood envelopes versus fixed frame and synchronized leaf-leaf motion; sampled SAT, not continuous physics. Hardware contact is not certified.'
    b.scene['metadata']['damage_policy']='Broken supporting wood removes its handle/lock assemblies and disables matching sockets and grips. Missing glass disables glazing socket.'
    return b.scene


def handle_interface(style,role='HandleFront'):
    if style not in ('lever','pull'): raise ValueError('Unknown handle style')
    return {'interface':'mount.handle.v1','role':role,'style':style,'footprint':[.05,.22],'projection':.10,'required_capabilities':['hand_grasp','two_bolt_mount'],'thickness_range':[.028,.12]}


def can_connect(socket,component_interface):
    """Fail-closed structural mating check, independent of asset-specific IDs."""
    def number(value,positive=False):
        return not isinstance(value,bool) and isinstance(value,(int,float)) and math.isfinite(value) and (value>0 if positive else value>=0)
    def pair(value,positive=True):
        return isinstance(value,(list,tuple)) and len(value)==2 and all(number(v,positive) for v in value)
    try:
        c=component_interface; coverage=socket['coverage']
        if not socket['enabled'] or socket['interface']!=c['interface'] or socket['role']!=c['role']: return False
        dimensions=c['footprint']; available=coverage.get('footprint',coverage.get('opening',[])[:2])
        if not pair(dimensions) or not pair(available): return False
        if any(a>b+1e-9 for a,b in zip(dimensions,available)): return False
        if not set(c.get('required_capabilities',[])).issubset(coverage.get('capabilities',[])): return False
        projection=c.get('projection',0)
        if not number(projection) or projection>coverage.get('max_projection',float('inf')): return False
        if 'style' in c and c['style'] not in coverage.get('styles',[]): return False
        if 'thickness_range' in c:
            limits=c['thickness_range']; thickness=coverage.get('support_thickness',0)
            if not pair(limits) or limits[0]>limits[1] or not number(thickness,True) or not limits[0]<=thickness<=limits[1]: return False
        if socket['interface']=='pane.rect.v1':
            thickness=c.get('thickness')
            if not number(thickness,True) or thickness>coverage['max_thickness']: return False
        if socket['interface']=='hinge.pin.v1':
            diameter=c.get('pin_diameter'); socket_diameter=coverage['pin_diameter']
            if not number(diameter,True) or abs(diameter-socket_diameter)>coverage.get('pin_tolerance',.0005): return False
        if 'range_degrees' in c:
            span=c['range_degrees']; allowed=coverage.get('range_degrees')
            if not pair(span,False) or not pair(allowed,False) or span[0]>span[1] or span[0]<allowed[0] or span[1]>allowed[1]: return False
        if socket['interface']=='opening.rect.v1' and 'thickness' in c:
            if not number(c['thickness'],True) or c['thickness']>coverage['opening'][2]: return False
        return True
    except (KeyError,TypeError,ValueError,IndexError,AttributeError): return False


def rectangles_overlap(a,b,tolerance=1e-10):
    """Separating-axis theorem for convex XY rectangles, including rotated ones.

    Mere contact is treated as nonpenetrating; positive overlap beyond tolerance
    on every separating axis means an intersection.
    """
    for polygon in [a,b]:
        for p,q in zip(polygon,polygon[1:]+polygon[:1]):
            dx,dy=q[0]-p[0],q[1]-p[1]; length=math.hypot(dx,dy)
            axis=(-dy/length,dx/length)
            pa=[v[0]*axis[0]+v[1]*axis[1] for v in a]; pb=[v[0]*axis[0]+v[1]*axis[1] for v in b]
            if min(max(pa),max(pb))-max(min(pa),min(pb))<=tolerance: return False
    return True


def clearance_sweep(scene,step_degrees=.5):
    if not math.isfinite(step_degrees) or step_degrees<=0: raise ValueError('Positive finite step required')
    collisions=[]; samples=0; pairs_tested=0
    nodes={n['id']:n for n in scene['nodes']}; leaves=scene['metadata']['leaf_envelopes']; frames=scene['metadata']['frame_envelopes']
    def transformed(leaf,angle):
        n=nodes[leaf['node']]; radians=math.radians(angle)*leaf['angle_sign']; c,s=math.cos(radians),math.sin(radians)
        return [[x*c-y*s+n['translation'][0],x*s+y*c+n['translation'][1]] for x,y in leaf['rect']]
    def z_overlap(a,b): return min(a['z'][1],b['z'][1])-max(a['z'][0],b['z'][0])>1e-10
    for leaf in leaves:
        steps=math.ceil(leaf['maximum_degrees']/step_degrees)
        for i in range(steps+1):
            angle=min(i*step_degrees,leaf['maximum_degrees']); rect=transformed(leaf,angle); samples+=1
            for frame in frames:
                if z_overlap(leaf,frame) and rectangles_overlap(rect,frame['rect']): collisions.append({'node':leaf['node'],'frame':frame['id'],'angle_degrees':angle})
    for index,leaf in enumerate(leaves):
        for other in leaves[index+1:]:
            maximum=min(leaf['maximum_degrees'],other['maximum_degrees']); steps=math.ceil(maximum/step_degrees)
            for i in range(steps+1):
                angle=min(i*step_degrees,maximum); pairs_tested+=1
                if z_overlap(leaf,other) and rectangles_overlap(transformed(leaf,angle),transformed(other,angle)):
                    collisions.append({'node':leaf['node'],'other_node':other['node'],'angle_degrees':angle})
    return {'method':'2D rectangle SAT + Z-interval separation; frame and synchronized leaf pairs','step_degrees':step_degrees,'samples':samples,'leaf_pair_samples':pairs_tested,'range_degrees':[0,min(l['maximum_degrees'] for l in leaves)],'collisions':collisions,'passed':not collisions}


def _point_triangle_distance(p,a,b,c):
    # Closest point on triangle, Ericson region tests, squared metric distance.
    ab=_sub(b,a); ac=_sub(c,a); ap=_sub(p,a); d1=_dot(ab,ap); d2=_dot(ac,ap)
    if d1<=0 and d2<=0: return math.sqrt(_dot(ap,ap))
    bp=_sub(p,b); d3=_dot(ab,bp); d4=_dot(ac,bp)
    if d3>=0 and d4<=d3: return math.sqrt(_dot(bp,bp))
    vc=d1*d4-d3*d2
    if vc<=0 and d1>=0 and d3<=0:
        q=_add(a,_mul(ab,d1/(d1-d3))); return math.sqrt(_dot(_sub(p,q),_sub(p,q)))
    cp=_sub(p,c); d5=_dot(ab,cp); d6=_dot(ac,cp)
    if d6>=0 and d5<=d6: return math.sqrt(_dot(cp,cp))
    vb=d5*d2-d1*d6
    if vb<=0 and d2>=0 and d6<=0:
        q=_add(a,_mul(ac,d2/(d2-d6))); return math.sqrt(_dot(_sub(p,q),_sub(p,q)))
    va=d3*d6-d5*d4
    if va<=0 and d4-d3>=0 and d5-d6>=0:
        q=_add(b,_mul(_sub(c,b),(d4-d3)/((d4-d3)+(d5-d6)))); return math.sqrt(_dot(_sub(p,q),_sub(p,q)))
    denominator=1/(va+vb+vc); q=_add(a,_add(_mul(ab,vb*denominator),_mul(ac,vc*denominator)))
    return math.sqrt(max(0,_dot(_sub(p,q),_sub(p,q))))


def grip_surface_distance(scene,grip):
    part_id=scene['metadata']['grip_supports'].get(grip['id'])
    part=next((p for p in scene['parts'] if p['id']==part_id),None)
    if part is None or part['node']!=grip['node']: return float('inf')
    distances=[]; vs=part['vertices']
    for f in part['faces']:
        for i in range(1,len(f)-1): distances.append(_point_triangle_distance(grip['position'],vs[f[0]],vs[f[i]],vs[f[i+1]]))
    return min(distances)


def validate_scene(scene):
    errors=[]
    if scene.get('schema_version')!=1: errors.append('Unsupported schema version')
    if scene.get('units')!='m': errors.append('Units must be meters')
    nodes={n['id']:n for n in scene['nodes']}
    for name in ['nodes','parts','sockets','grips']:
        ids=[o['id'] for o in scene[name]]
        if len(set(ids))!=len(ids): errors.append('Duplicate '+name+' IDs')
    for node in scene['nodes']:
        if node['parent'] is not None and node['parent'] not in nodes: errors.append(node['id']+': missing parent')
        if not all(math.isfinite(v) for v in node['translation']+[node['rotation_z']]): errors.append(node['id']+': nonfinite transform')
        try: world_point(scene,node['id'],[0,0,0])
        except (KeyError,ValueError): errors.append(node['id']+': invalid hierarchy')
    for part in scene['parts']:
        prefix=part['id']+': '
        if part['node'] not in nodes: errors.append(prefix+'missing node')
        if any(not math.isfinite(x) for v in part['vertices'] for x in v): errors.append(prefix+'nonfinite vertex')
        edges=collections.Counter(); volume=0.0
        if len(part['faces'])!=len(part['uvs']): errors.append(prefix+'UV count mismatch'); continue
        for f,uv in zip(part['faces'],part['uvs']):
            if len(f)<3 or len(uv)!=len(f): errors.append(prefix+'invalid face/UV'); continue
            if any(i<0 or i>=len(part['vertices']) for i in f): errors.append(prefix+'invalid index'); continue
            if any(not math.isfinite(v) for pair in uv for v in pair): errors.append(prefix+'nonfinite UV')
            for a,b in zip(f,f[1:]+f[:1]): edges[a,b]+=1
            a=part['vertices'][f[0]]
            for i in range(1,len(f)-1):
                b,c=part['vertices'][f[i]],part['vertices'][f[i+1]]; cross=_cross(_sub(b,a),_sub(c,a))
                if _dot(cross,cross)<=1e-20: errors.append(prefix+'zero area triangle')
                volume+=_dot(a,_cross(b,c))/6
        if any(count!=1 or edges[b,a]!=1 for (a,b),count in edges.items()): errors.append(prefix+'nonmanifold/inconsistent edges')
        if volume<=1e-12: errors.append(prefix+'nonpositive signed volume')
    sockets={s['id']:s for s in scene['sockets']}
    for socket in sockets.values():
        if socket['node'] not in nodes: errors.append(socket['id']+': missing node')
        if not all(math.isfinite(v) for v in socket['translation']+[socket['rotation_z']]): errors.append(socket['id']+': nonfinite socket transform')
        part_ids={p['id'] for p in scene['parts']}
        if socket['enabled'] and any(id not in part_ids for id in socket['coverage'].get('support_part_ids',[])): errors.append(socket['id']+': missing structural support')
    for grip in scene['grips']:
        if grip['socket_id'] not in sockets: errors.append(grip['id']+': missing socket')
        elif grip['enabled'] and not sockets[grip['socket_id']]['enabled']: errors.append(grip['id']+': enabled on disabled socket')
        if grip['enabled'] and grip_surface_distance(scene,grip)>1e-8: errors.append(grip['id']+': unsupported grasp')
    if clearance_sweep(scene).get('collisions'): errors.append('Leaf/frame sweep collision')
    return errors
