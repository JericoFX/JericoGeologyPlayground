"""Six damp timber windows, photographed in a single real Blender scene."""
import sys,math,json,hashlib
from pathlib import Path
ROOT=Path(__file__).resolve().parent;sys.path.insert(0,str(ROOT))
import bpy
from damp_wood import build_damp_window
from blender_adapter import clear_scene,import_asset,setup_stage,_material
from core import asset_hash
from evidence import freeze_file,freeze_manifest

def text(body,x,z,size=.065):
 cu=bpy.data.curves.new('Caption','FONT');cu.body=body;cu.align_x='CENTER';cu.size=size
 ob=bpy.data.objects.new('Stage.caption.'+body,cu);bpy.context.collection.objects.link(ob)
 ob.location=(x,-.19,z);ob.rotation_euler=(math.pi/2,0,0);cu.materials.append(bpy.data.materials['Stage.caption'])

def main():
 out=ROOT/'output/evidence';out.mkdir(parents=True,exist_ok=True);clear_scene();records=[]
 for family,z in [('old',2.20),('new',.32)]:
  for state,x in [('intact',-1.72),('broken',0),('empty',1.72)]:
   data=build_damp_window(dict(family=family,glazing=state,seed=29));records.append({'family':family,'state':state,'asset_hash':asset_hash(data)})
   import_asset(data,family+'.'+state,(x,0,z))
 scene=setup_stage(camera=(.30,-13,3.28),target=(0,0,2.12),ortho=5.50,resolution=(2100,1640),samples=96)
 scene.cycles.use_denoising=False;scene.render.threads=6;scene.view_settings.look='AgX - Medium High Contrast'
 scene.cycles.max_bounces=6;scene.cycles.diffuse_bounces=2;scene.cycles.glossy_bounces=2;scene.cycles.transmission_bounces=4
 scene.cycles.caustics_reflective=False;scene.cycles.caustics_refractive=False
 wallmat=_material('Stage.wall',(.066,.076,.078),.92)[0]
 bpy.ops.mesh.primitive_cube_add(size=1,location=(0,.60,2.10));o=bpy.context.object;o.name='Stage.wall';o.scale=(6,.08,4.4);o.data.materials.append(wallmat)
 line=_material('Stage.rear_detail',(.12,.14,.14),.9)[0]
 for base in (.32,2.20):
  for dz in (.30,.97):
   bpy.ops.mesh.primitive_cube_add(size=1,location=(0,.55,base+dz));o=bpy.context.object;o.name='Stage.depth_reference';o.scale=(6,.012,.025);o.data.materials.append(line)
 _material('Stage.caption',(.82,.85,.83),.75)
 text('VENTANAS DE MADERA HUMEDA',0,4.07,.105)
 text('MADERA VIEJA  /  desgaste y grietas longitudinales',0,3.74,.076)
 text('MADERA COMUN NUEVA  /  cantos y uniones mas integros',0,1.87,.076)
 for base in (.32,2.20):
  for state,x in [('ENTERO',-1.72),('ROTO',0),('VACIO',1.72)]:text(state,x,base-.245,.064)
 text('Claro 1,12 x 1,28 m   |   vidrio unico de 4 mm   |   misma humedad en ambas familias',0,-.09,.045)
 path=out/'damp-wood-six-states-working.png';scene.render.filepath=str(path);bpy.ops.render.render(write_still=True)
 record=freeze_file(path,'damp-wood-six-states');record.update(assets=records,blender=bpy.app.version_string,source_hashes={p.name:hashlib.sha256(p.read_bytes()).hexdigest() for p in ROOT.glob('*.py')})
 path=out/'damp-wood-authoring-working.blend';bpy.ops.wm.save_as_mainfile(filepath=str(path));record['blend']=freeze_file(path,'damp-wood-authoring')
 (out/'evidence-record.json').write_text(json.dumps(record,indent=2));print('EVIDENCE '+str(freeze_manifest([record],out)),flush=True)
if __name__=='__main__':main()
