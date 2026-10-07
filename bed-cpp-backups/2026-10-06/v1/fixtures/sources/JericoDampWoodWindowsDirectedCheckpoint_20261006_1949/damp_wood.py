"""Damp wooden fixed windows, composed from the proven single-pane generator.

Age controls bounded mesh loss and timber weathering. Wetness controls the
portable finish only. Geometry, local supports and UVs remain engine-neutral.
"""
from __future__ import annotations
import copy, math, random
from core import Builder
from singlepane import build_singlepane
from glass_fracture import build_fracture
from damage_math import carve_panel, point_in_panel
from crack_math import crack_profiles

FAMILIES=('old','new')

def _amount(value,name):
 if isinstance(value,bool) or not isinstance(value,(int,float)) or not math.isfinite(value) or not 0<=value<=1:
  raise ValueError(name+' must be finite in 0..1')
 return float(value)

def _wood_member(part,recipe,age,index):
 bounds=[value for axis in range(3) for value in (min(v[axis] for v in part['vertices']),max(v[axis] for v in part['vertices']))]
 x0,x1,y0,y1,z0,z1=bounds; horizontal=x1-x0>z1-z0
 b=Builder(recipe);current=b.box(part['id'],part['node'],x0,x1,y0,y1,z0,z1,'damp_wood',.0006,part['role'])
 initial=(x1-x0)*(y1-y0)*(z1-z0)
 current['panel_bounds']=bounds
 rng=random.Random(recipe['seed']+9071+index*997)
 # Small handling nicks on new timber; genuinely missing edge wood on old.
 # Damage remains on the exposed outer perimeter, away from glazing contact.
 load={'frame.left':1.0,'frame.right':.52,'frame.head':.40,'frame.sill':1.15}[part['id']]
 depth=.0012+(.030*load if age>.35 else .010)*age
 extent=(.010+(.080*load if age>.35 else .030)*age)*(1+rng.uniform(-.17,.17))
 if horizontal:
  edge=z0 if part['id']=='frame.sill' else z1; sign=1 if part['id']=='frame.sill' else -1
  along=x0+(x1-x0)*(.23 if part['id']=='frame.sill' and age>.35 else .22+.09*index)
  profile=[[along-extent,edge-sign*.004],[along+extent*.6,edge-sign*.004],
           [along+extent*.41,edge+sign*depth*.55],[along+extent*.07,edge+sign*depth],
           [along-extent*.36,edge+sign*depth*.43]]
 else:
  edge=x0 if part['id']=='frame.left' else x1;sign=1 if part['id']=='frame.left' else -1
  along=z0+(min(.095,(z1-z0)*.12) if part['id']=='frame.left' and age>.35 else (z1-z0)*(.27+.16*index))
  profile=[[edge-sign*.004,along-extent],[edge+sign*depth*.44,along-extent*.63],
           [edge+sign*depth,along+extent*.14],[edge+sign*depth*.35,along+extent*.72],
           [edge-sign*.004,along+extent]]
 current=carve_panel(current,profile,depth=None if age>.35 else .0022)
 # Checks follow the dominant timber grain; no transverse metal-like tears.
 if age>.35 or part['id']=='frame.right':
  center=[(x0+x1)/2,(z0+z1)/2]
  if not horizontal:center[0]+=(x1-x0)*(.13 if index==0 else -.13);center[1]=z0+(z1-z0)*.61
  else:center[0]=x0+(x1-x0)*.62;center[1]=(z0+z1)/2
  length=(x1-x0 if horizontal else z1-z0)*(.045+.33*age)
  profiles=crack_profiles(center,length,.0012+.0023*age,.0012+.007*age,recipe['seed']+index*31,
                          direction_xz=(1,0) if horizontal else (0,1),branches=0,segments=3)
  # Reuse the primary variable-width check. It is a shallow physical cavity.
  current=carve_panel(current,profiles[0]['polygon_xz'],depth=profiles[0]['depth'])
  current['wood_check']={'grain_axis':'X' if horizontal else 'Z','centerline_xz':profiles[0]['centerline_xz'],
                         'length_m':length,'depth_m':profiles[0]['depth']}
 if age>.35 and part['id']=='frame.sill':
  second=crack_profiles([x0+(x1-x0)*.33,z0+(z1-z0)*.63],(x1-x0)*.15,.0014,.0038,recipe['seed']+771,direction_xz=(1,0),branches=0,segments=3)[0]
  current=carve_panel(current,second['polygon_xz'],depth=second['depth'])
 current['face_materials']=['wood_exposed' if m=='cut_metal' else m for m in current['face_materials']]
 current['wood_damage']={'age':age,'initial_volume_m3':initial,
                         'removed_volume_m3':initial-current['cut_math']['remaining_volume'],
                         'policy':'Localized outer-edge loss and shallow checks parallel to grain; no structural fracture or strength model'}
 return current

