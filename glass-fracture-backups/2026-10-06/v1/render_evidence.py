"""blender -b --python render_evidence.py -- --quick --views closed open"""
import sys, json, argparse, hashlib, math
from pathlib import Path
ROOT=Path(__file__).resolve().parent
sys.path.insert(0,str(ROOT))
import bpy
from core import asset_hash
from presets import build_preset as build_asset
from blender_adapter import clear_scene,import_asset,setup_stage,aim


from evidence import freeze_file,freeze_manifest


def render_view(name,out,quick=False,preset="mine_timber"):
    label=(preset+"-"+name) if preset!="mine_timber" else name
    clear_scene()
    door_recipe=dict(kind='door',width=1.02,height=2.14,thickness=.052,damage='deteriorated',handle='lever',seed=42)
    window_recipe=dict(kind='window',width=1.28,height=1.18,thickness=.048,damage='deteriorated',handle='pull',seed=73)
    if name=='hardware_pull': door_recipe['handle']='pull'
    if name in ('open','open_oblique'):
        door_recipe['angle']=70; window_recipe['angle']=52
    if name=='damage':
        door_recipe['damage']='broken'; window_recipe['damage']='broken'
    door_recipe['preset']=preset; window_recipe['preset']=preset
    door=build_asset(door_recipe); window=build_asset(window_recipe)
    if name=='reference_front':
        import_asset(door,'door')
        scene=setup_stage(camera=(.15,-7,1.36),target=(0,0,1.15),ortho=2.67,resolution=(1050,1500),samples=96 if quick else 192)
        for obj in bpy.data.objects:
            if obj.type=='LIGHT':obj.data.energy*=.72
    elif name in ('hardware','hardware_pull'):
        import_asset(door,'door',(0,0,0))
        scene=setup_stage(camera=(1.7,-3.2,1.85),target=(.27,-.05,1.06),ortho=.67,resolution=(1200,1100),samples=48 if quick else 96)
    else:
        import_asset(door,'door',(-.88,0,0),name=='uv')
        import_asset(window,'window',(.97,.05,.73),name=='uv')
        scene=setup_stage(camera=((-3,-7,3.15) if name=='open_oblique' else (2.7,-7,3.0)),target=(.05,-.08,1.16),ortho=4.25,resolution=(1600,1120),samples=40 if quick else 96)
        # Quiet support plinth puts the smaller window at a readable height.
        bpy.ops.mesh.primitive_cube_add(size=1,location=(.97,.11,.3475))
        o=bpy.context.object; o.name='Stage.window_plinth'; o.dimensions=(1.65,.5,.71)
        bpy.ops.object.transform_apply(location=False,rotation=False,scale=True)
        o.data.materials.append(bpy.data.materials['Stage.concrete'])
        m=o.modifiers.new('soft corners','BEVEL'); m.width=.01; m.segments=3
    if name=='uv': scene.cycles.samples=24 if quick else 48
    if quick:
        scene.render.resolution_percentage=80
    tmp=out/(name+'-working.png')
    scene.render.filepath=str(tmp)
    bpy.ops.render.render(write_still=True)
    result=freeze_file(tmp,label)
    result.update(view=name,door_hash=asset_hash(door),window_hash=asset_hash(window),samples=scene.cycles.samples,door_recipe=door['recipe'],window_recipe=window['recipe'],blender=bpy.app.version_string,resolution=[scene.render.resolution_x,scene.render.resolution_y,scene.render.resolution_percentage],source_hashes={f:hashlib.sha256((ROOT/f).read_bytes()).hexdigest() for f in ['core.py','presets.py','blender_adapter.py','render_evidence.py']})
    if name=='closed':
        # Preview project is separate from each portable asset export.
        blend=out/'authoring-preview-working.blend'
        bpy.ops.wm.save_as_mainfile(filepath=str(blend))
        result['blend']=freeze_file(blend,preset+'-authoring-preview')
    return result


def main():
    ap=argparse.ArgumentParser(); ap.add_argument('--out',default=str(ROOT/'output/evidence')); ap.add_argument('--quick',action='store_true'); ap.add_argument('--preset',default='mine_timber',choices=['mine_timber','reinforced_timber']); ap.add_argument('--views',nargs='+',default=['closed','open','hardware','uv','damage'])
    a=ap.parse_args(sys.argv[sys.argv.index('--')+1:] if '--' in sys.argv else [])
    out=Path(a.out).resolve(); out.mkdir(parents=True,exist_ok=True)
    records=[]
    for view in a.views:
        records.append(render_view(view,out,a.quick,a.preset))
        manifest=freeze_manifest(records,out)
        print('FROZEN_RENDER '+json.dumps(records[-1]),flush=True)
        print('FROZEN_MANIFEST '+str(manifest),flush=True)

if __name__=='__main__': main()
