import importlib.util,unittest,math
class PhysicsFixtureTests(unittest.TestCase):
 def setUp(self):self.assertIsNotNone(importlib.util.find_spec('physics_fixture'),'Fixture bridge required')
 def test_approved_source_and_body_partition(self):
  import physics_fixture as p
  scene=p.load_approved_scene('fixtures/approved_door.json')
  frame,leaf=p.collision_parts(scene)
  self.assertEqual(len(frame),3)
  self.assertGreaterEqual(len(leaf),10)
  self.assertTrue(all(x['node']=='door.leaf' for x in leaf))
  self.assertTrue(all(x['material'].startswith('wood') for x in leaf))
 def test_box_subtraction_preserves_exact_orthogonal_volume(self):
  from physics_fixture import subtract_box,box_volume
  b=[[-1,-1,-1],[1,1,1]];c=[[-.2,-.2,-2],[.2,.2,2]]
  pieces=subtract_box(b,c)
  self.assertAlmostEqual(sum(box_volume(p) for p in pieces),8-.4*.4*2)
  self.assertEqual(len(pieces),4)
  for p in pieces:self.assertGreater(box_volume(p),0)
 def test_offtarget_cut_is_unchanged_and_complete_cut_is_empty(self):
  from physics_fixture import subtract_box
  b=[[0,0,0],[1,1,1]]
  self.assertEqual(subtract_box(b,[[2,2,2],[3,3,3]]),[b])
  self.assertEqual(subtract_box(b,[[-1,-1,-1],[2,2,2]]),[])
 def test_engine_fixture_states_come_from_support_contract(self):
  import physics_fixture as p
  self.assertTrue(hasattr(p,'build_engine_fixture'),'Engine fixture builder required')
  scene=p.load_approved_scene('fixtures/approved_door.json')
  f=p.build_engine_fixture(scene,'out/contract_examples')
  self.assertEqual([s['support_count'] for s in f['stages']],[3,1,0])
  self.assertEqual([s['hinge_active'] for s in f['stages']],[True,True,False])
  self.assertEqual(f['stages'][2]['commands'],['remove_constraint','activate_body'])
  self.assertIn(f['hinge_mount_id'],f['stages'][1]['support_mount_ids'])
  self.assertLess(sum(p.box_volume(x) for x in f['stages'][2]['frame_boxes']),sum(p.box_volume(x) for x in f['stages'][0]['frame_boxes']))

 def test_bridge_rejects_damage_it_cannot_rebuild(self):
  import physics_fixture as p,json,pathlib,tempfile
  scene=p.load_approved_scene('fixtures/approved_door.json')
  for mutation in ['part_removal','leaf_cut']:
   with tempfile.TemporaryDirectory() as d:
    for name in ['initial','one_support','zero_support']:
     state=json.loads((pathlib.Path('out/contract_examples')/(name+'.json')).read_text())
     if name=='zero_support':
      if mutation=='part_removal':state['damage']['removed_source_part_ids']=['frame.left']
      else:state['damage']['cuts'][0]['body']='door.leaf'
     (pathlib.Path(d)/(name+'.json')).write_text(json.dumps(state))
    with self.assertRaises(ValueError):p.build_engine_fixture(scene,d)

 def test_fixture_cli_creates_output_from_clean_source_extraction(self):
  import pathlib,tempfile,shutil,subprocess,sys
  root=pathlib.Path(__file__).resolve().parents[1]
  with tempfile.TemporaryDirectory() as d:
   clean=pathlib.Path(d)
   for name in ['physics_fixture.py','support_contract.py','generate_contract.py']:
    shutil.copy2(root/name,clean/name)
   (clean/'fixtures').mkdir();shutil.copy2(root/'fixtures/approved_door.json',clean/'fixtures/approved_door.json')
   a=subprocess.run([sys.executable,'generate_contract.py','--output','out/contract_examples'],cwd=clean,capture_output=True,text=True)
   self.assertEqual(a.returncode,0,a.stderr)
   self.assertFalse((clean/'output').exists())
   b=subprocess.run([sys.executable,'physics_fixture.py'],cwd=clean,capture_output=True,text=True)
   self.assertEqual(b.returncode,0,b.stderr)
   self.assertTrue((clean/'output/physics_fixture.json').is_file())
   self.assertTrue((clean/'build/generated_fixture.hpp').is_file())

 def test_invalid_cuts_rejected(self):
  from physics_fixture import subtract_box
  for cut in [[[0,0,0],[0,1,1]],[[float('nan'),0,0],[1,1,1]]]:
   with self.assertRaises(ValueError):subtract_box([[0,0,0],[1,1,1]],cut)
 def test_sequential_cuts_do_not_restore_material(self):
  from physics_fixture import apply_cuts,box_volume
  boxes=[[[0,0,0],[1,1,3]]]
  a=apply_cuts(boxes,[[[-1,-1,.5],[2,2,.7]]])
  b=apply_cuts(boxes,[[[-1,-1,.5],[2,2,.7]],[[-1,-1,1.5],[2,2,1.7]]])
  self.assertAlmostEqual(sum(box_volume(x) for x in a),2.8)
  self.assertAlmostEqual(sum(box_volume(x) for x in b),2.6)

if __name__=='__main__':unittest.main()
