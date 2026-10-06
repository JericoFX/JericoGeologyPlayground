"""Original metric industrial doors and fixed single-pane windows.

Authoring only: this module emits geometry and local support references. The
engine selects joints, locks, strength, looseness and failure behavior.
"""
from __future__ import annotations
import math
from core import Builder, normalize_recipe, clearance_sweep, _uvs

PRESETS=('industrial_solid','industrial_vented','industrial_louvered','industrial_window')

def _normalize(recipe):
 local=dict(recipe); preset=local.pop('preset','industrial_solid')
 if preset not in PRESETS: raise ValueError('Unknown industrial preset')
 operations=local.pop('operations',[]); glazing=local.pop('glazing','intact'); finish=local.pop('finish','weathered_gray')
 if finish not in ('weathered_gray','charcoal'):raise ValueError('Unknown finish')
 if not isinstance(operations,list): raise ValueError('operations must be a list')
 if glazing not in ('intact','broken','empty'): raise ValueError('Unknown glazing state')
 defaults=dict(kind='window' if preset=='industrial_window' else 'door',width=1.16 if preset=='industrial_window' else 1.0,height=1.25 if preset=='industrial_window' else 2.1,thickness=.052,seed=29)
 defaults.update(local); r=normalize_recipe(defaults);r['finish']=finish
 if r['kind']!=('window' if preset=='industrial_window' else 'door'): raise ValueError('kind and preset mismatch')
 if preset=='industrial_window' and r['angle']!=0: raise ValueError('Fixed glazing has no opening angle')
 return r,preset,glazing,operations

def _panel(b,id,node,x0,x1,y0,y1,z0,z1,material='painted_steel',bevel=.0015,role='Sheet'):
 p=b.box(id,node,x0,x1,y0,y1,z0,z1,material,bevel,role)
 p['panel_bounds']=[x0,x1,y0,y1,z0,z1]
 p['weather']={'edge_width_m':.013,'seed':b.r['seed'],'finish':b.r.get('finish','weathered_gray'),'bottom_height_m':max(.28,b.r['height']*.27),'bounds':p['panel_bounds'],'art_direction':'matte worn grey coating, localized bottom corrosion and joint rust'}
 return p

def _frame(b,window=False):
 w,h,t=(b.r[k] for k in ('width','height','thickness')); f=.07 if window else .09
 y0=-t/2-.017; y1=t/2+.033
 b.node('frame',[0,0,0],'Frame',None)
 for side,x0,x1 in [('left',-w/2-f,-w/2),('right',w/2,w/2+f)]:
  p=_panel(b,'frame.'+side,'frame',x0,x1,y0,y1,-f if window else 0,h+f,role='Frame')
  for i,z in enumerate((h*.09,h*.91)):
   b.rivet('frame.'+side+'.anchor.'+str(i),'frame',(x0+x1)/2,y0,z,True,.010)
 _panel(b,'frame.head','frame',-w/2,w/2,y0,y1,h,h+f,role='Frame')
 if window: _panel(b,'frame.sill','frame',-w/2,w/2,y0-.008,y1,-f,0,role='Frame')
 else: _panel(b,'frame.threshold','frame',-w/2,w/2,y0+.008,y1,-.018,0,'iron',.001,'Threshold')
 # Front recessed stops sit clear of the rotating leaf's swept envelope.
 for side,sign in [('left',-1),('right',1)]:
  x=sign*w/2
  _panel(b,'frame.stop.'+side,'frame',min(x,x-sign*.009),max(x,x-sign*.009),t/2+.011,t/2+.021,0,h,'rust',.0005,'Stop')
 b.socket('frame.opening','frame','FrameOpening','opening.rect.v1',[0,0,0],{'opening':[w,h,t],'capabilities':['fixed_glazing' if window else 'hinged_leaf'],'max_leaf_count':0 if window else 1,'support_part_ids':['frame.left','frame.right','frame.head']})
 b.scene['metadata']['frame_envelopes']=[{'id':p['id'],'rect':[[p['panel_bounds'][0],p['panel_bounds'][2]],[p['panel_bounds'][1],p['panel_bounds'][2]],[p['panel_bounds'][1],p['panel_bounds'][3]],[p['panel_bounds'][0],p['panel_bounds'][3]]],'z':p['panel_bounds'][4:6]} for p in b.scene['parts'] if p['role']=='Frame']

