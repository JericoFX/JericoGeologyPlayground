#!/usr/bin/env python3
"""Offline fixtures for pure authoring interface functions, not production code."""
import argparse, copy, hashlib, json, math, sys
from pathlib import Path
p=argparse.ArgumentParser();p.add_argument('--source',type=Path,required=True);p.add_argument('--out',type=Path,required=True);a=p.parse_args();source=a.source.resolve();out=a.out.resolve();out.mkdir(parents=True,exist_ok=True);sys.path.insert(0,str(source))
from core import build_asset, handle_interface, can_connect, grip_surface_distance
scene=build_asset({});front=next(s for s in scene['sockets'] if s['role']=='HandleFront');back=next(s for s in scene['sockets'] if s['role']=='HandleBack');hinge=next(s for s in scene['sockets'] if s['role']=='Hinge');opening=scene['sockets'][0]
window=build_asset(dict(kind='window'));pane=next(s for s in window['sockets'] if s['role']=='Glass');tests=[]
def add(name,socket,component):tests.append(dict(name=name,socket=socket,component=component,expected=can_connect(socket,component)))
for role,socket in [('HandleFront',front),('HandleBack',back)]:
 for style in ('lever','pull'):add(role+'_'+style,socket,handle_interface(style,role))
for key,value in [('role','Glass'),('interface','mount.handle.v2'),('footprint',[10,10]),('footprint',[0,.22]),('footprint',[True,.22]),('required_capabilities',['electric']),('projection',.15),('projection',-1),('projection',True),('style','knob'),('thickness_range',[.08,.12]),('thickness_range',[.12,.028]),('thickness_range',[0,.12])]:
 c=handle_interface('pull');c[key]=value;add('handle_'+key+'_'+str(value),front,c)
s=copy.deepcopy(front);s['enabled']=False;add('disabled_handle',s,handle_interface('pull'))
for key in ('interface','role','footprint'):
 c=handle_interface('pull');del c[key];add('missing_component_'+key,front,c)
for c in (None,[],{},'invalid'):add('malformed_component_'+str(c),front,c)
base=dict(interface='pane.rect.v1',role='Glass',footprint=pane['coverage']['footprint'],required_capabilities=['glazing'],thickness=.004)
for value in (.004,.006,.0061,0,-1,True,None):c=dict(base,thickness=value);add('pane_thickness_'+str(value),pane,c)
base=dict(interface='hinge.pin.v1',role='Hinge',footprint=[.02,.05],pin_diameter=.028,range_degrees=[0,100])
for value in (.028,.0284,.029,0,True):c=dict(base,pin_diameter=value);add('hinge_diameter_'+str(value),hinge,c)
for value in ([0,105],[0,106],[10,0],[-1,10],[False,10],[0]):c=dict(base,range_degrees=value);add('hinge_range_'+str(value),hinge,c)
base=dict(interface='opening.rect.v1',role='FrameOpening',footprint=[.95,2],thickness=.052)
for value in (.052,.053,0,True):c=dict(base,thickness=value);add('opening_thickness_'+str(value),opening,c)
(out/'compatibility.json').write_text(json.dumps(tests,indent=2,allow_nan=False)+'\n')
# This minimal triangle scene exercises every vertex/edge/interior Voronoi region.
triangle=dict(metadata=dict(grip_supports={'g':'p'}),parts=[dict(id='p',node='n',vertices=[[0,0,0],[1,0,0],[0,1,0]],faces=[[0,1,2]])]);distances=[]
for point in ([0,0,0],[1,0,0],[0,1,0],[.2,.2,.7],[-.1,-.2,.3],[1.2,-.1,.2],[-.1,1.2,.2],[.3,-.1,.5],[-.2,.4,.5],[.8,.8,.3],[.5,.5,0],[.1,.2,0]):
 grip=dict(id='g',node='n',position=point);distances.append(dict(scene=triangle,grip=grip,expected=grip_surface_distance(triangle,grip)))
(out/'distances.json').write_text(json.dumps(distances,indent=2)+'\n')
(out/'provenance.json').write_text(json.dumps(dict(source_snapshot=source.name,core_sha256=hashlib.sha256((source/'core.py').read_bytes()).hexdigest(),compatibility_cases=len(tests),distance_cases=len(distances),scope='Pure authoring compatibility and support distance; no physics, UI or runtime interaction'),indent=2)+'\n')
print('Generated',len(tests),'compatibility cases and',len(distances),'point/triangle distance cases')
