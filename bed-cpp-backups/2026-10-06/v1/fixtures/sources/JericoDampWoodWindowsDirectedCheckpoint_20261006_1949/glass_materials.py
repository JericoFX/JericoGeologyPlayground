"""Original transparent dusty glass; metric rebate mask and analytic scratches."""
import math, random
from blender_adapter import _material,_node,_noise,_ramp

def glass_material(part,edge=False):
 key='Fracture.glass.edge' if edge else 'Fracture.glass.surface'
 m,t,bs=_material(key,(.30,.40,.35),.12)
 bs.inputs['Transmission Weight'].default_value=.96;bs.inputs['IOR'].default_value=1.52
 bs.inputs['Coat Weight'].default_value=.12
 if edge:
  bs.inputs['Base Color'].default_value=(.48,.63,.55,1)
  bs.inputs['Roughness'].default_value=.16;bs.inputs['Transmission Weight'].default_value=.58
  return m
 tex=_node(t,'ShaderNodeTexCoord');xyz=_node(t,'ShaderNodeSeparateXYZ');t.links.new(tex.outputs['Object'],xyz.inputs[0])
 def op(kind,a,b=0,name=None):
  n=_node(t,'ShaderNodeMath',operation=kind)
  if name:n.name=name
  for i,v in enumerate((a,b)):
   if hasattr(v,'node'):t.links.new(v,n.inputs[i])
   else:n.inputs[i].default_value=v
  return n.outputs[0]
 def clamp(v):return op('MINIMUM',1,op('MAXIMUM',0,v))
 x,z=xyz.outputs['X'],xyz.outputs['Z'];x0,x1,z0,z1=part.get('glass_bounds',[-.591,.591,-.011,1.261])
 distances=[op('SUBTRACT',x,x0),op('SUBTRACT',x1,x),op('SUBTRACT',z,z0),op('SUBTRACT',z1,z)]
 edge_dist=distances[0]
 for d in distances[1:]:edge_dist=op('MINIMUM',edge_dist,d)
 dust=clamp(op('SUBTRACT',1,op('DIVIDE',edge_dist,.085),name='Rebate dust accumulation'))
 streak=_noise(t,1,3,.60,(47,3,2.2));dust=op('MULTIPLY',dust,op('ADD',.38,op('MULTIPLY',streak.outputs['Fac'],.6)))
 bottom=clamp(op('SUBTRACT',1,op('DIVIDE',op('SUBTRACT',z,z0),.19)))
 dust=op('MAXIMUM',dust,op('MULTIPLY',bottom,op('MULTIPLY',streak.outputs['Fac'],.55)))
 # Sparse metric line segments with an analytic distance field: actual scratch
 # direction/length, not uniformly multiplied noise or a painted crack image.
 scratch=0;rng=random.Random(203)
 for i in range(11):
  ax=rng.uniform(x0+.04,x1-.04);az=rng.uniform(z0+.04,z1-.04)
  dx=rng.uniform(-.025,.025);dz=rng.uniform(.015,.075);den=dx*dx+dz*dz
  vx,vz=op('SUBTRACT',x,ax),op('SUBTRACT',z,az)
  f=clamp(op('DIVIDE',op('ADD',op('MULTIPLY',vx,dx),op('MULTIPLY',vz,dz)),den))
  xx,zz=op('SUBTRACT',vx,op('MULTIPLY',f,dx)),op('SUBTRACT',vz,op('MULTIPLY',f,dz))
  distance=op('SQRT',op('ADD',op('MULTIPLY',xx,xx),op('MULTIPLY',zz,zz)))
  scratch=op('MAXIMUM',scratch,clamp(op('SUBTRACT',1,op('DIVIDE',distance,.00024))))
 scratch=op('MULTIPLY',scratch,.5,name='Analytic scratch wear')
 rough=op('ADD',.095,op('ADD',op('MULTIPLY',dust,.30),scratch));t.links.new(rough,bs.inputs['Roughness'])
 color=_node(t,'ShaderNodeMixRGB');t.links.new(dust,color.inputs[0]);color.inputs[1].default_value=(.30,.40,.35,1);color.inputs[2].default_value=(.31,.29,.22,1);t.links.new(color.outputs[0],bs.inputs['Base Color'])
 # Dust has texture only where dirt actually accumulates. Clean glass remains
 # optically smooth, with no hammered/noisy displacement across the whole pane.
 micro=_noise(t,900,2,.6);bump=_node(t,'ShaderNodeBump');bump.inputs['Distance'].default_value=.000025;bump.inputs['Strength'].default_value=.18
 t.links.new(op('MULTIPLY',micro.outputs['Fac'],dust),bump.inputs['Height']);t.links.new(bump.outputs[0],bs.inputs['Normal'])
 m['provenance']='Original metric dust and analytic scratch shader; physical gaps are separate mesh geometry'
 return m
