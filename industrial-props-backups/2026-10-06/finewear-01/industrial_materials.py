"""Original localized steel-weather shaders. No texture or code from add-ons."""
import bpy
import json,hashlib

def steel_material(part,side=None):
 from blender_adapter import _material,_noise,_ramp,_node
 key=part['material']; data=part.get('weather',{})
 signature=hashlib.sha256(json.dumps({'weather':data,'wear':part.get('edge_wear',[]),'side':side},sort_keys=True).encode()).hexdigest()[:12]
 m,t,bs=_material('Industrial.'+part['id']+'.'+signature,(.06,.067,.066),.84,.15)
 m['provenance']='Original industrial shader, no add-on assets'
 coord=_node(t,'ShaderNodeTexCoord'); xyz=_node(t,'ShaderNodeSeparateXYZ');t.links.new(coord.outputs['Object'],xyz.inputs[0])
 def mathop(op,a,b=None,name=None):
  n=_node(t,'ShaderNodeMath',operation=op)
  if name:n.name=name
  for i,v in enumerate((a,b)):
   if v is None:continue
   if hasattr(v,'node'): t.links.new(v,n.inputs[i])
   else:n.inputs[i].default_value=v
  return n.outputs[0]
 x0,x1,y0,y1,z0,z1=data.get('bounds',part.get('panel_bounds',[-1,1,-.03,.03,0,2.1]))
 distances=[mathop('SUBTRACT',xyz.outputs['X'],x0),mathop('SUBTRACT',x1,xyz.outputs['X']),mathop('SUBTRACT',xyz.outputs['Z'],z0),mathop('SUBTRACT',z1,xyz.outputs['Z'])]
 edge=distances[0]
 for d in distances[1:]:edge=mathop('MINIMUM',edge,d)
 edge=mathop('SUBTRACT',1,mathop('DIVIDE',edge,data.get('edge_width_m',.013)),name='Analytic edge distance')
 edge=mathop('MAXIMUM',0,mathop('MINIMUM',1,edge))
 bottom=mathop('SUBTRACT',1,mathop('DIVIDE',xyz.outputs['Z'],data.get('bottom_height_m',.57)),name='Localized lower corrosion')
 bottom=mathop('MAXIMUM',0,mathop('MINIMUM',1,bottom))
 macro=_noise(t,8.2,5,.73,(1,1,1.5)); meso=_noise(t,51,3,.7);micro=_noise(t,600,2,.68)
 streak=_noise(t,1,3,.66,(65,55,3.2))
 seed=data.get('seed',29)
 if seed!=29:
  offset=(((seed-29)%997)*.137,((seed-29)%991)*.071,((seed-29)%983)*.053)
  for noise in (macro,meso,micro,streak):
   old=noise.inputs['Vector'].links[0].from_socket
   shifted=_node(t,'ShaderNodeVectorMath',operation='ADD');shifted.name='Seeded local offset';shifted.inputs[1].default_value=offset
   t.links.new(old,shifted.inputs[0]);t.links.new(shifted.outputs[0],noise.inputs['Vector'])
 rustfield=mathop('ADD',macro.outputs['Fac'],mathop('MULTIPLY',bottom,.18))
 rustfield=mathop('ADD',rustfield,mathop('MULTIPLY',edge,.13))
 rustfield=mathop('ADD',rustfield,mathop('MULTIPLY',streak.outputs['Fac'],.04))
 mask=_ramp(t,rustfield,[(.58,(0,0,0)),(.65,(1,1,1))])
 paint=_ramp(t,meso.outputs['Fac'],[(.23,(.010,.012,.012)),(.53,(.021,.025,.024)),(.82,(.039,.045,.042))])
 if data.get('finish','weathered_gray')=='weathered_gray':paint=_ramp(t,meso.outputs['Fac'],[(.23,(.052,.059,.057)),(.53,(.082,.090,.086)),(.82,(.12,.129,.123))])
 if key=='dark_steel':paint=_ramp(t,meso.outputs['Fac'],[(.2,(.009,.011,.012)),(.8,(.032,.036,.034))])
 rust=_ramp(t,meso.outputs['Fac'],[(.16,(.012,.008,.005)),(.47,(.040,.023,.012)),(.67,(.10,.049,.018)),(.86,(.17,.084,.034))])
 mixed=_node(t,'ShaderNodeMixRGB',blend_type='MIX');t.links.new(mask,mixed.inputs[0]);t.links.new(paint,mixed.inputs[1]);t.links.new(rust,mixed.inputs[2])
 # Broken coating reads as tiny ragged bright wear at exposed edges, not a uniform outline.
 wear=mathop('MULTIPLY',edge,mathop('MAXIMUM',0,mathop('SUBTRACT',micro.outputs['Fac'],.42)))
 wear=mathop('MULTIPLY',wear,.42)
 directed=_directed_wear(t,xyz,part,side,mathop)
 if directed is not None:
  m['directed_wear_side']=side
  wear=mathop('MAXIMUM',wear,mathop('MULTIPLY',directed,.84))
 scuffed=_node(t,'ShaderNodeMixRGB',blend_type='MIX');t.links.new(wear,scuffed.inputs[0]);t.links.new(mixed.outputs[0],scuffed.inputs[1]);scuffed.inputs[2].default_value=(.068,.073,.068,1)
 t.links.new(scuffed.outputs[0],bs.inputs['Base Color'])
 rough=_ramp(t,mask,[(0,(.76,.76,.76)),(1,(.96,.96,.96))]);t.links.new(rough,bs.inputs['Roughness'])
 metal=_ramp(t,mask,[(0,(.28,.28,.28)),(1,(.04,.04,.04))]);t.links.new(metal,bs.inputs['Metallic'])
 bump=_node(t,'ShaderNodeBump');bump.inputs['Distance'].default_value=.00045;bump.inputs['Strength'].default_value=.5
 height=mathop('ADD',mathop('MULTIPLY',micro.outputs['Fac'],.60),mathop('MULTIPLY',mask,.4))
 t.links.new(height,bump.inputs['Height']);t.links.new(bump.outputs['Normal'],bs.inputs['Normal'])
 return m

