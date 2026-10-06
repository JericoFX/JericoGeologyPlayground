"""Render original authoring against its reimported GLB; same light and pose."""
import sys,json
from pathlib import Path
ROOT=Path(__file__).resolve().parent;sys.path.insert(0,str(ROOT))
import bpy
from furniture import build_furniture
from blender_adapter import clear_scene,import_asset,setup_stage
from evidence import freeze_file
for preset in ['cabinet','workbench']:
 clear_scene();record=json.loads(next((ROOT/'output/exports'/preset).glob('export-record_*.json')).read_text())
 spacing=1.1 if preset=='cabinet' else 1.93
 import_asset(build_furniture({'preset':preset}),'source',(-spacing/2,0,0))
 before=set(bpy.context.scene.objects);bpy.ops.import_scene.gltf(filepath=record['glb'])
 for ob in set(bpy.context.scene.objects)-before:
  if ob.parent is None:ob.location.x+=spacing/2
 scene=setup_stage(camera=(2.5,-7,3.5),target=(0,0,.68 if preset=='cabinet' else .42),ortho=3.08 if preset=='cabinet' else 4.25,resolution=(1000,625),samples=20)
 for ob in bpy.data.objects:
  if ob.type=='LIGHT':ob.data.energy*=.76
 for i,label in enumerate(['AUTORIA','GLB REIMPORTADO']):
  cu=bpy.data.curves.new(label,'FONT');cu.body=label;cu.align_x='CENTER';cu.size=.047 if preset=='cabinet' else .055
  ob=bpy.data.objects.new('Caption.'+label,cu);bpy.context.collection.objects.link(ob);ob.location=((i-.5)*spacing,-.59 if preset=='cabinet' else -.70,.018);ob.rotation_euler=(1.57079632679,0,0)
 path=ROOT/'output/evidence'/(preset+'-portable-check-working.png');scene.render.filepath=str(path);bpy.ops.render.render(write_still=True);print('PORTABLE_REVIEW '+json.dumps(freeze_file(path,preset+'-portable-check')),flush=True)
