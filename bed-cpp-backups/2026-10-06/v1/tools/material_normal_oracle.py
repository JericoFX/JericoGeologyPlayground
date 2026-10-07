"""Offline original Cycles Bump oracle, including rotated mesh nodes.
The Position AOV captures the actual primary-ray Object coordinate used by each
sample. PNG/normal production never invokes this script.
"""
import bpy,json,pathlib,sys,math,argparse
from mathutils import Vector
ROOT=pathlib.Path(__file__).resolve().parents[1];sys.path.insert(0,str(ROOT/'tools'))
from material_oracle import original

def main():
 parser=argparse.ArgumentParser();parser.add_argument('--output',default=str(ROOT/'fixtures/material-normal-samples.json'));args=parser.parse_args(sys.argv[sys.argv.index('--')+1:] if '--' in sys.argv else [])
 folder=pathlib.Path('/tmp/jerico-normal-oracle');folder.mkdir(exist_ok=True);samples=[]
 prototypes=[('wood','wood_x',{}),('steel','painted_steel',{'weather':{'seed':37}}),('damp','damp_wood',{'wood_finish':{'family':'old','age':.86,'wetness':.82,'grain_axis':'Z','height_m':1.28}}),('glass','glass',{'glass_study':True,'glass_bounds':[-.591,.591,-.011,1.261]})]
 for family,key,extra in prototypes:
  for angle in (0.,.57,-1.13):
   bpy.ops.object.select_all(action='SELECT');bpy.ops.object.delete(use_global=False)
   pos=[-.571,-.031,.127] if family=='glass' else [-.217,-.031,.427]
   part={'id':'normal_probe','material':key,**extra};case={'id':f'{key}_{angle}','family':'steel' if family=='wood' else family,'part':part,'material':key,'position':pos}
   mat=original(case);tree=mat.node_tree;fixed=tree.nodes[-1];assert fixed.bl_idname=='ShaderNodeCombineXYZ';tex=tree.nodes.new('ShaderNodeTexCoord')
   for link in list(fixed.outputs[0].links):tree.links.new(tex.outputs['Object'],link.to_socket)
   tree.nodes.remove(fixed)
   bs=next(n for n in tree.nodes if n.type=='BSDF_PRINCIPLED');out=next(n for n in tree.nodes if n.type=='OUTPUT_MATERIAL');em=tree.nodes.new('ShaderNodeEmission');scale=tree.nodes.new('ShaderNodeVectorMath');scale.operation='SCALE';scale.inputs['Scale'].default_value=.5;tree.links.new(bs.inputs['Normal'].links[0].from_socket,scale.inputs[0]);add=tree.nodes.new('ShaderNodeVectorMath');add.operation='ADD';add.inputs[1].default_value=(.5,.5,.5);tree.links.new(scale.outputs[0],add.inputs[0]);tree.links.new(add.outputs[0],em.inputs['Color']);tree.links.new(em.outputs[0],out.inputs['Surface'])
   aov=tree.nodes.new('ShaderNodeOutputAOV');aov.name='LocalPosition';aov.aov_name='LocalPosition';tree.links.new(tex.outputs['Object'],aov.inputs['Color'])
   span=.256;half=span/2;px,py,pz=pos;verts=[(px-half,py,pz-half),(px+half,py,pz-half),(px+half,py,pz+half),(px-half,py,pz+half)];mesh=bpy.data.meshes.new('Probe');mesh.from_pydata(verts,[],[(0,1,2,3)]);mesh.update();obj=bpy.data.objects.new('Probe',mesh);bpy.context.collection.objects.link(obj);obj.rotation_euler.z=angle;mesh.materials.append(mat);bpy.context.view_layer.update();rotation=obj.matrix_world.to_3x3();center=obj.matrix_world@Vector(pos);normal=rotation@Vector((0,-1,0))
   camera=bpy.data.cameras.new('Camera');camera.type='ORTHO';camera.ortho_scale=span;cam=bpy.data.objects.new('Camera',camera);bpy.context.collection.objects.link(cam);cam.location=center+normal*2;cam.rotation_euler=(center-cam.location).to_track_quat('-Z','Y').to_euler();scene=bpy.context.scene;scene.camera=cam;scene.render.engine='CYCLES';scene.cycles.samples=1;scene.cycles.seed=17;scene.cycles.use_denoising=False;scene.render.threads_mode='FIXED';scene.render.threads=2;scene.render.resolution_x=scene.render.resolution_y=64;scene.render.resolution_percentage=100;scene.render.filter_size=.01;scene.render.image_settings.file_format='OPEN_EXR';scene.render.image_settings.color_depth='32';scene.render.filepath=str(folder/'normal.exr');scene.world.color=(0,0,0)
   view=scene.view_layers[0]
   while view.aovs:view.aovs.remove(view.aovs[0])
   view.aovs.add().name='LocalPosition';view.aovs[0].type='COLOR';scene.use_nodes=True;nodes=scene.node_tree.nodes;nodes.clear();layers=nodes.new('CompositorNodeRLayers');fileout=nodes.new('CompositorNodeOutputFile');fileout.base_path=str(folder);fileout.format.file_format='OPEN_EXR';fileout.format.color_depth='32';fileout.file_slots[0].path='position_';scene.node_tree.links.new(layers.outputs['LocalPosition'],fileout.inputs[0]);composite=nodes.new('CompositorNodeComposite');scene.node_tree.links.new(layers.outputs['Image'],composite.inputs[0])
   bpy.ops.render.render(write_still=True)
   def pixel(path):
    image=bpy.data.images.load(str(path),check_existing=False);values=list(image.pixels);offset=(32*64+32)*4;v=values[offset:offset+3];bpy.data.images.remove(image);return v
   encoded=pixel(folder/'normal.exr');actual=pixel(folder/'position_0001.exr');world=Vector([2*v-1 for v in encoded]);local=(rotation.transposed()@world).normalized();samples.append({'id':case['id'],'family':case['family'],'part':part,'material':key,'position':actual,'geometric_normal':[0,-1,0],'shading_normal':[0,-1,0],'object_to_world':[list(rotation.col[i]) for i in range(3)],'rotation_z':angle,'footprint_m':float(camera.ortho_scale)/64,'expected_object_normal':list(local),'expected_world_normal':list(world)})
 pathlib.Path(args.output).write_text(json.dumps({'blender':bpy.app.version_string,'method':'Unmodified source Bump evaluated on rotated planes; actual Object position captured by AOV; physical ray footprint equals orthographic span/64','samples':samples},indent=2)+'\n');print('Normal oracle wrote',len(samples),'rotated samples')
if __name__=='__main__':main()