def _door(b,preset):
 w,h,t=(b.r[k] for k in ('width','height','thickness')); p=.028
 depth=p+t+.012
 gap=max(.012,(2*w-math.sqrt(4*w*w-12*depth*depth))/6+.003)
 lw=w-2*gap; x0=-lw; y0=p; y1=p+t
 pivot=[w/2-gap,-t/2-p,0]
 b.node('door.leaf',pivot,'Leaf',angle=math.radians(b.r['angle']))
 sheet=_panel(b,'door.sheet','door.leaf',x0,0,y0,y1,gap,h-gap,bevel=.0018)
 # Shallow turned edge flanges and narrow welded seams; all are ordinary meshes.
 for name,a,c,z0,z1 in [('left',x0,x0+.018,gap,h-gap),('right',-.018,0,gap,h-gap),('bottom',x0+.018,-.018,gap,gap+.018),('top',x0+.018,-.018,h-gap-.018,h-gap)]:
  _panel(b,'door.fold.'+name,'door.leaf',a,c,y0-.0016,y0+.001,gap if name in ('left','right') else z0,h-gap if name in ('left','right') else z1,'dark_steel',.0006,'FoldedSeam')
 for i,z in enumerate((h*.16,h*.84)):
  prefix='door.hinge.'+str(i)
  _panel(b,prefix+'.leaf_plate','door.leaf',-.065,-.004,y0-.006,y0,z-.049,z+.049,'iron',.001,'HingePlate')
  for k,dz in enumerate((-.030,.030)): b.rivet(prefix+'.leaf_bolt.'+str(k),'door.leaf',-.044,y0-.006,z+dz,True,.007)
  _panel(b,prefix+'.neck','door.leaf',-.014,.006,.008,y0,z-.019,z+.019,'iron',.001,'HingeNeck')
  # Three knuckles share an actual vertical pin axis, anchored in both members.
  for k,(a,c) in enumerate(((-.056,-.020),(-.018,.018),(.020,.056))):
   node='door.leaf' if k==1 else 'frame'; xy=[0,0] if node=='door.leaf' else pivot[:2]
   b.cylinder(prefix+'.knuckle.'+str(k),node,[xy[0],xy[1],z+a],[xy[0],xy[1],z+c],.017,'iron',16,.0009,'HingeKnuckle')
  b.cylinder(prefix+'.pin','frame',[pivot[0],pivot[1],z-.060],[pivot[0],pivot[1],z+.060],.006,'dark_steel',12,.0005,'HingePin')
  _panel(b,prefix+'.frame_plate','frame',pivot[0]+.009,pivot[0]+.071,pivot[1]+.009,pivot[1]+.022,z-.049,z+.049,'iron',.001,'HingePlate')
  for k,dz in enumerate((-.030,.030)): b.rivet(prefix+'.frame_bolt.'+str(k),'frame',pivot[0]+.052,pivot[1]+.009,z+dz,True,.007)
  s=b.socket(prefix+'.socket','door.leaf','Hinge','hinge.pin.v1',[0,0,z],{'footprint':[.065,.098],'pin_diameter':.012,'capabilities':['rotate_z'],'range_degrees':[0,105],'support_part_ids':['door.sheet']})
  s['support_ends']={'leaf':{'node':'door.leaf','part_ids':['door.sheet'],'samples':[[x,y0+.005,z+dz] for x in (-.020,-.044) for dz in (-.029,.029)],'required_depth_m':.005},'frame':{'node':'frame','part_ids':['frame.right'],'samples':[[w/2+.022,-t/2+.006,z+dz] for dz in (-.029,.029)],'required_depth_m':.005}}
 for front,y in [(True,y0),(False,y1)]:
  b.handle('door.handle.'+('front' if front else 'back'),'door.leaf',x0+.12,y,h*.51,front,True,support_id='door.sheet',support_width=.22,lever_direction=1)
  for handle_part in b.scene['parts']:
   if handle_part['id']=='door.handle.'+('front' if front else 'back')+'.plate':handle_part['bevel']=.0024
  socket=b.scene['sockets'][-1]
  socket['support_ends']={'mount':{'node':'door.leaf','part_ids':['door.sheet'],'samples':[[x0+.12,y0+.005 if front else y1-.005,h*.51+dz] for dz in (-.083,0,.083)],'required_depth_m':.005}}
 # Keyway is a recessed dark solid marker, no gameplay locking implementation.
 b.cylinder('door.keyway','door.leaf',[x0+.12,y0-.0065,h*.51-.052],[x0+.12,y0-.0072,h*.51-.052],.005,'grime',12,0,'Keyway')
 b.scene['metadata']['leaf_envelopes']=[{'node':'door.leaf','rect':[[x0,y0-.002],[0,y0-.002],[0,y1+.008],[x0,y1+.008]],'z':[gap,h-gap],'angle_sign':1,'maximum_degrees':105}]
 b.scene['metadata']['leaf_dimensions']={'width':lw,'height':h-2*gap,'thickness':t,'clearance_gap':gap,'hinge_side':'right'}
 if preset in ('industrial_vented','industrial_louvered'): _vented(b,sheet,preset=='industrial_vented')
 if preset=='industrial_louvered': _lock_details(b,x0,y0,w,h,t)

