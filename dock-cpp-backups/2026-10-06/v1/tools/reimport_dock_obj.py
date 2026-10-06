"""Read-only Blender OBJ/MTL portability check; no geometry or material repair."""
import bpy,sys,argparse,json,math
from pathlib import Path
p=argparse.ArgumentParser();p.add_argument('--obj',required=True);p.add_argument('--scene',required=True);p.add_argument('--out',required=True);a=p.parse_args(sys.argv[sys.argv.index('--')+1:])
bpy.ops.object.select_all(action='SELECT');bpy.ops.object.delete(use_global=False)
bpy.ops.wm.obj_import(filepath=str(Path(a.obj).resolve()),forward_axis='Y',up_axis='Z')
source=json.loads(Path(a.scene).read_text());meshes=[o for o in bpy.context.scene.objects if o.type=='MESH'];expected=sum(sum(len(f)-2 for f in p['faces']) for p in source['parts'])
assert len(meshes)==len(source['parts'])
triangles=0
for o in meshes:
 assert not o.modifiers
 o.data.calc_loop_triangles();triangles+=len(o.data.loop_triangles)
 assert o.data.uv_layers.active
 for v in o.data.vertices:assert all(math.isfinite(c) for c in v.co)
 for n in o.data.corner_normals:assert all(math.isfinite(c) for c in n.vector)
 for uv in o.data.uv_layers.active.data:assert all(math.isfinite(c) for c in uv.uv)
assert triangles==expected
report=dict(read_only=True,blender=bpy.app.version_string,mesh_parts=len(meshes),triangles=triangles,expected_triangles=expected,finite_normals_uvs=True,modifiers=0,limitation='Blender OBJ importer ignores norm; exported normal PNG retained. GLB is preferred PBR route.')
Path(a.out).write_text(json.dumps(report,indent=2));print(json.dumps(report))
