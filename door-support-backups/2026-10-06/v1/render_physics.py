"""Render only recorded Jolt poses; no interpolated or hand-authored motion."""
import argparse,csv,json,sys,hashlib
from pathlib import Path
ROOT=Path(__file__).resolve().parent;sys.path.insert(0,str(ROOT))
import bpy
from mathutils import Matrix,Vector,Quaternion
from blender_adapter import clear_scene,import_asset,setup_stage
from blender_damage import apply_frame_cuts
from evidence import freeze_file,freeze_manifest


def setup(out,video=False):
 scene_data=json.loads((ROOT/'fixtures/approved_door.json').read_text())
 clear_scene();objects=import_asset(scene_data,'door',(0,0,1.0))
 scene=setup_stage(camera=(3.0,-6.6,3.25),target=(0,-.12,1.65),ortho=4.15,resolution=(480,640) if video else (800,1050),samples=8 if video else 40)
 scene.render.threads=3;scene.cycles.use_adaptive_sampling=True
 bpy.data.objects['Stage.floor'].location.z=0
 # Visual bench uprights. They denote the externally fixed frame, not new props.
 mat=bpy.data.materials['Mine.iron']
 for x in [-.90,.90]:
  bpy.ops.mesh.primitive_cube_add(size=1,location=(x,.20,1.76));o=bpy.context.object;o.dimensions=(.045,.065,3.52);o.name='Stage.fixed_test_stand';o.data.materials.append(mat)
 bpy.ops.mesh.primitive_cube_add(size=1,location=(0,.20,3.5));bpy.context.object.dimensions=(1.845,.065,.045);bpy.context.object.data.materials.append(mat)
 text_mat=bpy.data.materials.new('Evidence.caption');text_mat.use_nodes=True
 nt=text_mat.node_tree;nt.nodes.clear();em=nt.nodes.new('ShaderNodeEmission');em.inputs['Color'].default_value=(.8,.78,.71,1);outnode=nt.nodes.new('ShaderNodeOutputMaterial');nt.links.new(em.outputs[0],outnode.inputs['Surface'])
 captions=[]
 for name,size,y in [('status',.115,1.83),('source',.058,1.66)]:
  cu=bpy.data.curves.new(name,'FONT');cu.align_x='CENTER';cu.size=size;cu.materials.append(text_mat)
  obj=bpy.data.objects.new('Evidence.'+name,cu);bpy.context.collection.objects.link(obj);obj.parent=scene.camera;obj.location=(0,y,-4);captions.append(cu)
 captions[1].body='JOLT 5.6  /  BANCO ELEVADO 1 m'
 return objects,scene,captions


def pose_leaf(objects,row):
 q=Quaternion((float(row['qw']),float(row['qx']),float(row['qy']),float(row['qz'])))
 objects['door.leaf'].matrix_world=Matrix.Translation(Vector([float(row[k]) for k in ['x','y','z']]))@q.to_matrix().to_4x4()
 bpy.context.view_layer.update()


def main():
 ap=argparse.ArgumentParser();ap.add_argument('--video',action='store_true');ap.add_argument('--out',default=str(ROOT/'output/evidence'));a=ap.parse_args(sys.argv[sys.argv.index('--')+1:] if '--' in sys.argv else [])
 out=Path(a.out).resolve();out.mkdir(parents=True,exist_ok=True)
 with (ROOT/'output/physics/jolt_poses.csv').open() as f:rows=list(csv.DictReader(f))
 snapshots={name:json.loads((ROOT/'out/contract_examples'/(name+'.json')).read_text()) for name in ['initial','one_support','zero_support']}
 objects,scene,captions=setup(out,a.video);applied=set();records=[];geometry_reports=[]
 steps=list(range(0,601,10)) if a.video else [110,230,600]
 if a.video:(out/'video_frames').mkdir(exist_ok=True)
 for index,step in enumerate(steps):
  row=rows[step];state=snapshots[row['stage']]
  new_cuts=[c for c in state['damage']['cuts'] if c['event_id'] not in applied]
  if new_cuts:
   reports=apply_frame_cuts(objects,new_cuts)
   for r in reports:
    if r['nonmanifold_edges'] or r['outside_vertices_missing'] or r['zero_area_triangles'] or not r['finite_uvs']:raise ValueError('Invalid cut mesh: '+str(r))
   geometry_reports.extend(reports);applied.update(c['event_id'] for c in new_cuts)
  for mount in state['mounts']:
   if not mount['supported']:
    prefix=mount['id'].removesuffix('.socket')
    for pid,obj in objects.items():
     if pid.startswith(prefix+'.barrel.') or pid==prefix+'.frame_plate':obj.hide_render=True
  pose_leaf(objects,row)
  count=int(row['supports']);captions[0].body=('3 APOYOS' if count==3 else '1 APOYO / SIGUE UNIDA' if count==1 else '0 APOYOS / LIBRE')
  label={110:'jolt-three-supports',230:'jolt-one-support',600:'jolt-fallen'}.get(step,'frame')
  path=(out/'video_frames'/f'{index:04d}.png') if a.video else (out/(label+'-working.png'))
  scene.render.filepath=str(path);bpy.ops.render.render(write_still=True)
  if not a.video:
   record=freeze_file(path,label);record.update(physics_step=step,physics_time_s=float(row['time']),supports=count,source_csv_sha256=hashlib.sha256((ROOT/'output/physics/jolt_poses.csv').read_bytes()).hexdigest(),pose_source='Unmodified Jolt BodyInterface.GetPosition/GetRotation samples');records.append(record)
   print('FROZEN_RENDER '+json.dumps(record),flush=True)
 if not a.video:
  blend=out/'jolt-final-working.blend';bpy.ops.wm.save_as_mainfile(filepath=str(blend));scene_record=freeze_file(blend,'jolt-damaged-scene')
  result={'stills':records,'geometry_checks':geometry_reports,'scene':scene_record,'physics_report':json.loads((ROOT/'output/physics/jolt_report.json').read_text())}
  p=out/'jolt-evidence-working.json';p.write_text(json.dumps(result,indent=2));print('FROZEN_MANIFEST '+json.dumps(freeze_file(p,'jolt-evidence')),flush=True)
 else:
  print('VIDEO_FRAMES_READY '+str(out/'video_frames'),flush=True)

if __name__=='__main__':main()
