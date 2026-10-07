"""Read-only GLB viewer. Stage/lights/cameras only; asset data stays unchanged."""
import argparse,sys,json
from pathlib import Path
import bpy
from mathutils import Vector
p=argparse.ArgumentParser();p.add_argument('--glb',required=True);p.add_argument('--out',required=True);p.add_argument('--resolution',type=int,default=720);p.add_argument('--samples',type=int,default=32);p.add_argument('--views',default='overview,mug,lantern,bottle');a=p.parse_args(sys.argv[sys.argv.index('--')+1:])
bpy.ops.object.select_all(action='SELECT');bpy.ops.object.delete(use_global=False)
bpy.ops.import_scene.gltf(filepath=str(Path(a.glb).resolve()))
assets=[o for o in bpy.context.scene.objects if o.type=='MESH'];assert all(not o.modifiers for o in assets)
s=bpy.context.scene;s.render.engine='CYCLES';s.cycles.samples=a.samples;s.cycles.use_denoising=False;s.cycles.max_bounces=12;s.cycles.transmission_bounces=10
s.render.resolution_x=a.resolution;s.render.resolution_y=a.resolution;s.render.resolution_percentage=100;s.world.color=(.12,.12,.12);s.view_settings.view_transform='AgX'
def light(name,loc,power,size,color):
 d=bpy.data.lights.new(name,'AREA');d.energy=power;d.shape='DISK';d.size=size;d.color=color;o=bpy.data.objects.new(name,d);s.collection.objects.link(o);o.location=loc;o.rotation_euler=(Vector((0,0,.10))-o.location).to_track_quat('-Z','Y').to_euler()
light('viewer key',(-.5,-.65,1),33,.8,(1,.92,.83));light('viewer fill',(.6,-.1,.55),16,.6,(.82,.9,1));light('viewer rim',(0,.5,.7),26,.5,(1,.94,.83))
bpy.ops.mesh.primitive_plane_add(size=20,location=(0,0,-.001));floor=bpy.context.object;floor.name='VIEWER_ONLY_floor';mat=bpy.data.materials.new('VIEWER_ONLY_floor_material');mat.diffuse_color=(.025,.032,.038,1);floor.data.materials.append(mat)
d=bpy.data.cameras.new('VIEWER_ONLY_camera');cam=bpy.data.objects.new('VIEWER_ONLY_camera',d);s.collection.objects.link(cam);s.camera=cam;d.type='ORTHO'
views={'overview':((.47,-1.4,.66),(.012,0,.12),.66),'mug':((.11,-.65,.33),(-.19,0,.046),.17),'lantern':((.31,-1,.47),(0,0,.140),.33),'bottle':((.51,-.9,.36),(.20,0,.095),.245)}
out=Path(a.out);out.mkdir(parents=True,exist_ok=True)
for name in a.views.split(','):
 loc,target,scale=views[name];cam.location=loc;cam.rotation_euler=(Vector(target)-cam.location).to_track_quat('-Z','Y').to_euler();d.ortho_scale=scale;s.render.filepath=str(out/(name+'.png'));bpy.ops.render.render(write_still=True)
(out/'viewer-report.json').write_text(json.dumps({'read_only':True,'asset_meshes':len(assets),'asset_modifiers':0,'blender':bpy.app.version_string,'resolution':a.resolution,'samples':a.samples,'denoised':False,'views':a.views.split(',')},indent=2))