def _border(b,prefix,node,x0,x1,z0,z1,y0,y1,width=.021,role='InsertFrame'):
 for name,a,c,d,e in [('left',x0-width,x0,z0-width,z1+width),('right',x1,x1+width,z0-width,z1+width),('bottom',x0,x1,z0-width,z0),('top',x0,x1,z1,z1+width)]:
  _panel(b,prefix+'.'+name,node,a,c,y0,y1,d,e,'dark_steel',.0009,role)
 for i,(x,z) in enumerate(((x0-width/2,z0-width/2),(x1+width/2,z0-width/2),(x0-width/2,z1+width/2),(x1+width/2,z1+width/2),((x0+x1)/2,z0-width/2),((x0+x1)/2,z1+width/2))):
  b.rivet(prefix+'.rivet.'+str(i),node,x,y0,z,True,.0055)

def _vented(b,sheet,upper_mesh=True):
 from damage_math import carve_panel
 x0,x1,y0,y1,z0,z1=sheet['panel_bounds']; w=x1-x0; h=b.r['height']; cx=(x0+x1)/2
 openings=[('mesh',cx-w*.165,cx+w*.165,h*.655,h*.835),('vent',cx-w*.27,cx+w*.27,h*.13,h*.345)]
 if not upper_mesh:openings=[o for o in openings if o[0]=='vent']
 current=sheet;b.scene['metadata']['design_openings']=[]
 for name,a,c,d,e in openings:
  current=carve_panel(current,[[a,d],[c,d],[c,e],[a,e]],depth=None)
  b.scene['metadata']['design_openings'].append({'id':name,'center':[(a+c)/2,(y0+y1)/2,(d+e)/2],'bounds_xz':[a,c,d,e],'physical_through_hole':True})
  _border(b,'door.'+name+'.rim','door.leaf',a,c,d,e,y0-.012,y0+.003)
  if name=='mesh':
   # Clip constant-slope diagonal wires against the aperture. No image alpha.
   for sign in (-1,1):
    constants=[x-sign*z for x,z in ((a,d),(a,e),(c,d),(c,e))]
    lo,hi=min(constants),max(constants); count=max(2,math.ceil((hi-lo)/.044))
    for i in range(1,count):
     q=lo+(hi-lo)*i/count; points=[]
     for x in (a,c):
      z=(x-q)/sign
      if d-1e-8<=z<=e+1e-8:points.append([x,z])
     for z in (d,e):
      x=q+sign*z
      if a+1e-8<x<c-1e-8:points.append([x,z])
     if len(points)==2:
      u,v=points;b.cylinder('door.mesh.wire.'+str(sign)+'.'+str(i),'door.leaf',[u[0],y0+.012+(sign+1)*.0008,u[1]],[v[0],y0+.012+(sign+1)*.0008,v[1]],.0011,'iron',6,.0001,'WireMesh')
  else:
   pitch=(e-d)/8
   for i in range(8):
    z=d+(i+.5)*pitch
    blade=_panel(b,'door.vent.blade.'+str(i),'door.leaf',a+.001,c-.001,y0+.008,y0+.037,z-pitch*.035,z+pitch*.035,'dark_steel',.0007,'Louver')
    # Folded louver spans one slot vertically while receding into the panel.
    for v in blade['vertices']:
     v[2]+=(v[1]-(y0+.0225))*math.tan(math.radians(42))
    blade['uvs']=_uvs(blade['vertices'],blade['faces'])
    blade.pop('panel_bounds');blade.pop('weather')
 b.scene['parts'][b.scene['parts'].index(sheet)]=current
 b.scene['metadata']['mesh_wire_pitch_m']=.044

def _lock_details(b,x0,y0,w,h,t):
 from damage_math import carve_panel
 z=h*.51
 b.cylinder('door.lock.escutcheon','door.leaf',[x0+.12,y0-.006,z-.065],[x0+.12,y0-.010,z-.065],.014,'iron',16,.001,'LockEscutcheon')
 _panel(b,'door.lock.key_slot','door.leaf',x0+.118,x0+.122,y0-.0104,y0-.0101,z-.072,z-.058,'grime',0,'Keyway')
 px=-w/2-.029;fy=-t/2-.017
 plate=_panel(b,'frame.strike_plate','frame',px-.016,px+.016,fy-.005,fy,z-.070,z+.070,'dark_steel',.0007,'StrikePlate')
 cut=carve_panel(plate,[[px-.005,z-.021],[px+.005,z-.021],[px+.005,z+.021],[px-.005,z+.021]],depth=None)
 b.scene['parts'][b.scene['parts'].index(plate)]=cut
 for i,dz in enumerate((-.052,.052)):b.rivet('frame.strike.screw.'+str(i),'frame',px,fy-.005,z+dz,True,.006)


