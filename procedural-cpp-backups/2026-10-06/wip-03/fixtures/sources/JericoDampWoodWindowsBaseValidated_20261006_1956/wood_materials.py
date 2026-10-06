"""Original anisotropic damp-timber authoring shader, baked by export_assets.

Only standard Principled basecolor, roughness and tangent normals are needed.
No coat, screen-space tricks or engine-specific wetness nodes are required.
"""
from blender_adapter import _material,_node,_noise,_ramp

def wood_material(part,exposed=False):
 finish=part['wood_finish'];age=finish['age'];wet=finish['wetness'];axis=finish['grain_axis']
 name='DampWood.'+finish['family']+('.exposed.' if exposed else '.surface.')+axis+'.a%.3f.w%.3f'%(age,wet)
 reference=.58+age*.29-wet*.35
 m,t,bs=_material(name,(.08,.04,.02),reference,0)
 m['wetness']=wet;m['age']=age;m['roughness_reference']=reference;m['portable_channels']='basecolor, roughness, tangent normal'
 fine=_noise(t,1,4,.69,(5,210,210) if axis=='X' else (210,210,5))
 coarse=_noise(t,1,3,.62,(1.2,18,18) if axis=='X' else (18,18,1.2))
 mixed=_node(t,'ShaderNodeMixRGB',blend_type='MULTIPLY');mixed.inputs[0].default_value=.42
 t.links.new(coarse.outputs['Fac'],mixed.inputs[1]);t.links.new(fine.outputs['Fac'],mixed.inputs[2])
 fresh=[(.032,.014,.0055),(.095,.044,.016),(.20,.113,.051),(.31,.194,.096)]
 aged=[(.013,.011,.008),(.044,.035,.024),(.108,.088,.060),(.20,.163,.111)]
 colors=[tuple((1-age)*a+age*b for a,b in zip(f,n)) for f,n in zip(fresh,aged)]
 if exposed:colors=[tuple(v*1.13 for v in col) for col in colors]
 color=_ramp(t,mixed.outputs['Color'],list(zip((.20,.34,.47,.65),colors)))
 # A broad irregular wet band rises from the sill, with additional broad patches.
 coord=_node(t,'ShaderNodeTexCoord');xyz=_node(t,'ShaderNodeSeparateXYZ');t.links.new(coord.outputs['Object'],xyz.inputs[0])
 height=_node(t,'ShaderNodeMapRange');height.clamp=True
 height.inputs['From Min'].default_value=-.12;height.inputs['From Max'].default_value=finish['height_m']*.42
 height.inputs['To Min'].default_value=1-wet*.52;height.inputs['To Max'].default_value=1-wet*.26
 t.links.new(xyz.outputs['Z'],height.inputs['Value'])
 stain=_node(t,'ShaderNodeMixRGB',blend_type='MULTIPLY');stain.inputs[0].default_value=1
 t.links.new(color,stain.inputs[1]);t.links.new(height.outputs['Result'],stain.inputs[2]);t.links.new(stain.outputs[0],bs.inputs['Base Color'])
 patch=_noise(t,3.4,3,.63,(1,1,.34))
 rough=_ramp(t,patch.outputs['Fac'],[(.23,(max(.15,reference-.10),)*3),(.73,(min(.94,reference+.13),)*3)])
 t.links.new(rough,bs.inputs['Roughness'])
 bump=_node(t,'ShaderNodeBump');bump.inputs['Strength'].default_value=.22+age*.23
 bump.inputs['Distance'].default_value=.0003+age*.00065
 t.links.new(fine.outputs['Fac'],bump.inputs['Height']);t.links.new(bump.outputs['Normal'],bs.inputs['Normal'])
 return m
