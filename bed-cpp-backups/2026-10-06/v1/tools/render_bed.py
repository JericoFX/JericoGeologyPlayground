"""Read-only GLB/OBJ viewer: no geometry, materials, normals, UV or modifiers repaired."""
import bpy,sys,argparse,json
from pathlib import Path
from mathutils import Vector
p=argparse.ArgumentParser();p.add_argument('--asset',required=True);p.add_argument('--out',required=True);p.add_argument('--resolution',type=int,default=700);p.add_argument('--samples',type=int,default=24);p.add_argument('--overview-only',action='store_true');a=p.parse_args(sys.argv[sys.argv.index('--')+1:])
bpy.ops.object.select_all(action='SELECT');bpy.ops.object.delete(use_global=False)
asset=Path(a.asset).resolve()
if asset.suffix=='.glb':bpy.ops.import_scene.gltf(filepath=str(asset))
else:bpy.ops.wm.obj_import(filepath=str(asset),forward_axis='Y',up_axis='Z')
assets=list(bpy.context.scene.objects)
for ob in assets:assert not ob.modifiers
scene=bpy.context.scene;scene.render.engine='CYCLES';scene.cycles.samples=a.samples;scene.cycles.use_denoising=False;scene.render.resolution_x=a.resolution;scene.render.resolution_y=int(a.resolution*.8);scene.render.resolution_percentage=100
scene.world.color=(.14,.14,.14);scene.view_settings.view_transform='AgX'
def area(name,loc,power,size,color):
 d=bpy.data.lights.new(name,'AREA');d.energy=power;d.shape='DISK';d.size=size;d.color=color;o=bpy.data.objects.new(name,d);scene.collection.objects.link(o);o.location=loc;o.rotation_euler=(Vector((0,0,.5))-o.location).to_track_quat('-Z','Y').to_euler()
area('key',(-3,-2.8,4.4),430,3.4,(1,.93,.84));area('fill',(2.6,-1,2.5),210,2.5,(.81,.88,1));area('rim',(-.5,3.2,3.0),350,2.0,(1,.91,.8))
bpy.ops.mesh.primitive_plane_add(size=200,location=(0,0,-.003));ground=bpy.context.object;ground.name='VIEWER_ONLY_ground';m=bpy.data.materials.new('viewer ground');m.diffuse_color=(.043,.051,.052,1);ground.data.materials.append(m)
d=bpy.data.cameras.new('camera');cam=bpy.data.objects.new('camera',d);scene.collection.objects.link(cam);scene.camera=cam;d.type='ORTHO'
views=[('01-overview',(-3.0,-3.8,2.2),(0,0,.54),2.83),('02-structure',(-2.8,-2.1,1.15),(-.27,-.82,.52),.98),('03-textiles',(-1.8,1.5,1.8),(-.03,.39,.59),1.05),('04-side',(-3.5,0,1.2),(0,0,.53),2.6)]
if a.overview_only:views=views[:1]
out=Path(a.out);out.mkdir(parents=True,exist_ok=True)
for name,loc,target,scale in views:
 cam.location=loc;cam.rotation_euler=(Vector(target)-cam.location).to_track_quat('-Z','Y').to_euler();d.ortho_scale=scale;scene.render.filepath=str(out/(name+'.png'));bpy.ops.render.render(write_still=True)
(out/'reimport.json').write_text(json.dumps(dict(asset=asset.name,read_only=True,blender=bpy.app.version_string,modifiers=0,meshes=sum(ob.type=='MESH' for ob in assets),vertices=sum(len(ob.data.vertices) for ob in assets if ob.type=='MESH'),views=views),indent=2))
