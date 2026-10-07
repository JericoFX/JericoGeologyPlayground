"""Matched offline oracle or native-output-only preview. No native asset modifiers."""
import argparse,json,sys,math,hashlib
from pathlib import Path
ROOT=Path(__file__).resolve().parents[1]
import bpy
from mathutils import Vector
sys.path.insert(0,str(ROOT/'tools'))
from render_compare import material,aim,native_import

def setup(camera,target,ortho,resolution,samples,energy=1):
 s=bpy.context.scene;s.render.engine='CYCLES';s.cycles.samples=samples;s.cycles.seed=17;s.cycles.use_denoising=False;s.render.threads_mode='FIXED';s.render.threads=4;s.render.resolution_x=resolution[0];s.render.resolution_y=resolution[1];s.render.resolution_percentage=100;s.render.image_settings.file_format='PNG';s.world.color=(.045,.05,.06);s.view_settings.view_transform='AgX';s.view_settings.look='AgX - Medium High Contrast'
 bpy.ops.mesh.primitive_plane_add(size=200,location=(0,0,-.015));bpy.context.object.name='Stage.floor';bpy.context.object.data.materials.append(material('Stage.concrete',(.033,.035,.034),.92))
 for name,loc,power,color,size in [('key',(-3.5,-4.5,6),1550,(1,.82,.63),4),('fill',(3,-2,4),900,(.65,.77,1),3),('rim',(0,3,4.5),1500,(1,.9,.75),3)]:
  ld=bpy.data.lights.new('Stage.'+name,'AREA');ld.energy=power*energy;ld.shape='DISK';ld.size=size;ld.color=color;lo=bpy.data.objects.new('Stage.'+name,ld);bpy.context.collection.objects.link(lo);lo.location=loc;aim(lo,(0,0,1.2))
 cd=bpy.data.cameras.new('Stage.camera');co=bpy.data.objects.new('Stage.camera',cd);bpy.context.collection.objects.link(co);co.location=camera;aim(co,target);cd.type='ORTHO';cd.ortho_scale=ortho;cd.lens=50;s.camera=co;return s

