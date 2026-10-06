"""Original collision-proxy bridge, no dynamics implementation.

The leaf's approved wood meshes become convex hull inputs. Frame timber uses
AABB proxies with exact rectangular CSG subtraction. These are deliberately
labeled proxies, not the high-resolution rendered collision surface.
"""
import json,hashlib,math
APPROVED_HASH='3e0d55be93a08009d3d0a502b1444d328fc7f7b73294a4937475c3e2340a090c'


def load_approved_scene(path):
 with open(path) as stream:scene=json.load(stream)
 actual=hashlib.sha256(json.dumps(scene,sort_keys=True,separators=(',',':'),allow_nan=False).encode()).hexdigest()
 if actual!=APPROVED_HASH:raise ValueError('Approved source fixture hash mismatch')
 return scene


def collision_parts(scene):
 frame=[];leaf=[]
 for part in scene['parts']:
  if not part['material'].startswith('wood'):continue
  if part['node']=='frame' and part['role']=='Frame':
   frame.append({'id':part['id'],'node':'frame','bounds':[[min(v[i] for v in part['vertices']) for i in range(3)],[max(v[i] for v in part['vertices']) for i in range(3)]]})
  if part['node']=='door.leaf':leaf.append(part)
 return frame,leaf


def _validate(box):
 if not isinstance(box,(list,tuple)) or len(box)!=2 or any(len(v)!=3 for v in box):raise ValueError('Box requires min/max XYZ')
 if not all(isinstance(x,(float,int)) and not isinstance(x,bool) and math.isfinite(x) for v in box for x in v):raise ValueError('Box coordinates must be finite')
 if any(box[1][i]<=box[0][i] for i in range(3)):raise ValueError('Box must have positive extent')


def box_volume(box):
 return math.prod(box[1][i]-box[0][i] for i in range(3))


def subtract_box(box,cut):
 """Disjoint six-slab decomposition of an axis-aligned box minus a box."""
 _validate(box);_validate(cut)
 low=[max(box[0][i],cut[0][i]) for i in range(3)];high=[min(box[1][i],cut[1][i]) for i in range(3)]
 if any(high[i]<=low[i] for i in range(3)):return [[list(box[0]),list(box[1])]]
 current=[list(box[0]),list(box[1])];pieces=[]
 for axis in range(3):
  if current[0][axis]<low[axis]:
   part=[current[0][:],current[1][:]];part[1][axis]=low[axis];pieces.append(part);current[0][axis]=low[axis]
  if high[axis]<current[1][axis]:
   part=[current[0][:],current[1][:]];part[0][axis]=high[axis];pieces.append(part);current[1][axis]=high[axis]
 return pieces


def apply_cuts(boxes,cuts,max_pieces=256):
 result=boxes
 for cut in cuts:
  result=[part for box in result for part in subtract_box(box,cut)]
  if len(result)>max_pieces:raise ValueError('Collision-proxy fragment budget exceeded')
 return result


