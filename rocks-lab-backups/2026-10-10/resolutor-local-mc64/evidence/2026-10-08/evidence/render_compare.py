"""Cloud QA only: render actual exported C++ OBJ, never modify source geometry."""
import argparse, hashlib, importlib.util, json, sys, time
from pathlib import Path
import bpy
parser=argparse.ArgumentParser(); parser.add_argument('--obj',required=True);parser.add_argument('--out',required=True);parser.add_argument('--source',required=True);parser.add_argument('--view',choices=['main','front','detail','side','join305','join999'],default='main'); args=parser.parse_args(sys.argv[sys.argv.index('--')+1:])
path=Path(args.obj).resolve(); out=Path(args.out).resolve();out.parent.mkdir(parents=True,exist_ok=True)
source=Path(args.source).resolve(); spec=importlib.util.spec_from_file_location('neutral_render',source/'tools/render_surface_studies.py'); module=importlib.util.module_from_spec(spec);spec.loader.exec_module(module)
settings={
'samples':24,'resolution':[800,800],
'color_management':{'view_transform':'AgX','look':'AgX - Medium High Contrast','exposure':0.,'gamma':1.},
'world':{'color':[.22,.22,.22],'strength':.5},
'camera':{'location':[2.4,1.5,4.4],'target':[0.,0.,.05],'ortho_scale':2.7,'clip_start':.002,'clip_end':200.},
'lights':[{'name':'Key','location':[-3.6,3.8,6.],'power_watts':1680.,'disk_diameter_meters':2.5},{'name':'Fill','location':[4.8,.8,4.4],'power_watts':260.,'disk_diameter_meters':3.8},{'name':'Rim','location':[-1.,4.6,-3.],'power_watts':560.,'disk_diameter_meters':2.8}],
'material':{'base_color':[.30,.30,.30],'roughness':.87},'wireframe':None}
if args.view=='front': settings['camera'].update(location=[.5,.3,5.],target=[0.,0.,0.],ortho_scale=2.3)
if args.view=='detail': settings['camera'].update(location=[1.3,.7,5.],target=[-.23,.17,.24],ortho_scale=1.3)
if args.view=='side': settings['camera'].update(location=[5.,.25,.7],target=[0.,0.,.05],ortho_scale=2.3)
if args.view=='join305': settings['camera'].update(location=[-2.8,-.80,-1.15],target=[-.9375,-.5390625,-.3515625],ortho_scale=.18)
if args.view=='join999': settings['camera'].update(location=[2.6,.1,2.5],target=[.8203125,-.0234375,.375],ortho_scale=.18)
if args.view.startswith('join'): settings['wireframe']={'width_pixels':.8,'color':[.015,.08,.115]}
obj_hash=module.digest(path);start=time.monotonic();scene,clay=module.setup_scene(settings)
from mathutils import Vector, Matrix
z=(scene.camera.location-Vector(settings['camera']['target'])).normalized();x=Vector((0.,1.,0.)).cross(z).normalized();y=z.cross(x).normalized();scene.camera.rotation_euler=Matrix((x,y,z)).transposed().to_euler();settings['camera']['world_up']=[0.,1.,0.]
bpy.ops.object.select_all(action='DESELECT');bpy.ops.wm.obj_import(filepath=str(path),forward_axis='Y',up_axis='Z')
meshes=[o for o in bpy.context.selected_objects if o.type=='MESH'];assert meshes
before={o.name:module.mesh_fingerprint(o) for o in meshes}
for o in meshes:
 assert not o.modifiers and o.data.has_custom_normals
 o.data.materials.clear();o.data.materials.append(clay)
scene.render.filepath=str(out);bpy.ops.render.render(write_still=True)
assert before=={o.name:module.mesh_fingerprint(o) for o in meshes};assert obj_hash==module.digest(path)
record={'obj':str(path),'obj_sha256':obj_hash,'png_sha256':module.digest(out),'settings':settings,'blender_version':bpy.app.version_string,'imported_geometry_normals_transforms_unchanged':True,'triangles':sum(len(o.data.polygons) for o in meshes),'imported_fingerprint':before,'renderer_source_sha256':module.digest(source/'tools/render_surface_studies.py'),'wrapper_sha256':module.digest(Path(__file__)),'seconds':time.monotonic()-start}
out.with_suffix('.provenance.json').write_text(json.dumps(record,indent=2)+'\n');print('CLOUD_RENDER_VERIFIED '+str(out))