def cut_material(key):
 from blender_adapter import _material,_noise,_node,_ramp
 m,t,bs=_material('Industrial.'+key,(.026,.031,.029) if key=='cut_metal' else (.18,.25,.23),.88,.62 if key=='cut_metal' else 0)
 n=_noise(t,210,3,.72)
 if key=='cut_metal':t.links.new(_ramp(t,n.outputs['Fac'],[(.2,(.012,.016,.017)),(.8,(.051,.058,.054))]),bs.inputs['Base Color'])
 else:bs.inputs['Transmission Weight'].default_value=.65;bs.inputs['IOR'].default_value=1.52
 bump=_node(t,'ShaderNodeBump');bump.inputs['Distance'].default_value=.0003;bump.inputs['Strength'].default_value=.3;t.links.new(n.outputs['Fac'],bump.inputs['Height']);t.links.new(bump.outputs['Normal'],bs.inputs['Normal'])
 return m


def _directed_wear(tree,xyz,part,side,op):
 """Shader evaluation of the same exact-side, region-clipped CPU edge mask."""
 selected=[c for c in part.get('edge_wear',[]) if c['side']==side]
 if not selected:return None
 if any(c.get('fine_breakup') is True for c in selected):
  from fine_wear_adapter import shared_math
  from fine_wear_math import contour_segments
  op=shared_math(op)
  selected=[dict(c,boundary_segments_xz=contour_segments(c)) if c.get('fine_breakup') is True else c for c in selected]
 def clamp(v):return op('MINIMUM',1,op('MAXIMUM',0,v))
 def smooth(v):
  u=clamp(v);u2=op('MULTIPLY',u,u);u3=op('MULTIPLY',u2,u)
  polynomial=op('ADD',op('MULTIPLY',u,op('SUBTRACT',op('MULTIPLY',6,u),15)),10)
  return op('MULTIPLY',u3,polynomial)
 def region_distance(region):
  if region['shape']=='box':
   values=[op('SUBTRACT',xyz.outputs[i],region['min'][i]) for i in range(3)]+[op('SUBTRACT',region['max'][i],xyz.outputs[i]) for i in range(3)]
   d=values[0]
   for v in values[1:]:d=op('MINIMUM',d,v)
   return d
  delta=[op('SUBTRACT',xyz.outputs[i],region['center'][i]) for i in range(3)]
  squared=[op('MULTIPLY',v,v) for v in delta]
  length=op('SQRT',op('ADD',op('ADD',squared[0],squared[1]),squared[2]))
  return op('SUBTRACT',region['radius'],length)
 def segment_distance(a,b):
  dx,dz=b[0]-a[0],b[1]-a[1];den=dx*dx+dz*dz
  if den<=1e-20:return None
  vx=op('SUBTRACT',xyz.outputs['X'],a[0]);vz=op('SUBTRACT',xyz.outputs['Z'],a[1])
  along=clamp(op('DIVIDE',op('ADD',op('MULTIPLY',vx,dx),op('MULTIPLY',vz,dz)),den))
  rx=op('SUBTRACT',vx,op('MULTIPLY',along,dx));rz=op('SUBTRACT',vz,op('MULTIPLY',along,dz))
  return op('SQRT',op('ADD',op('MULTIPLY',rx,rx),op('MULTIPLY',rz,rz)))
 result=0
 for config in selected:
  distances=[segment_distance(a,b) for a,b in config['boundary_segments_xz']]
  distances=[d for d in distances if d is not None]
  if not distances:continue
  distance=distances[0]
  for d in distances[1:]:distance=op('MINIMUM',distance,d)
  edge=op('SUBTRACT',1,smooth(op('DIVIDE',distance,config.get('width_m',.015))))
  phi=region_distance(config['region']);feather=config.get('feather_m',0)
  mask=smooth(op('DIVIDE',phi,feather)) if feather else op('SUBTRACT',1,op('LESS_THAN',phi,0))
  if not config.get('override_protection',False):
   for protected in config.get('protected_regions',[]):mask=op('MULTIPLY',mask,op('LESS_THAN',region_distance(protected),0))
  effect=op('MULTIPLY',op('MULTIPLY',edge,mask),config.get('amount',1))
  result=op('MAXIMUM',result,effect,name='Directed edge wear ('+str(side)+')')
 return result


