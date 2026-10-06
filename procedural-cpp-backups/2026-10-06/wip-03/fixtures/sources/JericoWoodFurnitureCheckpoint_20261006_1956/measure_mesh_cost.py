"""Non-destructive stage counts for the portable export's topology cost."""
import sys,json,collections
from pathlib import Path
ROOT=Path(__file__).resolve().parent;sys.path.insert(0,str(ROOT))
import bpy
from furniture import build_furniture
from blender_adapter import clear_scene,import_asset
from export_assets import _select,_prepare_atlas
result={}
for preset in ['cabinet','workbench']:
 clear_scene();objects=import_asset(build_furniture({'preset':preset}),preset);meshes=[o for o in objects.values() if o.type=='MESH']
 def count():
  by=collections.Counter()
  for obj in meshes:obj.data.calc_loop_triangles();by[obj['role']]+=len(obj.data.loop_triangles)
  return dict(triangles=sum(by.values()),by_role=dict(by))
 source=count()
 for obj in meshes:
  _select([obj])
  for mod in list(obj.modifiers):bpy.ops.object.modifier_apply(modifier=mod.name)
 bevel=count();_prepare_atlas(meshes,2048);portable=count()
 result[preset]=dict(source=source,applied_bevel=bevel,portable_subdivision=portable)
print('MESH_COST '+json.dumps(result),flush=True)
(ROOT/'output/mesh-cost.json').write_text(json.dumps(result,indent=2)+'\n')
