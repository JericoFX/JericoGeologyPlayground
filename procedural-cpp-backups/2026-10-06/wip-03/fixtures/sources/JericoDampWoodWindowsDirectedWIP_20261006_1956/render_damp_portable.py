"""Evidence rendered only from reimported delivered GLB, never source shaders."""
import sys,json,math
from pathlib import Path
ROOT=Path(__file__).resolve().parent;sys.path.insert(0,str(ROOT))
import bpy
from blender_adapter import clear_scene,setup_stage,_material
from evidence import freeze_file,freeze_manifest

def main():
 out=ROOT/'output/evidence';out.mkdir(parents=True,exist_ok=True);clear_scene();files=[]
 records=json.loads((ROOT/'output/exports/export-records.json').read_text())
 for family,x in [('old',-.82),('new',.82)]:
  record=next(r for r in records if r['family']==family and r['state']=='broken');files.append(record['glb'])
  before=set(bpy.data.objects);bpy.ops.import_scene.gltf(filepath=record['glb'])
  for o in set(bpy.data.objects)-before:
   if not o.parent:o.location.x+=x;o.location.z+=.23
 scene=setup_stage(camera=(.35,-8,2),target=(0,0,.98),ortho=3.65,resolution=(1300,850),samples=64);scene.render.threads=4
 scene.cycles.max_bounces=6;scene.cycles.diffuse_bounces=2;scene.cycles.glossy_bounces=2;scene.cycles.transmission_bounces=4
 scene.cycles.caustics_reflective=False;scene.cycles.caustics_refractive=False
 wallmat=_material('Stage.wall',(.066,.076,.078),.92)[0]
 bpy.ops.mesh.primitive_cube_add(size=1,location=(0,.6,1.0));o=bpy.context.object;o.name='Stage.wall';o.scale=(4,.08,2.5);o.data.materials.append(wallmat)
 mat=_material('Stage.caption',(.82,.85,.83),.75)[0]
 for body,x,z,size in [('GLB REIMPORTADO / PBR HORNEADO',0,1.95,.084),('VIEJA HUMEDA',-.82,.013,.073),('NUEVA HUMEDA',.82,.013,.073)]:
  cu=bpy.data.curves.new('Caption','FONT');cu.body=body;cu.align_x='CENTER';cu.size=size
  ob=bpy.data.objects.new('Stage.caption.'+body,cu);bpy.context.collection.objects.link(ob);ob.location=(x,-.17,z);ob.rotation_euler=(math.pi/2,0,0);cu.materials.append(mat)
 path=out/'damp-portable-working.png';scene.render.filepath=str(path);bpy.ops.render.render(write_still=True)
 record=freeze_file(path,'damp-wood-portable-reimport');record.update(source='Delivered GLB imported into a clean Blender scene; PBR textures only',glbs=files,blender=bpy.app.version_string)
 (out/'portable-evidence-record.json').write_text(json.dumps(record,indent=2));print(freeze_manifest([record],out))
if __name__=='__main__':main()
