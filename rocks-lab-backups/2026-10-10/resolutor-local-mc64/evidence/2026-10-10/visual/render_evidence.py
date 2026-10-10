"""Render-only evidence. Direct exported OBJ; never edit source coordinates, normals, topology, transforms."""
import argparse, copy, hashlib, importlib.util, json, sys, time
from pathlib import Path
import bpy
from mathutils import Vector, Matrix
p=argparse.ArgumentParser(); p.add_argument('--jobs',required=True); args=p.parse_args(sys.argv[sys.argv.index('--')+1:])
HERE=Path(__file__).resolve().parent
ROOT=HERE.parents[1]
SOURCE=ROOT/'original/prototype-v3-erosion-resolved/tools/render_surface_studies.py'
spec=importlib.util.spec_from_file_location('neutral_renderer',SOURCE); mod=importlib.util.module_from_spec(spec);spec.loader.exec_module(mod)
SETTINGS={
 'engine':'CYCLES','device':'CPU','threads':2,'samples':24,'denoising':False,'adaptive_sampling':False,'render_seed':1,'resolution':[800,800],
 'color_management':{'view_transform':'AgX','look':'AgX - Medium High Contrast','exposure':0.,'gamma':1.},
 'world':{'color':[.22,.22,.22],'strength':.5},
 'camera':{'location':[2.4,1.5,4.4],'target':[0.,0.,.05],'ortho_scale':2.7,'clip_start':.002,'clip_end':200.,'world_up':[0.,1.,0.]},
 'lights':[{'name':'Key','location':[-3.6,3.8,6.],'power_watts':1680.,'disk_diameter_meters':2.5},{'name':'Fill','location':[4.8,.8,4.4],'power_watts':260.,'disk_diameter_meters':3.8},{'name':'Rim','location':[-1.,4.6,-3.],'power_watts':560.,'disk_diameter_meters':2.8}],
 'material':{'base_color':[.30,.30,.30],'roughness':.87,'metallic':0.},'wireframe':None,
 'obj_import_axes':{'forward_axis':'Y','up_axis':'Z'},'source_geometry_changes':[],'source_normal_changes':[],'source_transform_changes':[],
 'light_aim_target':[0.,0.,.05]
}
assert bpy.app.version == (5,2,2), bpy.app.version_string
bpy.context.scene.render.threads_mode='FIXED';bpy.context.scene.render.threads=2
jobs=json.loads(Path(args.jobs).read_text())
for job in jobs:
 path=(ROOT/job['obj']).resolve(); out=HERE/'renders'/f"{job['name']}.png";recordpath=out.with_suffix('.provenance.json')
 settings=copy.deepcopy(SETTINGS)
 for key,val in job.get('settings',{}).items():
  if isinstance(val,dict) and isinstance(settings.get(key),dict):settings[key].update(val)
  else:settings[key]=val
 obj_hash=mod.digest(path);settings_hash=mod.stable_hash(settings);wrapper_hash=mod.digest(Path(__file__))
 if recordpath.exists() and out.exists():
  old=json.loads(recordpath.read_text())
  if old['obj_sha256']==obj_hash and old['settings_sha256']==settings_hash and old['wrapper_sha256']==wrapper_hash and old['png_sha256']==mod.digest(out):
   print('VERIFIED_SKIP '+job['name'],flush=True);continue
 start=time.monotonic();scene,clay=mod.setup_scene(settings)
 # Fix studio lamp orientations to original main target, even when camera moves.
 for lamp in (o for o in scene.objects if o.type=='LIGHT'):
  lamp.rotation_euler=(Vector(settings['light_aim_target'])-lamp.location).to_track_quat('-Z','Y').to_euler()
 z=(scene.camera.location-Vector(settings['camera']['target'])).normalized();x=Vector(settings['camera']['world_up']).cross(z).normalized();y=z.cross(x).normalized();scene.camera.rotation_euler=Matrix((x,y,z)).transposed().to_euler()
 bpy.ops.object.select_all(action='DESELECT');bpy.ops.wm.obj_import(filepath=str(path),forward_axis='Y',up_axis='Z')
 meshes=[o for o in bpy.context.selected_objects if o.type=='MESH'];assert meshes
 before={o.name:mod.mesh_fingerprint(o) for o in meshes}
 stats=[]
 for obj in meshes:
  assert not obj.modifiers and obj.data.has_custom_normals
  obj.data.materials.clear();obj.data.materials.append(clay)
  stats.append({'name':obj.name,'vertices':len(obj.data.vertices),'triangles':len(obj.data.polygons),'has_custom_normals':obj.data.has_custom_normals,'smooth_polygons':sum(p.use_smooth for p in obj.data.polygons),'matrix_world':[list(row) for row in obj.matrix_world]})
 scene.render.filepath=str(out);bpy.ops.render.render(write_still=True)
 assert before=={o.name:mod.mesh_fingerprint(o) for o in meshes};assert obj_hash==mod.digest(path)
 record={'job':job,'obj':str(path),'obj_sha256':obj_hash,'png_sha256':mod.digest(out),'settings':settings,'settings_sha256':settings_hash,'blender_version':bpy.app.version_string,'imported_geometry_normals_transforms_unchanged':True,'mesh_stats':stats,'imported_fingerprint':before,'renderer_source_sha256':mod.digest(SOURCE),'wrapper_sha256':wrapper_hash,'seconds':time.monotonic()-start}
 recordpath.write_text(json.dumps(record,indent=2,ensure_ascii=False)+'\n');print('CLOUD_RENDER_VERIFIED '+str(out),flush=True)
