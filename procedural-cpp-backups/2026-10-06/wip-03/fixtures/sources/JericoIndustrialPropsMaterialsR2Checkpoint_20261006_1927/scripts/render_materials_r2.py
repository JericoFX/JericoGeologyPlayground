"""Evidence only: render an actual old or R2 GLB under identical staging."""
import argparse, hashlib, json, sys
from pathlib import Path
ROOT=Path(__file__).resolve().parents[1];sys.path.insert(0,str(ROOT))
import bpy
from mathutils import Vector
from blender_adapter import clear_scene,setup_stage,aim
p=argparse.ArgumentParser();p.add_argument('--glb',type=Path,required=True);p.add_argument('--out',type=Path,required=True)
a=p.parse_args(sys.argv[sys.argv.index('--')+1:]);clear_scene()
bpy.ops.import_scene.gltf(filepath=str(a.glb.resolve()))
objects={ob.get('stable_id'):ob for ob in bpy.context.scene.objects}
meshes=[ob for ob in objects.values() if ob.type=='MESH']
materials={s.material for ob in meshes for s in ob.material_slots}
assert all(any(n.type=='TEX_IMAGE' for n in m.node_tree.nodes) for m in materials)
assert not any(n.type=='TEX_NOISE' for m in materials for n in m.node_tree.nodes)
bpy.context.view_layer.update();leaf=objects['door.leaf']
target=leaf.matrix_world@Vector((-.62,.028,1.55))
camera=leaf.matrix_world@Vector((-1.34,-2.4,1.92))
scene=setup_stage(camera=camera,target=target,ortho=.60,resolution=(1200,1200),samples=32)
scene.cycles.use_denoising=False
scene.cycles.seed=0
for ob in bpy.data.objects:
    if ob.type=='LIGHT':ob.data.energy*=.82
key=bpy.data.objects['Stage.key'];key.location=leaf.matrix_world@Vector((-1.58,-1.45,2.0));key.data.energy=200;key.data.size=1.2;key.data.color=(1,.91,.78);aim(key,target)
bpy.data.objects['Stage.fill'].data.energy=500
scene.world.color=(.025,.030,.032)
a.out.parent.mkdir(parents=True,exist_ok=True);scene.render.filepath=str(a.out)
bpy.ops.render.render(write_still=True)
record=dict(glb=str(a.glb.resolve()),glb_sha256=hashlib.sha256(a.glb.read_bytes()).hexdigest(),camera=list(camera),target=list(target),ortho=.60,resolution=[1200,1200],samples=32,seed=0,denoising=False,shader='Actual reimported GLB baked PBR; no authoring shader',blender=bpy.app.version_string,script_sha256=hashlib.sha256(Path(__file__).read_bytes()).hexdigest(),image_sha256=hashlib.sha256(a.out.read_bytes()).hexdigest())
a.out.with_suffix('.json').write_text(json.dumps(record,indent=2)+'\n')
print('RENDERED '+str(a.out),flush=True)
