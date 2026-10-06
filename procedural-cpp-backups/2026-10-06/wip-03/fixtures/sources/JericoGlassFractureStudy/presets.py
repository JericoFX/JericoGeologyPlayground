"""Reference-guided original variants layered on the tested scene contract.

References are artistic guidance only; no reference pixels become materials.
The original mine_timber recipe stays byte-for-byte unchanged.
"""
import math
import random
from core import build_asset,Builder,clearance_sweep


def build_preset(recipe):
    recipe=dict(recipe)
    preset=recipe.pop('preset','mine_timber')
    if preset not in ('mine_timber','reinforced_timber'):
        raise ValueError('Unknown preset: '+str(preset))
    scene=build_asset(recipe)
    if preset=='mine_timber': return scene
    r=scene['recipe']; w,h,t=[r[k] for k in ('width','height','thickness')]
    b=Builder(r); b.scene=scene
    rng=random.Random(r['seed']+9021)
    f=.115 if r['kind']=='door' else .09
    scale=1.5; ff=f*scale
    def widen(x):
        if abs(x)<=w/2:return x
        return math.copysign(w/2+(abs(x)-w/2)*scale,x)
    # Increase external timber mass without shrinking the authored clear opening.
    for part in scene['parts']:
        if part['material']=='wood':part['material']='wood_rough'
        if part['role']=='Frame':
            for v in part['vertices']:
                v[0]=widen(v[0])
                if v[2]>h:v[2]=h+(v[2]-h)*scale
                if v[2]<0:v[2]*=scale
            from core import _uvs
            part['uvs']=_uvs(part['vertices'],part['faces'])
    for envelope in scene['metadata']['frame_envelopes']:
        envelope['rect']=[[widen(x),y] for x,y in envelope['rect']]
        if envelope['z'][1]>h:envelope['z'][1]=h+(envelope['z'][1]-h)*scale
        if envelope['z'][0]<0:envelope['z'][0]*=scale
    fy=-t/2-.01
    for side,sign in [('left',-1),('right',1)]:
        cx=sign*(w/2+ff*.51)
        # Distinct perpendicular plates form substantial, bolted corner joinery.
        for tag,z0,z1 in [('upper',h-.13,h+.045),('foot',.026,.19)]:
            b.box('reinforced.frame.'+side+'.'+tag,'frame',cx-.046,cx+.046,fy-.008,fy-.001,z0,z1,'iron',.002,'CornerReinforcement')
            for j,z in enumerate([z0+.025,z1-.025]):b.rivet('reinforced.frame.'+side+'.'+tag+'.bolt.'+str(j),'frame',cx,fy-.008,z,True,.012)
        b.box('reinforced.frame.'+side+'.head','frame',cx-.067,cx+.067,fy-.01,fy-.002,h+.025,h+.087,'iron',.002,'CornerReinforcement')
        for j,x in enumerate([cx-.044,cx+.044]):b.rivet('reinforced.frame.'+side+'.head.bolt.'+str(j),'frame',x,fy-.01,h+.057,True,.011)
    if r['kind']=='door':
        node='door.leaf'; dims=scene['metadata']['dimensions']; lw=dims['leaf_width']; py=dims['pivot_front_offset']
        # Keep original hinge rivets visibly proud of the wider, thicker bands.
        for part in scene['parts']:
            if part['id'].startswith('door.hinge.') and '.rivet.' in part['id']:
                for v in part['vertices']:v[1]-=.006
        # Three broad horizontal bands replace the light baseline hinge straps.
        for i,z in enumerate([h*.13,h*.51,h*.88]):
            old='door.hinge.'+str(i)+'.strap'
            scene['parts']=[p for p in scene['parts'] if p['id']!=old]
            b.box(old,node,.006,lw-.006,py-.014,py-.004,z-.041,z+.041,'iron',.0018,'HingeStrap')
            for j,fraction in enumerate([.13,.31,.50,.69,.87]):
                b.rivet('reinforced.band.'+str(i)+'.bolt.'+str(j),node,lw*fraction,py-.014,z,True,.011)
        for i,(a,c) in enumerate([((.042,h*.855),(lw-.042,h*.535)),((.042,h*.155),(lw-.042,h*.485))]):
            dx,dz=c[0]-a[0],c[1]-a[1];length=math.hypot(dx,dz); ox,oz=-dz/length*.025,dx/length*.025
            profile=[(a[0]-ox,a[1]-oz),(c[0]-ox,c[1]-oz),(c[0]+ox,c[1]+oz),(a[0]+ox,a[1]+oz)]
            b.prism('reinforced.diagonal.'+str(i),node,profile,py-.012,py-.003,'iron',.0014,'DiagonalReinforcement')
            for j,q in enumerate([.12,.34,.56,.78,.93]):b.rivet('reinforced.diagonal.'+str(i)+'.bolt.'+str(j),node,a[0]+dx*q,py-.012,a[1]+dz*q,True,.009)
        for j,x in enumerate([.018,lw-.048]):
            b.box('reinforced.leaf.edge.'+str(j),node,x,x+.03,py-.012,py-.005,.035,h-.038,'iron',.0015,'EdgeReinforcement')
    else:
        for node in [n['id'] for n in scene['nodes'] if n['role']=='Leaf']:
            for part in list(scene['parts']):
                if part['node']!=node or part['role']!='CasementFrame':continue
                xs=[v[0] for v in part['vertices']]; zs=[v[2] for v in part['vertices']]; ys=[v[1] for v in part['vertices']]
                if max(zs)-min(zs)<max(xs)-min(xs):continue
                cx=(min(xs)+max(xs))/2
                for k,z in enumerate([h*.17,h*.83]):
                    b.box('reinforced.'+part['id']+'.clip.'+str(k),node,cx-.019,cx+.019,min(ys)-.007,min(ys)-.001,z-.045,z+.045,'iron',.001,'CasementClip')
                    b.rivet('reinforced.'+part['id']+'.clip.'+str(k)+'.bolt',node,cx,min(ys)-.007,z,True,.0065)
    # Finite, irregular longitudinal checks sit at each surviving board surface.
    # These are closed shallow prisms, never holes or floating texture planes.
    for part in list(scene['parts']):
        if part['material']!='wood_rough' or part['role'] not in ('Plank','Frame','CasementFrame'):continue
        xs=[v[0] for v in part['vertices']]; zs=[v[2] for v in part['vertices']]; ys=[v[1] for v in part['vertices']]
        xmin,xmax=min(xs),max(xs); zmin,zmax=min(zs),max(zs); y=min(ys)
        if zmax-zmin < .3 or xmax-xmin<.035:continue
        for j in range(4 if part['role']=='Plank' else 2):
            x=rng.uniform(xmin+.01,xmax-.01); z=rng.uniform(zmin+.035,zmax-.21)
            length=min(rng.uniform(.13,.63),zmax-z-.015); wide=rng.uniform(.0008,.002)
            profile=[(x-wide,z),(x+wide*.65,z+length*.12),(x+wide*.35,z+length),(x-wide*.55,z+length*.77)]
            b.prism('wear.'+part['id']+'.check.'+str(j),part['node'],profile,y-.0004,y+.0002,'grime',.0001,'WeatherCheck')
    scene['metadata']['preset']=preset
    scene['metadata']['art_direction']='Original reinforced mine timber: deep joints, bolted iron bands, visible diagonal bracing, localized longitudinal checking and corrosion. Guided by user reference, not a pixel/texture copy.'
    scene['recipe']=dict(r,preset=preset)
    scene['metadata']['clearance']=clearance_sweep(scene)
    return scene
