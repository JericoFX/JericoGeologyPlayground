"""Dedicated evidence view of the final companion window, no external pixels."""
import sys,json,hashlib
from pathlib import Path
ROOT=Path(__file__).resolve().parents[1];sys.path.insert(0,str(ROOT))
import bpy
from core import asset_hash
from presets import build_preset
from blender_adapter import clear_scene,import_asset,setup_stage
from evidence import freeze_file,freeze_manifest
out=ROOT/'output/evidence';out.mkdir(parents=True,exist_ok=True)
recipe=json.loads((ROOT/'recipes/window_reinforced.json').read_text())
scene_data=build_preset(recipe)
clear_scene();import_asset(scene_data,'window',(0,0,.14))
scene=setup_stage(camera=(2,-5,1.75),target=(0,0,.73),ortho=1.95,resolution=(1300,1200),samples=112)
for obj in bpy.data.objects:
    if obj.type=='LIGHT':obj.data.energy*=.74
scene.render.filepath=str(out/'window-companion-working.png')
bpy.ops.render.render(write_still=True)
result=freeze_file(Path(scene.render.filepath),'window-companion')
result.update(view='window_companion',recipe=scene_data['recipe'],asset_hash=asset_hash(scene_data),samples=112,blender=bpy.app.version_string,source_hashes={f:hashlib.sha256((ROOT/f).read_bytes()).hexdigest() for f in ['core.py','presets.py','blender_adapter.py','scripts/render_window.py']})
print('FROZEN_RENDER '+json.dumps(result),flush=True)
print('FROZEN_MANIFEST '+str(freeze_manifest([result],out)),flush=True)