def build_engine_fixture(scene,states_directory):
 from pathlib import Path
 directory=Path(states_directory)
 snapshots=[json.loads((directory/(name+'.json')).read_text()) for name in ['initial','one_support','zero_support']]
 for state in snapshots:
  if state['source_scene_hash']!=APPROVED_HASH:raise ValueError('Support fixture/source mismatch')
  if state['damage']['removed_source_part_ids']:raise ValueError('This bounded collision bridge does not rebuild explicit part removals')
  if any(c['body']!='frame' or c['kind']!='box' for c in state['damage']['cuts']):raise ValueError('This bounded collision bridge only rebuilds frame box cuts')
 frame,leaf=collision_parts(scene)
 keep=snapshots[1]['articulation']['support_mount_ids']
 if len(keep)!=1:raise ValueError('Bench requires exactly one surviving mount at intermediate stage')
 mount=next(m for m in snapshots[0]['mounts'] if m['id']==keep[0])
 anchor=mount['endpoints'][0]['world_transform']['translation']
 leaf_node=next(n for n in scene['nodes'] if n['id']=='door.leaf')
 stages=[]
 for name,s in zip(['initial','one_support','zero_support'],snapshots):
  cuts=[(c['minimum'],c['maximum']) for c in s['damage']['cuts'] if c['body']=='frame']
  stages.append({'name':name,'support_count':s['articulation']['supports_alive'],'hinge_active':s['articulation']['desired_active'],'support_mount_ids':s['articulation']['support_mount_ids'],'frame_boxes':apply_cuts([p['bounds'] for p in frame],cuts),'cuts':s['damage']['cuts'],'commands':[c['command'] for c in s['planned_commands']]})
 return {'source_asset_hash':APPROVED_HASH,'source_basis':scene['basis'],'fixture_lift_m':1.0,'hinge_mount_id':keep[0],'hinge_point':anchor,'leaf_origin':leaf_node['translation'],'leaf_rotation_z':leaf_node['rotation_z'],'leaf_hulls':[{'id':p['id'],'vertices':p['vertices']} for p in leaf],'stages':stages,'collision_proxy':'Approved wood convex hulls for leaf; frame timber AABBs minus accepted local CSG boxes. Decorative metal excluded from collision.','source_geometry_modified':False}


def write_cpp_header(fixture,path):
 from pathlib import Path
 def v(point):return '{'+','.join(format(float(x),'.17g') for x in point)+'}'
 def q(value):return json.dumps(value)
 lines=['#pragma once','#include <string>','#include <vector>',
  'struct P3 { double x,y,z; };','struct HullInput { std::string id; std::vector<P3> vertices; };',
  'struct BoxInput { P3 minimum,maximum; };','struct StageInput { std::string name; int support_count; bool hinge_active; std::vector<BoxInput> boxes; std::vector<std::string> commands; };',
  'struct FixtureInput { std::string hash; P3 leaf_origin,hinge_point; double lift,rotation_z; std::vector<HullInput> hulls; std::vector<StageInput> stages; };',
  'inline FixtureInput make_fixture() { FixtureInput f;',
  'f.hash='+q(fixture['source_asset_hash'])+';', 'f.leaf_origin='+v(fixture['leaf_origin'])+';', 'f.hinge_point='+v(fixture['hinge_point'])+';',
  'f.lift='+repr(fixture['fixture_lift_m'])+';','f.rotation_z='+repr(fixture['leaf_rotation_z'])+';']
 for h in fixture['leaf_hulls']:lines.append('f.hulls.push_back({'+q(h['id'])+',{'+','.join(v(p) for p in h['vertices'])+'}});')
 for s in fixture['stages']:
  boxes=','.join('{'+v(b[0])+','+v(b[1])+'}' for b in s['frame_boxes'])
  commands=','.join(q(c) for c in s['commands'])
  lines.append('f.stages.push_back({'+q(s['name'])+','+str(s['support_count'])+','+('true' if s['hinge_active'] else 'false')+',{'+boxes+'},{'+commands+'}});')
 lines.append('return f; }')
 Path(path).parent.mkdir(parents=True,exist_ok=True);Path(path).write_text('\n'.join(lines)+'\n')


if __name__=='__main__':
 from pathlib import Path
 root=Path(__file__).resolve().parent
 fixture=build_engine_fixture(load_approved_scene(root/'fixtures/approved_door.json'),root/'out/contract_examples')
 (root/'output').mkdir(parents=True,exist_ok=True)
 (root/'output/physics_fixture.json').write_text(json.dumps(fixture,indent=2)+'\n')
 write_cpp_header(fixture,root/'build/generated_fixture.hpp')
 print(json.dumps({'source_asset_hash':fixture['source_asset_hash'],'supports':[s['support_count'] for s in fixture['stages']],'header':'build/generated_fixture.hpp'}))