def directed_hardware_material(part,side,source):
 """Add local scuffing to a copy; preserve the source shader outside the mask."""
 from blender_adapter import _node
 signature=hashlib.sha256(json.dumps({'wear':part.get('edge_wear',[]),'side':side},sort_keys=True).encode()).hexdigest()[:12]
 m=source.copy();m.name='Industrial.hardware.'+part['id']+'.'+signature
 t=m.node_tree;bs=next(n for n in t.nodes if n.type=='BSDF_PRINCIPLED')
 coord=_node(t,'ShaderNodeTexCoord');xyz=_node(t,'ShaderNodeSeparateXYZ');t.links.new(coord.outputs['Object'],xyz.inputs[0])
 def op(kind,a,b=None,name=None):
  n=_node(t,'ShaderNodeMath',operation=kind)
  if name:n.name=name
  for i,v in enumerate((a,b)):
   if v is None:continue
   if hasattr(v,'node'):t.links.new(v,n.inputs[i])
   else:n.inputs[i].default_value=v
  return n.outputs[0]
 mask=_directed_wear(t,xyz,part,side,op)
 if mask is None:return source
 m['directed_wear_side']=side;m['provenance']='Original local hardware scuff shader; source shader retained outside selected region'
 original=bs.inputs['Base Color'];mix=_node(t,'ShaderNodeMixRGB',blend_type='MIX')
 if original.is_linked:t.links.new(original.links[0].from_socket,mix.inputs[1])
 else:mix.inputs[1].default_value=original.default_value
 mix.inputs[2].default_value=(.095,.105,.099,1);t.links.new(mask,mix.inputs[0]);t.links.new(mix.outputs[0],original)
 return m
