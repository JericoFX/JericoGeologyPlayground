"""Read-only GLB import, lights/camera/stage only. Never saves or exports geometry."""
import argparse,sys,json,math
from pathlib import Path
import bpy
from mathutils import Vector
p=argparse.ArgumentParser();p.add_argument('--glb',required=True);p.add_argument('--out',required=True);p.add_argument('--resolution',type=int,default=760);p.add_argument('--samples',type=int,default=24);p.add_argument('--views',default='overview,books,joint,back');a=p.parse_args(sys.argv[sys.argv.index('--')+1:])
bpy.ops.object.select_all(action='SELECT');bpy.ops.object.delete(use_global=False)
bpy.ops.import_scene.gltf(filepath=str(Path(a.glb).resolve()))
assets=[o for o in bpy.context.scene.objects if o.type=='MESH']
assert all(not o.modifiers for o in assets)
scene=bpy.context.scene;scene.render.engine='CYCLES';scene.cycles.samples=a.samples;scene.cycles.use_denoising=False
scene.render.resolution_x=a.resolution;scene.render.resolution_y=a.resolution;scene.render.resolution_percentage=100
scene.world.color=(.12,.12,.12);scene.view_settings.view_transform='AgX'
def area(name,loc,power,size,color):
 d=bpy.data.lights.new(name,'AREA');d.energy=power;d.shape='DISK';d.size=size;d.color=color;o=bpy.data.objects.new(name,d);scene.collection.objects.link(o);o.location=loc;o.rotation_euler=(Vector((0,0,1.0))-o.location).to_track_quat('-Z','Y').to_euler()
area('large warm key',(-3,-4,4.5),400,3.5,(1,.92,.82));area('soft cool fill',(3,-1,3),250,3,(.8,.9,1));area('rim',(1,3,4),350,2,(1,.94,.85))
bpy.ops.mesh.primitive_plane_add(size=200,location=(0,0,-.008));ground=bpy.context.object;ground.name='VIEWER_ONLY_studio_floor';m=bpy.data.materials.new('VIEWER_ONLY_studio_material');m.diffuse_color=(.035,.045,.055,1);ground.data.materials.append(m)
camera=bpy.data.cameras.new('VIEWER_ONLY_camera');cam=bpy.data.objects.new('VIEWER_ONLY_camera',camera);scene.collection.objects.link(cam);scene.camera=cam;camera.type='ORTHO'
views={'overview':((3.3,-6.6,3.2),(0,0,1.04),2.65),'books':((.4,-3.8,1.95),(-.22,-.015,1.76),.87),'joint':((-1.6,-3.5,2.3),(-.58,-.1,1.60),.56),'back':((3,5,2.9),(0,0,1.04),2.65),'lower':((1.2,-3.6,1.2),(0,-.02,.52),1.18)}
out=Path(a.out);out.mkdir(parents=True,exist_ok=True)
for name in a.views.split(','):
 loc,target,scale=views[name];cam.location=loc;cam.rotation_euler=(Vector(target)-cam.location).to_track_quat('-Z','Y').to_euler();camera.ortho_scale=scale;scene.render.filepath=str(out/(name+'.png'));bpy.ops.render.render(write_still=True)
(out/'viewer-report.json').write_text(json.dumps({'source':str(a.glb),'read_only':True,'asset_meshes':len(assets),'asset_modifiers':0,'blender':bpy.app.version_string,'views':a.views.split(','),'resolution':a.resolution},indent=2))
