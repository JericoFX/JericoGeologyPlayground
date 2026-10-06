"""Render the actual reimported fine-wear GLB with its baked PBR, not source nodes."""
import argparse
import hashlib
import json
import sys
from pathlib import Path

ROOT = Path(__file__).resolve().parent
sys.path.insert(0, str(ROOT))
import bpy
from mathutils import Vector
from blender_adapter import clear_scene, setup_stage, aim
from evidence import freeze_file, freeze_manifest


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--glb', type=Path, required=True)
    parser.add_argument('--label', default='fine-wear-portable-1024')
    parser.add_argument('--size', type=int, default=900)
    parser.add_argument('--samples', type=int, default=24)
    args = parser.parse_args(sys.argv[sys.argv.index('--')+1:] if '--' in sys.argv else [])
    glb = args.glb.resolve()
    out = ROOT/'output/evidence'
    out.mkdir(parents=True, exist_ok=True)
    clear_scene()
    bpy.ops.import_scene.gltf(filepath=str(glb))
    objects = {ob.get('stable_id'):ob for ob in bpy.context.scene.objects}
    meshes = [ob for ob in objects.values() if ob.type == 'MESH']
    if not meshes:
        raise ValueError('No GLB meshes imported')
    materials = {slot.material for ob in meshes for slot in ob.material_slots}
    if not all(any(n.type=='TEX_IMAGE' for n in mat.node_tree.nodes) for mat in materials):
        raise ValueError('Expected imported texture-based PBR materials')
    if any(any(n.type=='TEX_NOISE' for n in mat.node_tree.nodes) for mat in materials):
        raise ValueError('Procedural source shader must not enter portable preview')
    bpy.context.view_layer.update()
    leaf = objects['door.leaf']
    target = leaf.matrix_world @ Vector((-.913,.028,.862))
    camera = leaf.matrix_world @ Vector((-1.60,-2.65,1.05))
    scene = setup_stage(camera=camera,target=target,ortho=.345,
                        resolution=(args.size,args.size),samples=args.samples)
    scene.cycles.use_denoising = False
    for ob in bpy.data.objects:
        if ob.type == 'LIGHT':ob.data.energy *= .82
    key=bpy.data.objects['Stage.key']
    key.location=leaf.matrix_world @ Vector((-1.58,-1.45,1.9))
    key.data.energy=200;key.data.size=1.2;key.data.color=(1,.91,.78);aim(key,target)
    bpy.data.objects['Stage.fill'].data.energy=500
    scene.world.color=(.025,.030,.032)
    provenance=dict(glb=glb.name,glb_sha256=hashlib.sha256(glb.read_bytes()).hexdigest(),
                    shader='Actual GLB reimport; baked texture PBR only',camera=list(camera),target=list(target),
                    ortho=.345,resolution=[args.size,args.size],samples=args.samples,blender=bpy.app.version_string,
                    script_sha256=hashlib.sha256(Path(__file__).read_bytes()).hexdigest())
    pre=freeze_manifest([provenance],out)
    png=out/(args.label+'-working.png');scene.render.filepath=str(png)
    bpy.ops.render.render(write_still=True)
    record=freeze_file(png,args.label);record.update(provenance,pre_render_manifest=str(pre))
    print('PORTABLE_RENDER '+json.dumps(record),flush=True)
    print('PORTABLE_RENDER_MANIFEST '+str(freeze_manifest([record],out)),flush=True)

if __name__=='__main__':main()
