"""Offline parity oracle plus native-output preview; not a production generator.

Reference mode alone executes frozen Python geometry/shaders/modifiers. Native
mode imports only already-generated vertices, UVs, normals and C++ textures.
"""
import argparse,json,sys,math
from pathlib import Path
ROOT=Path(__file__).resolve().parents[1]
import bpy
from mathutils import Vector

def material(name,color,rough=.78):
 m=bpy.data.materials.new(name);m.use_nodes=True;bs=m.node_tree.nodes.get('Principled BSDF');bs.inputs['Base Color'].default_value=(*color,1);bs.inputs['Roughness'].default_value=rough;return m

def aim(o,p):o.rotation_euler=(Vector(p)-o.location).to_track_quat('-Z','Y').to_euler()
def stage(out,res=640,samples=32):
 s=bpy.context.scene;s.render.engine='CYCLES';s.cycles.samples=samples;s.cycles.use_denoising=False;s.cycles.seed=17;s.render.threads_mode='FIXED';s.render.threads=4;s.render.resolution_x=res;s.render.resolution_y=res;s.render.resolution_percentage=100;s.render.image_settings.file_format='PNG';s.render.filepath=str(out);s.world.color=(.045,.05,.06);s.view_settings.view_transform='AgX';s.view_settings.look='AgX - Medium High Contrast'
 bpy.ops.mesh.primitive_plane_add(size=200,location=(0,0,-.015));bpy.context.object.data.materials.append(material('Stage.floor',(.033,.035,.034),.92))
 for name,loc,power,color,size in [('key',(-3.5,-4.5,6),1550,(1,.82,.63),4),('fill',(3,-2,4),900,(.65,.77,1),3),('rim',(0,3,4.5),1500,(1,.9,.75),3)]:
  ld=bpy.data.lights.new('Stage.'+name,'AREA');ld.energy=power;ld.shape='DISK';ld.size=size;ld.color=color;lo=bpy.data.objects.new('Stage.'+name,ld);bpy.context.collection.objects.link(lo);lo.location=loc;aim(lo,(0,0,1.2))
 cd=bpy.data.cameras.new('Stage.camera');co=bpy.data.objects.new('Stage.camera',cd);bpy.context.collection.objects.link(co);co.location=(.32,-7,1.9);aim(co,(0,0,.86));cd.type='ORTHO';cd.ortho_scale=1.83;cd.lens=50;s.camera=co
 wallmat=material('Stage.rear_wall',(.052,.065,.062),.94);bpy.ops.mesh.primitive_cube_add(size=1,location=(0,.85,1));bpy.context.object.scale=(4,.08,2.4);bpy.context.object.data.materials.append(wallmat)
 bandmat=material('Stage.background_band',(.13,.115,.082),.89)
 for z in [.53,1.13]:
  bpy.ops.mesh.primitive_cube_add(size=1,location=(0,.79,z));bpy.context.object.scale=(4,.007,.045);bpy.context.object.data.materials.append(bandmat)
 return s

def native_material(part,folder):
 texp=part.get('textures')
 if not texp:raise RuntimeError('Native material textures missing: '+part['id'])
 m=bpy.data.materials.new('Native.'+part['id']);m.use_nodes=True;t=m.node_tree;bs=t.nodes.get('Principled BSDF')
 def image(key,noncolor=True):
  node=t.nodes.new('ShaderNodeTexImage');coord=t.nodes.new('ShaderNodeUVMap');coord.uv_map='AtlasUV';t.links.new(coord.outputs['UV'],node.inputs['Vector']);node.image=bpy.data.images.load(str(folder/texp[key]),check_existing=True);node.image.colorspace_settings.name='Non-Color' if noncolor else 'sRGB';node.interpolation='Linear';return node.outputs['Color']
 t.links.new(image('base_color',False),bs.inputs['Base Color'])
 params=t.nodes.new('ShaderNodeSeparateColor');t.links.new(image('material_params'),params.inputs['Color']);
 for output,input in [('Red','Roughness'),('Green','Metallic'),('Blue','Transmission Weight')]:t.links.new(params.outputs[output],bs.inputs[input])
 surf=t.nodes.new('ShaderNodeSeparateColor');t.links.new(image('surface'),surf.inputs['Color']);t.links.new(surf.outputs['Red'],bs.inputs['Coat Weight']);t.links.new(surf.outputs['Green'],bs.inputs['Specular IOR Level'])
 scale=t.nodes.new('ShaderNodeMath');scale.operation='MULTIPLY_ADD';scale.inputs[1].default_value=2;scale.inputs[2].default_value=1;t.links.new(surf.outputs['Blue'],scale.inputs[0]);t.links.new(scale.outputs[0],bs.inputs['IOR'])
 constants=part.get('material_constants',{})
 for key,input in [('ior','IOR'),('specular','Specular IOR Level'),('coat','Coat Weight'),('roughness','Roughness'),('metallic','Metallic'),('transmission','Transmission Weight')]:
  if key in constants:
   for link in list(bs.inputs[input].links):t.links.remove(link)
   bs.inputs[input].default_value=constants[key]
 normal=t.nodes.new('ShaderNodeNormalMap');normal.uv_map='AtlasUV';t.links.new(image('normal'),normal.inputs['Color']);t.links.new(normal.outputs['Normal'],bs.inputs['Normal']);return m