def _window(b,glazing):
 from damage_math import carve_panel,irregular_polygon
 w,h,t=(b.r[k] for k in ('width','height','thickness'))
 # Move structural steel 12 mm away from the clear pane, leaving a true rebate.
 for part in b.scene['parts']:
  if part['id'].startswith('frame.left') or part['id']=='frame.stop.left':dx=-.012
  elif part['id'].startswith('frame.right') or part['id']=='frame.stop.right':dx=.012
  else:dx=0
  for v in part['vertices']:
   v[0]+=dx
   if part['id']=='frame.head':
    v[2]+=.012;v[0]+=math.copysign(.012,v[0])
   elif part['id']=='frame.sill':
    v[2]-=.012;v[0]+=math.copysign(.012,v[0])
   elif part['id'] in ('frame.left','frame.right'):
    if v[2]<0:v[2]-=.012
    if v[2]>h:v[2]+=.012
  if 'panel_bounds' in part:
   vs=part['vertices'];part['panel_bounds']=[min(v[0] for v in vs),max(v[0] for v in vs),min(v[1] for v in vs),max(v[1] for v in vs),min(v[2] for v in vs),max(v[2] for v in vs)]
   part['weather']['bounds']=part['panel_bounds']
  part['uvs']=_uvs(part['vertices'],part['faces'])
 supports=[]
 for side,a,c in [('front',-.014,-.002),('back',.002,.014)]:
  _border(b,'window.bead.'+side,'frame',-w/2,w/2,0,h,a,c,.026,'GlazingBead')
  supports += ['window.bead.'+side+'.'+v for v in ('left','right','bottom','top')]
 s=b.socket('window.glass.socket','frame','Glass','pane.rect.v1',[0,0,h/2],{'footprint':[w+.022,h+.022],'opening':[w,h],'thickness':.004,'max_thickness':.004,'capabilities':['glazing','fixed_glazing'],'support_part_ids':supports,'support_overlap':.011,'edge_clearance':.001,'glazing_state':glazing},glazing!='empty')
 s['support_ends']={'frame':{'node':'frame','part_ids':supports,'samples':[[-w/2-.010,-.008,h/2],[w/2+.010,-.008,h/2],[0,-.008,-.010],[0,-.008,h+.010]],'required_depth_m':.005}}
 b.scene['metadata'].update(glazing_state=glazing,leaf_envelopes=[],dimensions={'clear_opening':[w,h,t],'structural_opening':[w+.024,h+.024],'glass_size':[w+.022,h+.022,.004],'glass_edge_clearance':.001,'glazing_bearing_m':.011},grasp_policy='Not applicable: fixed single-pane glazing; no moving leaf, hinge or grasp handle.',retained_glass_fraction=1 if glazing=='intact' else 0)
 if glazing=='empty':b.scene['metadata']['missing_parts'].append('window.glass');return
 glass=_panel(b,'window.glass','frame',-w/2-.011,w/2+.011,-.002,.002,-.011,h+.011,'glass',.00012,'GlassPane');glass.pop('weather')
 if glazing=='broken':
  center=[w*.025,h*.51]; polygon=irregular_polygon(center,[w*.48,h*.45],b.r['seed']+782,vertices=11,jaggedness=.70)
  broken=carve_panel(glass,polygon,depth=None);b.scene['parts'][b.scene['parts'].index(glass)]=broken
  area=abs(sum(a[0]*c[1]-c[0]*a[1] for a,c in zip(polygon,polygon[1:]+polygon[:1])))/2
  b.scene['metadata']['glass_hole']={'center_xz':center,'polygon_xz':polygon,'area_m2':area,'construction':'Original 2-D polygon Boolean and 4 mm extruded closed retained glass'}
  b.scene['metadata']['retained_glass_fraction']=1-area/((w+.022)*(h+.022))


def build_industrial(recipe):
 r,preset,glazing,operations=_normalize(recipe); b=Builder(r)
 _frame(b,preset=='industrial_window')
 if preset=='industrial_window': _window(b,glazing)
 else: _door(b,preset)
 b.scene['recipe']=dict(r,preset=preset,glazing=glazing,operations=operations)
 b.scene['metadata'].update(generator='JericoIndustrialPropsLab/industrial.py',preset=preset,authoring_only=True,physics_policy='Geometry, local axes and support references only. Engine chooses joint type, lock, strength, looseness, constraints and damage response.',source_geometry='Original procedural geometry and original damage algorithms; no imported add-on code or assets',damage_operations=[],support_validation='Material sample queries on generated source/cut solids; not a strength simulation.')
 b.scene['metadata']['clearance']=clearance_sweep(b.scene)
 if operations:
  from localized_damage import apply_operations
  return apply_operations(b.scene,operations)
 return b.scene
