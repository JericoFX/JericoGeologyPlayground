"""Read-only Blender consumer check. Never saves, exports, bakes or repairs.
blender -b --factory-startup --python this.py -- asset.obj optional-report.json
"""
import bpy
import json
import math
from pathlib import Path
import shlex
import sys

args=sys.argv[sys.argv.index('--')+1:]
source=Path(args[0]).resolve()
positions=[]; faces=0; object_names=[]
for line in source.read_text().splitlines():
    tokens=shlex.split(line,comments=True)
    if not tokens: continue
    if tokens[0]=='v': positions.append(tuple(float(x) for x in tokens[1:]))
    elif tokens[0]=='f': faces+=1
    elif tokens[0]=='o': object_names.append(tokens[1])
assert positions and faces and object_names
bpy.ops.wm.read_factory_settings(use_empty=True)
# Blender labels its viewing-forward direction +Y; Y/Z is its identity
# axis conversion. Asset fronts remain -Y and vertices remain unchanged.
# https://docs.blender.org/manual/en/4.3/files/import_export/obj.html
# Importer validation/repair is disabled.
result=bpy.ops.wm.obj_import(filepath=str(source),forward_axis='Y',up_axis='Z',use_split_objects=True,use_split_groups=False,validate_meshes=False)
assert result=={'FINISHED'},result
meshes=[o for o in bpy.data.objects if o.type=='MESH']
assert len(meshes)==len(object_names),(len(meshes),len(object_names))
assert sum(len(o.data.polygons) for o in meshes)==faces
actual_points=[]
for obj in meshes:
    assert len(obj.data.uv_layers)>=1,(obj.name,'missing atlas UV')
    assert obj.data.materials,(obj.name,'missing material')
    assert all(len(p.vertices)==3 for p in obj.data.polygons),(obj.name,'non-triangle')
    assert all(math.isfinite(x) for v in obj.data.vertices for x in v.co)
    assert all(math.isfinite(x) and 0<=x<=1 for loop in obj.data.uv_layers.active.data for x in loop.uv)
    actual_points.extend(tuple(obj.matrix_world@v.co) for v in obj.data.vertices)
expected_bounds=[[min(v[i] for v in positions) for i in range(3)],[max(v[i] for v in positions) for i in range(3)]]
actual_bounds=[[min(v[i] for v in actual_points) for i in range(3)],[max(v[i] for v in actual_points) for i in range(3)]]
assert all(abs(a-b)<2e-6*max(1,abs(b)) for row,expected in zip(actual_bounds,expected_bounds) for a,b in zip(row,expected)),(actual_bounds,expected_bounds)
material_info=[]
for mat in bpy.data.materials:
    if not mat.use_nodes: continue
    shaders=[n for n in mat.node_tree.nodes if n.type=='BSDF_PRINCIPLED']
    images=[n for n in mat.node_tree.nodes if n.type=='TEX_IMAGE' and n.image]
    info={'name':mat.name,'image_nodes':len(images),'image_paths':[bpy.path.abspath(n.image.filepath) for n in images]}
    for image_node in images:
        assert Path(bpy.path.abspath(image_node.image.filepath)).is_file(),image_node.image.filepath
        assert image_node.image.size[0]>0 and image_node.image.size[1]>0,image_node.image.filepath
    for shader in shaders:
        info['base_color_linked']=shader.inputs['Base Color'].is_linked
        info['normal_linked']=shader.inputs['Normal'].is_linked
        info['roughness_linked']=shader.inputs['Roughness'].is_linked
        info['metallic_linked']=shader.inputs['Metallic'].is_linked
        info['roughness_default']=shader.inputs['Roughness'].default_value
        info['metallic_default']=shader.inputs['Metallic'].default_value
        info['ior']=shader.inputs['IOR'].default_value
        info['alpha']=shader.inputs['Alpha'].default_value
        info['transmission_weight']=shader.inputs['Transmission Weight'].default_value
    material_info.append(info)
limitations=[]
if any(not m.get('normal_linked',False) for m in material_info):
    limitations.append('Blender 4.3.2 OBJ importer ignores native MTL norm statements; the original PNG16 normal maps remain in the bundle but are not connected by this consumer.')
report={'status':'PASS_GEOMETRY_WITH_MATERIAL_LIMITATIONS' if limitations else 'PASS','limitations':limitations,'blender_axis_options':{'forward_axis':'Y','up_axis':'Z'},'consumer':'Blender '+bpy.app.version_string,'input':str(source),'meshes':len(meshes),'triangles':faces,'materials':len(bpy.data.materials),'images':len(bpy.data.images),'normal_map_nodes':sum(n.type=='NORMAL_MAP' for m in bpy.data.materials if m.use_nodes for n in m.node_tree.nodes),'custom_normal_meshes':sum(o.data.has_custom_normals for o in meshes),'world_bounds':actual_bounds,'source_axis_convention':'Z-up; -Y forward; meters','importer_mesh_validation_enabled':False,'no_asset_generation_save_or_repair':True,'material_import':material_info}
print('READONLY_OBJ_IMPORT '+json.dumps(report))
if len(args)>1: Path(args[1]).write_text(json.dumps(report,indent=2)+'\n')
