"""Read-only portable GLB preview. Never repairs or regenerates asset data."""
import sys,argparse,json
from pathlib import Path
import bpy
from mathutils import Vector
ROOT=Path(__file__).resolve().parents[1]
sys.path.insert(0,str(ROOT/'tools'))
from render_compare import stage
p=argparse.ArgumentParser();p.add_argument('--glb',required=True);p.add_argument('--out',required=True);p.add_argument('--samples',type=int,default=32);p.add_argument('--resolution',type=int,default=640);a=p.parse_args(sys.argv[sys.argv.index('--')+1:])
bpy.ops.object.select_all(action='SELECT');bpy.ops.object.delete(use_global=False)
bpy.ops.import_scene.gltf(filepath=str(Path(a.glb).resolve()))
objects=list(bpy.context.scene.objects)
for ob in objects:
 if ob.parent is None:ob.location+=Vector((0,0,.20))
for ob in objects:
 assert len(ob.modifiers)==0,'Read-only GLB preview must not add modifiers'
s=stage(Path(a.out),a.resolution,a.samples);bpy.ops.render.render(write_still=True)
Path(a.out).with_suffix('.import.json').write_text(json.dumps(dict(glb=a.glb,blender=bpy.app.version_string,read_only_import=True,asset_modifiers=0,meshes=sum(o.type=='MESH' for o in objects),vertices=sum(len(o.data.vertices) for o in objects if o.type=='MESH')),indent=2))