def build_damp_window(recipe):
 if not isinstance(recipe,dict):raise TypeError('Recipe must be an object')
 local=copy.deepcopy(recipe);family=local.pop('family','old')
 if family not in FAMILIES:raise ValueError('family must be old or new')
 age=_amount(local.pop('age',.86 if family=='old' else .08),'age')
 wetness=_amount(local.pop('wetness',.82),'wetness')
 local.setdefault('seed',29);local.setdefault('width',1.12);local.setdefault('height',1.28);local.setdefault('thickness',.06)
 local.setdefault('glazing','intact');local['preset']='single_pane_fixed'
 scene=build_singlepane(local);r=scene['recipe'];glazing=r['glazing']
 # Use the immutable approved glass implementation, not a second fracture path.
 glass=build_fracture(dict(preset='industrial_window',kind='window',width=r['width'],height=r['height'],
                          thickness=r['thickness'],glazing=glazing,seed=r['seed'],energy=.66,impact=[.15,.56],ray_count=18))
 scene['parts']=[p for p in scene['parts'] if p['role']!='GlassPane']+[copy.deepcopy(p) for p in glass['parts'] if p['role']=='GlassPane']
 for key in ('glass_hole','glass_fracture','retained_glass_fraction'):
  if key in glass['metadata']:scene['metadata'][key]=copy.deepcopy(glass['metadata'][key])
 samples=[];member_index=0
 for i,p in enumerate(scene['parts']):
  if p['role']=='Frame':
   p=_wood_member(p,r,age,member_index);member_index+=1;scene['parts'][i]=p
   x0,x1,y0,y1,z0,z1=p['panel_bounds'];horizontal=x1-x0>z1-z0
   for f in (.20,.50,.80):
    if horizontal:point=[x0+(x1-x0)*f,y0+.022,z1-.004 if p['id']=='frame.sill' else z0+.004]
    else:point=[x1-.004 if p['id']=='frame.left' else x0+.004,y0+.022,z0+(z1-z0)*f]
    if not point_in_panel(p,point):raise ValueError('Wood damage compromised protected supporting core')
    samples.append({'part_id':p['id'],'point':point})
  if p['material'] in ('wood','wood_rough','damp_wood'):
   p['material']='damp_wood'
   if 'face_materials' in p:p['face_materials']=['damp_wood' if v in ('wood','wood_rough') else v for v in p['face_materials']]
   span=[max(v[k] for v in p['vertices'])-min(v[k] for v in p['vertices']) for k in range(3)]
   p['wood_finish']={'family':family,'age':age,'wetness':wetness,'grain_axis':'X' if span[0]>span[2] else 'Z',
                     'height_m':r['height'],'seed':r['seed']}
  # The historic grime geometry is a seam accent, reduced on newer timber.
  if p['role']=='DirtSeam' and age<.35:p['material']='damp_wood';p['wood_finish']={'family':family,'age':age,'wetness':wetness,'grain_axis':'Z','height_m':r['height'],'seed':r['seed']}
 scene['recipe'].update(family=family,age=age,wetness=wetness,preset='damp_wood_single_pane')
 scene['metadata'].update(generator='JericoDampWoodWindowsLab/damp_wood.py',preset='damp_wood_single_pane',
  wood_finish={'family':family,'age':age,'wetness':wetness},wood_support_samples=samples,
  art_direction='Common wet timber, newer with restrained handling wear; old timber has desaturated grain, localized edge splinters and real lengthwise checks.',
  damage_policy='Age controls real local edge loss and shallow grain-aligned checks independently of wetness. All six states reuse the same supporting frame per family.',
  physics_policy='Authoring geometry and support references only. Engine owns joints, strength, collision and all interaction mechanics.',
  authoring_only=True,simulation=False,
  approved_glass_snapshot='source-snapshot-3c6da6d852c6',
  finish_portability='Wet color darkening and roughness variation bake into PBR maps. No runtime-only moisture shader, droplets, or animation is required.')
 return scene
