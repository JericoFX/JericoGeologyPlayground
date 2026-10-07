"""True Blender before/after evidence, unchanged frame and matched lighting."""
import sys,json,hashlib
from pathlib import Path
ROOT=Path(__file__).resolve().parent;sys.path.insert(0,str(ROOT))
import bpy
from industrial import build_industrial
from glass_fracture import build_fracture
from blender_adapter import clear_scene,import_asset,setup_stage,_material
from evidence import freeze_file,freeze_manifest
from core import asset_hash

def main():
 clay='--clay' in sys.argv
 out=ROOT/'output/fracture';out.mkdir(parents=True,exist_ok=True)
 recipe=json.loads((ROOT/'recipes/industrial_window_broken.json').read_text());before=build_industrial(recipe);after=build_fracture(recipe)
 assert [p for p in before['parts'] if p['role']!='GlassPane']==[p for p in after['parts'] if p['role']!='GlassPane']
 source_hashes={f:hashlib.sha256((ROOT/f).read_bytes()).hexdigest() for f in ['glass_fracture.py','glass_materials.py','blender_adapter.py','industrial.py','render_glass_fracture.py']}
 clear_scene();import_asset(before,'before',(-.83,0,.20));import_asset(after,'after',(.83,0,.20))
 scene=setup_stage(camera=(.62,-7,2.0),target=(0,0,.86),ortho=3.43,resolution=(1200,850),samples=128)
 scene.cycles.use_denoising=False;scene.render.threads=6
 if clay:
  scene.cycles.samples=32
  solid=_material('Diagnostic.glass.clay',(.32,.37,.36),.78)[0]
  for o in bpy.data.objects:
   if o.type=='MESH' and o.get('role')=='GlassPane':
    o.data.materials.clear();o.data.materials.append(solid)
 # Same simple stage behind both panes makes transmitted background observable.
 mat=_material('Stage.rear_wall',(.052,.065,.062),.94)[0]
 bpy.ops.mesh.primitive_cube_add(size=1,location=(0,.85,1.0));wall=bpy.context.object;wall.name='Stage.rear_wall';wall.scale=(4,.08,2.4);wall.data.materials.append(mat)
 bandmat=_material('Stage.background_band',(.13,.115,.082),.89)[0]
 for z in [.53,1.13]:
  bpy.ops.mesh.primitive_cube_add(size=1,location=(0,.79,z));o=bpy.context.object;o.name='Stage.background_depth_marker';o.scale=(4,.007,.045);o.data.materials.append(bandmat)
 captionmat=_material('Stage.caption',(.70,.67,.60),.7)[0]
 for x,label in [(-.83,'ANTES'),(.83,'ROTURA NUEVA')]:
  cu=bpy.data.curves.new('caption','FONT');cu.body=label;cu.align_x='CENTER';cu.size=.073
  ob=bpy.data.objects.new('Stage.caption.'+label,cu);bpy.context.collection.objects.link(ob);ob.location=(x,-.16,.014);ob.rotation_euler=(math.pi/2,0,0);cu.materials.append(captionmat)
 label='glass-clay-topology' if clay else 'glass-before-after'
 path=out/(label+'-working.png');scene.render.filepath=str(path);bpy.ops.render.render(write_still=True)
 record=freeze_file(path,label);record.update(before_hash=asset_hash(before),after_hash=asset_hash(after),same_frame=True,diagnostic_clay=clay,blender=bpy.app.version_string,source_hashes=source_hashes,metrics=after['metadata']['glass_fracture'])
 path=out/(label+'-working.blend');bpy.ops.wm.save_as_mainfile(filepath=str(path));record['blend']=freeze_file(path,label+'-scene')
 (out/(label+'-record.json')).write_text(json.dumps(record,indent=2));print('EVIDENCE '+str(freeze_manifest([record],out)),flush=True)
if __name__=='__main__':
 import math
 main()