def main():
 p=argparse.ArgumentParser();p.add_argument('--family',choices=['industrial','finewear','wood','singlepane','furniture','damp'],required=True);p.add_argument('--mode',choices=['reference','native','glb'],required=True);p.add_argument('--recipe',required=True);p.add_argument('--scene');p.add_argument('--glb');p.add_argument('--out',required=True);p.add_argument('--clay',action='store_true');p.add_argument('--samples',type=int,default=32);p.add_argument('--scale',type=float,default=.5);p.add_argument('--view',default='source');a=p.parse_args(sys.argv[sys.argv.index('--')+1:]);recipe=json.loads(Path(a.recipe).read_text())
 bpy.ops.object.select_all(action='SELECT');bpy.ops.object.delete(use_global=False)
 source={'industrial':'JericoIndustrialPropsMaterialsR2Checkpoint_20261006_1927','finewear':'JericoIndustrialPropsFineWearCheckpoint_20261006_1921','wood':'JericoWindowsSinglePaneLab','singlepane':'JericoWindowsSinglePaneLab','furniture':'JericoWoodFurnitureCheckpoint_20261006_1956','damp':'JericoDampWoodWindowsDirectedWIP_20261006_1956'}[a.family]
 family=a.family;preset=recipe.get('preset','cabinet');offset=(0,0,0)
 if family in ('industrial','finewear'):
  camera=(.9,-7,2.35);target=(0,0,1.06);ortho=2.63;resolution=(960,1280);energy=.82
  if preset=='industrial_window':offset=(0,0,.28)
 elif family=='wood':camera=(.15,-7,1.36);target=(0,0,1.15);ortho=2.67;resolution=(1050,1500);energy=.72
 elif family=='furniture':camera=(2.5,-7,3.5);target=(0,0,.68 if preset=='cabinet' else .42);ortho=2.05 if preset=='cabinet' else 2.5;resolution=(1200,1200);energy=.76
 else:camera=(.32,-7,1.9);target=(0,0,.86);ortho=1.90;resolution=(1000,1000);energy=1;offset=(0,0,.20)
 resolution=tuple(round(x*a.scale) for x in resolution)
 if a.mode=='reference':
  sys.path.insert(0,str(ROOT/'fixtures/sources'/source))
  if family in ('industrial','finewear'):
   from industrial import build_industrial as build
  elif family=='wood':
   from presets import build_preset as build
  elif family=='singlepane':
   from singlepane import build_singlepane as build
  elif family=='furniture':
   from furniture import build_furniture as build
  else:
   from damp_wood import build_damp_window as build
  from blender_adapter import import_asset
  data=build(recipe);objects=import_asset(data,'reference',offset)
  if a.clay:
   solid=material('Reference.clay',(.32,.37,.36),.78)
   for ob in objects.values():
    if ob.type=='MESH':
     for i in range(len(ob.data.materials)):ob.data.materials[i]=solid
 elif a.mode=='glb':
  bpy.ops.import_scene.gltf(filepath=str(Path(a.glb).resolve()))
  imported=list(bpy.context.scene.objects)
  for ob in imported:
   if ob.parent is None:ob.location+=Vector(offset)
   assert not ob.modifiers,'Portable preview must not repair the asset'
 else:
  scene_path=Path(a.scene);data=json.loads(scene_path.read_text());native_import(data,a.clay,scene_path.parent,offset)
 asset_objects=[o for o in bpy.context.scene.objects if o.type=='MESH']
 s=setup(camera,target,ortho,resolution,a.samples,energy)
 if family in ('industrial','finewear'):s.world.color=(.025,.030,.032)
 if family in ('singlepane','damp'):
  wallmat=material('Stage.wall',(.066,.076,.078),.92);bpy.ops.mesh.primitive_cube_add(size=1,location=(0,.60,1.0));bpy.context.object.scale=(6,.08,2.4);bpy.context.object.data.materials.append(wallmat)
  band=material('Stage.rear_detail',(.12,.14,.14),.9)
  for z in (.50,1.17):
   bpy.ops.mesh.primitive_cube_add(size=1,location=(0,.55,z));bpy.context.object.scale=(6,.012,.025);bpy.context.object.data.materials.append(band)
 if a.view=='closeup':
  s.camera.location=(.05,-7,2.01);aim(s.camera,(0,0,2));s.camera.data.ortho_scale=1.35
 if a.view=='oblique':s.camera.location=(2.8,-5,3);aim(s.camera,target)
 from bpy_extras.object_utils import world_to_camera_view
 bpy.context.view_layer.update()
 projected=[world_to_camera_view(s,s.camera,o.matrix_world@v.co) for o in asset_objects for v in o.data.vertices]
 bounds=[max(0,int(min(v.x for v in projected)*resolution[0])),max(0,int((1-max(v.y for v in projected))*resolution[1])),min(resolution[0],math.ceil(max(v.x for v in projected)*resolution[0])),min(resolution[1],math.ceil((1-min(v.y for v in projected))*resolution[1]))]
 out=Path(a.out);out.parent.mkdir(parents=True,exist_ok=True);s.render.filepath=str(out);bpy.ops.render.render(write_still=True)
 out.with_suffix('.settings.json').write_text(json.dumps(dict(family=family,mode=a.mode,source=source,recipe=recipe,camera=list(s.camera.location),rotation=list(s.camera.rotation_euler),ortho=s.camera.data.ortho_scale,resolution=resolution,samples=a.samples,clay=a.clay,native_asset_modifiers=0 if a.mode in ('native','glb') else None,view=a.view,read_only_glb_import=a.mode=='glb',asset_pixel_bounds=bounds,blender=bpy.app.version_string),indent=2))
if __name__=='__main__':main()