def native_import(data,clay,folder,offset=(0,0,.20)):
 nodes={};solid=material('Native.clay',(.32,.37,.36),.78)
 for n in data['nodes']:
  ob=bpy.data.objects.new('native.'+n['id'],None);bpy.context.collection.objects.link(ob);ob.location=n['translation'];ob.rotation_euler.z=n['rotation_z'];nodes[n['id']]=ob
 for n in data['nodes']:
  if n['parent'] is not None:nodes[n['id']].parent=nodes[n['parent']]
  else:nodes[n['id']].location+=Vector(offset)
 for p in data['parts']:
  me=bpy.data.meshes.new(p['id']);me.from_pydata(p['vertices'],[],p['faces']);me.update();ob=bpy.data.objects.new('native.'+p['id'],me);bpy.context.collection.objects.link(ob);ob.parent=nodes[p['node']]
  uv=me.uv_layers.new(name='MetricUV')
  for poly,coords in zip(me.polygons,p['uvs']):
   for li,co in zip(poly.loop_indices,coords):uv.data[li].uv=co
  if 'texture_uvs' in p:
   uv=me.uv_layers.new(name='AtlasUV');me.uv_layers.active=uv;uv.active_render=True
   for poly,coords in zip(me.polygons,p['texture_uvs']):
    for li,co in zip(poly.loop_indices,coords):uv.data[li].uv=co
  if 'normals' in p:
   for poly in me.polygons:poly.use_smooth=True
   me.normals_split_custom_set([n for face in p['normals'] for n in face])
  me.materials.append(solid if clay else native_material(p,folder))
  assert not ob.modifiers,'Native preview must not generate bevels or geometry'

def main():
 args=sys.argv[sys.argv.index('--')+1:] if '--' in sys.argv else [];p=argparse.ArgumentParser();p.add_argument('--mode',choices=['reference','native'],required=True);p.add_argument('--scene');p.add_argument('--out',required=True);p.add_argument('--clay',action='store_true');p.add_argument('--samples',type=int,default=32);p.add_argument('--resolution',type=int,default=640);o=p.parse_args(args)
 bpy.ops.object.select_all(action='SELECT');bpy.ops.object.delete(use_global=False)
 if o.mode=='native':
  path=Path(o.scene);data=json.loads(path.read_text());native_import(data,o.clay,path.parent)
 else:
  sys.path.insert(0,str(ROOT/'fixtures/sources/JericoGlassFractureStudy'));from glass_fracture import build_fracture;from blender_adapter import import_asset
  recipe=json.loads((ROOT/'fixtures/sources/JericoGlassFractureStudy/recipes/industrial_window_broken.json').read_text());data=build_fracture(recipe);objects=import_asset(data,'reference',(0,0,.20))
  if o.clay:
   solid=material('Reference.clay',(.32,.37,.36),.78)
   for ob in objects.values():
    if ob.type=='MESH':ob.data.materials.clear();ob.data.materials.append(solid)
 s=stage(Path(o.out),o.resolution,o.samples);bpy.ops.render.render(write_still=True)
 Path(o.out).with_suffix('.settings.json').write_text(json.dumps({'mode':o.mode,'camera_location':list(s.camera.location),'camera_rotation':list(s.camera.rotation_euler),'ortho':s.camera.data.ortho_scale,'cycles_seed':s.cycles.seed,'samples':s.cycles.samples,'blender_version':bpy.app.version_string,'native_modifiers':0 if o.mode=='native' else None,'clay':o.clay},indent=2))
if __name__=='__main__':main()
