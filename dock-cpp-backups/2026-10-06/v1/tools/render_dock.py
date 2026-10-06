"""Read-only Blender GLB preview. Does not generate or repair dock geometry."""
import bpy,sys,argparse,json,math
from pathlib import Path
from mathutils import Vector
p=argparse.ArgumentParser();p.add_argument('--glb',required=True);p.add_argument('--out',required=True);p.add_argument('--resolution',type=int,default=720);p.add_argument('--samples',type=int,default=24);p.add_argument('--clay',action='store_true');a=p.parse_args(sys.argv[sys.argv.index('--')+1:])
bpy.ops.object.select_all(action='SELECT');bpy.ops.object.delete(use_global=False)
bpy.ops.import_scene.gltf(filepath=str(Path(a.glb).resolve()))
assets=list(bpy.context.scene.objects)
for o in assets: assert not o.modifiers,'Viewer may not repair asset geometry'
if a.clay:
 m=bpy.data.materials.new('QA neutral clay');m.diffuse_color=(.42,.46,.49,1);m.use_nodes=True;m.node_tree.nodes.get('Principled BSDF').inputs['Base Color'].default_value=(.42,.46,.49,1);m.node_tree.nodes.get('Principled BSDF').inputs['Roughness'].default_value=.7
 for ob in assets:
  if ob.type=='MESH':ob.data.materials.clear();ob.data.materials.append(m)
scene=bpy.context.scene;scene.render.engine='CYCLES';scene.cycles.samples=a.samples;scene.cycles.use_denoising=False
scene.render.resolution_x=a.resolution;scene.render.resolution_y=int(a.resolution*.75);scene.render.resolution_percentage=100
scene.world.color=(.20,.20,.20);scene.view_settings.view_transform='AgX'
def area(name,loc,power,size,color):
 data=bpy.data.lights.new(name,'AREA');data.energy=power;data.shape='DISK';data.size=size;data.color=color;o=bpy.data.objects.new(name,data);scene.collection.objects.link(o);o.location=loc;o.rotation_euler=(Vector((0,2,1))-o.location).to_track_quat('-Z','Y').to_euler()
area('large soft key',(-3,-4,10),1800,7,(1,.89,.77));area('cool fill',(5,6,7),1400,6,(.73,.84,1));area('edge',(-7,7,6),1300,5,(1,.96,.88))
# Stage geometry is kept distinct and never saved or included in exported asset.
bpy.ops.mesh.primitive_plane_add(size=200,location=(0,0,-.66));stage=bpy.context.object;stage.name='VIEWER_ONLY_ground';mat=bpy.data.materials.new('studio ground');mat.diffuse_color=(.075,.095,.12,1);stage.data.materials.append(mat)
data=bpy.data.cameras.new('QA camera');cam=bpy.data.objects.new('QA camera',data);scene.collection.objects.link(cam);scene.camera=cam;data.type='ORTHO'
views=[('01-overview',(10,13,11),(0,2,.6),12.8),('02-side',(9,-12,5.8),(0,1.5,.7),12.4),('03-top',(0,2,18),(0,2,0),11.6),('04-damage',(-2.8,-3.4,4.8),(-1,.25,1.35),3.9)]
out=Path(a.out);out.mkdir(parents=True,exist_ok=True)
for name,loc,target,scale in views:
 cam.location=loc;cam.rotation_euler=(Vector(target)-cam.location).to_track_quat('-Z','Y').to_euler();data.ortho_scale=scale;scene.render.filepath=str(out/(name+'.png'));bpy.ops.render.render(write_still=True)
(out/'preview.json').write_text(json.dumps(dict(glb=a.glb,read_only=True,blender=bpy.app.version_string,asset_modifiers=0,meshes=sum(o.type=='MESH' for o in assets),views=views,clay=a.clay),indent=2))
