"""Offline test oracle only. Evaluate original Blender graphs, never production assets.
Usage: blender -b -t 2 --python tools/material_oracle.py -- --output fixtures/material-samples.json
"""
import argparse, bpy, json, math, pathlib, random, sys, importlib, hashlib
ROOT=pathlib.Path(__file__).resolve().parents[1]
SOURCES=ROOT/'fixtures/sources'
paths={'glass':SOURCES/'JericoGlassFractureStudy','steel':SOURCES/'JericoIndustrialPropsMaterialsR2Checkpoint_20261006_1927','fine':SOURCES/'JericoIndustrialPropsFineWearCheckpoint_20261006_1921','damp':SOURCES/'JericoDampWoodWindowsDirectedCheckpoint_20261006_1949','furniture':SOURCES/'JericoWoodFurnitureCheckpoint_20261006_1956','furniture_1951':SOURCES/'JericoWoodFurnitureCheckpoint_20261006_1951'}
def load(family):
 for key in ('blender_adapter','industrial_materials','glass_materials','wood_materials','fine_wear_adapter'):
  sys.modules.pop(key,None)
 sys.path.insert(0,str(paths[family]));return importlib.import_module('blender_adapter')
def original(case):
 p=case['part'];key=case['material'];family=case['family'];a=load('furniture_1951' if family=='furniture' and p.get('_material_revision')=='1951' else family)
 if p.get('glass_study'):m=importlib.import_module('glass_materials').glass_material(p,key=='glass_edge')
 elif key in ('painted_steel','dark_steel'):
  m=importlib.import_module('industrial_materials').steel_material(p,p.get('_surface_side'))
  if family=='fine':m=importlib.import_module('fine_wear_adapter').apply_fine_wear(p,p.get('_surface_side'),m)
 elif key in ('cut_metal','glass_edge'):m=importlib.import_module('industrial_materials').cut_material(key)
 elif p.get('wood_finish'):m=importlib.import_module('wood_materials').wood_material(p,key=='wood_exposed')
 else:
  mats=a.build_materials();m=mats[key]
  if family=='furniture' and p.get('grain_axis'):
   m=m.copy();seed=int(hashlib.sha256(p['id'].encode()).hexdigest()[:8],16)
   shift=((seed%173)*.137,((seed//173)%113)*.153,((seed//193)%149)*.127)
   for coordinate in [n for n in m.node_tree.nodes if n.type=='TEX_COORD']:
    old=list(coordinate.outputs['Object'].links);add=a._node(m.node_tree,'ShaderNodeVectorMath',operation='ADD');add.inputs[1].default_value=shift
    m.node_tree.links.new(coordinate.outputs['Object'],add.inputs[0])
    for link in old:m.node_tree.links.new(add.outputs['Vector'],link.to_socket)
 
 if key in ('iron','rust') and p.get('edge_wear') and family in ('steel','fine'):
  m=importlib.import_module('industrial_materials').directed_hardware_material(p,p.get('_surface_side'),m)
  if family=='fine':m=importlib.import_module('fine_wear_adapter').apply_fine_wear(p,p.get('_surface_side'),m)
 m=m.copy();m.name=case['id'];t=m.node_tree
 vector=a._node(t,'ShaderNodeCombineXYZ')
 for i,x in enumerate(case['position']):vector.inputs[i].default_value=x
 for n in list(t.nodes):
  if n.type=='TEX_COORD':
   for link in list(n.outputs['Object'].links):t.links.new(vector.outputs[0],link.to_socket)
  if n.type=='NEW_GEOMETRY':
   for link in list(n.outputs['Position'].links):t.links.new(vector.outputs[0],link.to_socket)
 return m

def cases():
 result=[]
 def add(family,key,p,pts):
  for i,point in enumerate(pts):result.append(dict(id=f'{family}_{key}_{len(result)}',family=family,part=dict(id='part',material=key,**p),material=key,position=point))
 for bounds in ((-.591,.591,-.011,1.261),(-.8,.8,-.01,1.7)):
  x0,x1,z0,z1=bounds;p=dict(glass_study=True,glass_bounds=bounds)
  rng=random.Random(203);ax=rng.uniform(x0+.04,x1-.04);az=rng.uniform(z0+.04,z1-.04);dx=rng.uniform(-.025,.025);dz=rng.uniform(.015,.075)
  add('glass','glass',p,[(0,-.003,.7),(x0+.004,-.003,z0+.02),(x1-.042,.004,z1-.041),(ax+dx*.4,0,az+dz*.4),(ax+dx*.4+.00017,0,az+dz*.4)])
  add('glass','glass_edge',p,[(.1,0,.4)])
 for seed in (29,1,37,1007,2**32+1,2**63-1,-2**63):
  for key in ('painted_steel','dark_steel'):
   add('steel',key,dict(weather=dict(seed=seed,bounds=[-.8,.8,-.03,.03,0,2.1],finish='weathered_gray')),[(.151,-.03,1.123),(-.793,-.03,.03),(.3,.03,.18)])
 add('steel','painted_steel',dict(weather=dict(seed=29,bounds=[-.592,.592,-.051,.059,-.082,-.012],bottom_height_m=.3375)),[(.22606250000047515,-.051,-.05956249999998782)])
 wear=dict(side='front',boundary_segments_xz=[[[-.8,0],[-.8,2.1]]],region=dict(shape='box',min=[-.9,-.1,0],max=[0,.1,2.1]),width_m=.015,feather_m=.015,amount=.87,protected_regions=[])
 for family in ('steel','fine'):
  for side in ('front','back'):
   w=dict(wear,fine_breakup=family=='fine');add(family,'painted_steel',dict(weather=dict(seed=47),_surface_side=side,edge_wear=[w]),[(-.799,-.03,.25),(-.79,-.03,1.1),(-.76,-.03,.9)])
 # R2 preserves arbitrary fine_breakup metadata; its original adapter ignores
 # it. Family is explicit provenance, never inferred from that optional flag.
 for key in ('painted_steel','dark_steel','iron','rust'):
  for flag in (True,'ignored in R2'):
   add('steel',key,dict(weather=dict(seed=47),_surface_side='front',edge_wear=[dict(wear,fine_breakup=flag)]),[(-.799,-.03,.25),(-.79,-.03,1.1)])
 for key in ('iron','rust'):
  for side in ('front','back'):
   add('fine',key,dict(_surface_side=side,edge_wear=[dict(wear,fine_breakup=True)]),[(-.799,-.03,.25),(-.79,-.03,1.1)])
 for key in ('cut_metal','glass_edge','wood','wood_x','wood_rough','wood_rough_x','iron','rust','grime'):
  add('steel',key,{},[(.121,-.038,.092),(-.319,.024,1.211)])
 for age,wet in ((0,0),(1,.8),(.4,.5)):
  for axis in ('X','Z'):
   p=dict(wood_finish=dict(family='test',age=age,wetness=wet,grain_axis=axis,height_m=1.8))
   for key in ('wood','wood_exposed'):add('damp',key,p,[(.123,-.042,.11),(-.451,.031,1.75)])
 for axis in range(3):
  vector=[0,0,0];vector[axis]=1
  add('furniture','wood_'+str(axis),dict(grain_axis=vector),[(.147,-.028,.095),(-.451,.123,1.3)])
 for axis in range(3):
  vector=[0,0,0];vector[axis]=1
  add('furniture','wood_'+str(axis),dict(grain_axis=vector,_material_revision='1951'),[(.147,-.028,.095),(-.451,.123,1.3)])
 for key in ('wood_edge','iron','rust','paper','grime'):add('furniture',key,{},[(.113,-.042,.17)])
 real_damp=json.loads((ROOT/'fixtures/damp/old_intact.json').read_text())
 frame=next(p for p in real_damp['parts'] if p['id']=='frame.left')
 result.append(dict(id='damp_real_frame_left',family='damp',part=frame,material=frame['material'],position=[-.62,-.03,.12]))
 return result

def channel_material(base,row):
 m=base.copy();t=m.node_tree;bs=next(n for n in t.nodes if n.type=='BSDF_PRINCIPLED');bumps=[n for n in t.nodes if n.type=='BUMP']
 bump=bumps[-1] if bumps else None
 output=next(n for n in t.nodes if n.type=='OUTPUT_MATERIAL');em=t.nodes.new('ShaderNodeEmission');t.links.new(em.outputs[0],output.inputs['Surface'])
 if row==0:
  s=bs.inputs['Base Color']
  if s.is_linked:t.links.new(s.links[0].from_socket,em.inputs[0])
  else:em.inputs[0].default_value=s.default_value
 else:
  rgb=t.nodes.new('ShaderNodeCombineColor');rgb.mode='RGB'
  inputs={1:[bs.inputs['Roughness'],bs.inputs['Metallic'],bs.inputs['Transmission Weight']],2:[bs.inputs['IOR'],bs.inputs['Coat Weight'],bump.inputs['Height'] if bump else None],3:[bump.inputs['Strength'] if bump else None,bump.inputs['Distance'] if bump else None,bs.inputs['Specular IOR Level']]}[row]
  for i,s in enumerate(inputs):
   if s is None:rgb.inputs[i].default_value=0
   elif s.is_linked:t.links.new(s.links[0].from_socket,rgb.inputs[i])
   else:rgb.inputs[i].default_value=s.default_value
  t.links.new(rgb.outputs[0],em.inputs[0])
 return m

def main():
 parser=argparse.ArgumentParser();parser.add_argument('--output',default=str(ROOT/'fixtures/material-samples.json'));parser.add_argument('--bake-report');args=parser.parse_args(sys.argv[sys.argv.index('--')+1:] if '--' in sys.argv else [])
 bpy.ops.object.select_all(action='SELECT');bpy.ops.object.delete(use_global=False)
 testcases=json.loads(pathlib.Path(args.bake_report).read_text())['probes'] if args.bake_report else cases()
 n=len(testcases);vertices=[];faces=[];materials=[]
 for x,case in enumerate(testcases):
  mat=original(case)
  for y in range(4):
   start=len(vertices);vertices.extend([(x,y,0),(x+1,y,0),(x+1,y+1,0),(x,y+1,0)]);faces.append(tuple(range(start,start+4)));materials.append(channel_material(mat,y))
 mesh=bpy.data.meshes.new('Oracle cells');mesh.from_pydata(vertices,[],faces);mesh.update();obj=bpy.data.objects.new('Offline oracle',mesh);bpy.context.collection.objects.link(obj)
 for i,m in enumerate(materials):mesh.materials.append(m);mesh.polygons[i].material_index=i
 scene=bpy.context.scene;camera=bpy.data.cameras.new('Camera');cam=bpy.data.objects.new('Camera',camera);bpy.context.collection.objects.link(cam);cam.location=(n/2,2,10);camera.type='ORTHO';camera.ortho_scale=max(n,4);scene.camera=cam
 scene.render.engine='CYCLES';scene.cycles.samples=1;scene.cycles.use_denoising=False;scene.render.threads_mode='FIXED';scene.render.threads=2;scene.render.resolution_x=n*8;scene.render.resolution_y=32;scene.render.resolution_percentage=100;scene.render.filter_size=.01
 scene.world.color=(0,0,0);scene.view_settings.view_transform='Standard';scene.render.image_settings.file_format='OPEN_EXR';scene.render.image_settings.color_depth='32';scene.render.filepath=str(pathlib.Path(args.output).with_suffix('.exr'))
 bpy.ops.render.render(write_still=True);im=bpy.data.images.load(scene.render.filepath);pixels=list(im.pixels);width=n*8
 for x,c in enumerate(testcases):
  rows=[]
  for y in range(4):idx=((y*8+4)*width+x*8+4)*4;rows.append(pixels[idx:idx+3])
  c['expected']=dict(base_color=rows[0],roughness=rows[1][0],metallic=rows[1][1],transmission=rows[1][2],ior=rows[2][0],coat=rows[2][1],height=rows[2][2],bump_strength=rows[3][0],bump_distance=rows[3][1],specular=rows[3][2])
 pathlib.Path(args.output).write_text(json.dumps(dict(blender=bpy.app.version_string,method='Original graphs evaluated by Cycles emission into linear 32-bit EXR; fixed Object coordinates',samples=testcases),indent=2)+'\n')
 print('Material oracle wrote',len(testcases),'samples to',args.output)
if __name__=='__main__':main()
