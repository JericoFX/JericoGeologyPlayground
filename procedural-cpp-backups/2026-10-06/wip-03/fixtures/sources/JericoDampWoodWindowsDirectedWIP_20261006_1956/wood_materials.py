"""Fine anisotropic wood, physical grain bump and localized baked moisture."""
from blender_adapter import _material,_node,_noise,_ramp

def wood_material(part,exposed=False):
 finish=part['wood_finish'];age=finish['age'];wet=finish['wetness'];axis=finish['grain_axis']
 name='DampWood.'+finish['family']+('.exposed.' if exposed else '.surface.')+axis+'.a%.3f.w%.3f'%(age,wet)
 reference=.58+age*.29-wet*.35
 m,t,bs=_material(name,(.08,.04,.02),reference,0)
 m['wetness']=wet;m['age']=age;m['roughness_reference']=reference;m['portable_channels']='basecolor, roughness, tangent normal'
 fine=_noise(t,1,4,.69,(5,460,460) if axis=='X' else (460,460,5));fine.name='Fine lengthwise grain'
 grain=_noise(t,1,3,.63,(2.3,140,140) if axis=='X' else (140,140,2.3))
 mixed=_node(t,'ShaderNodeMixRGB',blend_type='MULTIPLY');mixed.inputs[0].default_value=.28
 t.links.new(grain.outputs['Fac'],mixed.inputs[1]);t.links.new(fine.outputs['Fac'],mixed.inputs[2])
 fresh=[(.026,.012,.004),(.082,.038,.012),(.17,.093,.038),(.25,.148,.066)]
 aged=[(.013,.011,.0075),(.040,.031,.020),(.082,.065,.041),(.135,.109,.072)]
 colors=[tuple((1-age)*a+age*b for a,b in zip(f,n)) for f,n in zip(fresh,aged)]
 if exposed:colors=[tuple(v*1.10 for v in col) for col in colors]
 color=_ramp(t,mixed.outputs['Color'],list(zip((.22,.38,.52,.70),colors)))
 coord=_node(t,'ShaderNodeTexCoord');xyz=_node(t,'ShaderNodeSeparateXYZ');t.links.new(coord.outputs['Object'],xyz.inputs[0])
 def height_mask(low,high,start,end):
  n=_node(t,'ShaderNodeMapRange');n.clamp=True;n.inputs['From Min'].default_value=low;n.inputs['From Max'].default_value=high
  n.inputs['To Min'].default_value=start;n.inputs['To Max'].default_value=end;t.links.new(xyz.outputs['Z'],n.inputs['Value']);return n.outputs['Result']
 bottom=height_mask(-.12,.40,1,0);top=height_mask(finish['height_m']-.18,finish['height_m']+.04,0,.62)
 joint=_node(t,'ShaderNodeMath',operation='MAXIMUM');joint.name='Wet joint accumulation';t.links.new(bottom,joint.inputs[0]);t.links.new(top,joint.inputs[1])
 patch=_noise(t,6.8,3,.64,(1.4,1,.35));patch.name='Irregular seepage patches'
 patchlevel=_node(t,'ShaderNodeMapRange');patchlevel.clamp=True;patchlevel.inputs['From Min'].default_value=.2;patchlevel.inputs['From Max'].default_value=.8
 patchlevel.inputs['To Min'].default_value=.42;patchlevel.inputs['To Max'].default_value=1;t.links.new(patch.outputs['Fac'],patchlevel.inputs['Value'])
 mask=_node(t,'ShaderNodeMath',operation='MULTIPLY');t.links.new(joint.outputs[0],mask.inputs[0]);t.links.new(patchlevel.outputs[0],mask.inputs[1])
 strength=_node(t,'ShaderNodeMath',operation='MULTIPLY');strength.inputs[1].default_value=wet*.65;t.links.new(mask.outputs[0],strength.inputs[0])
 dark=_node(t,'ShaderNodeMath',operation='SUBTRACT');dark.inputs[0].default_value=1-wet*.20;t.links.new(strength.outputs[0],dark.inputs[1])
 stain=_node(t,'ShaderNodeMixRGB',blend_type='MULTIPLY');stain.inputs[0].default_value=1
 t.links.new(color,stain.inputs[1]);t.links.new(dark.outputs[0],stain.inputs[2]);t.links.new(stain.outputs[0],bs.inputs['Base Color'])
 rough=_ramp(t,patch.outputs['Fac'],[(.2,(max(.18,reference-.065),)*3),(.8,(min(.94,reference+.08),)*3)])
 wetrough=_node(t,'ShaderNodeMath',operation='MULTIPLY');wetrough.inputs[1].default_value=wet*(.13+age*.07);t.links.new(mask.outputs[0],wetrough.inputs[0])
 finalrough=_node(t,'ShaderNodeMath',operation='SUBTRACT');t.links.new(rough,finalrough.inputs[0]);t.links.new(wetrough.outputs[0],finalrough.inputs[1]);t.links.new(finalrough.outputs[0],bs.inputs['Roughness'])
 bump=_node(t,'ShaderNodeBump');bump.inputs['Strength'].default_value=.30+age*.22;bump.inputs['Distance'].default_value=.00032+age*.00072
 t.links.new(fine.outputs['Fac'],bump.inputs['Height']);t.links.new(bump.outputs['Normal'],bs.inputs['Normal'])
 return m
