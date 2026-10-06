import unittest, sys, copy, math
from pathlib import Path
sys.path.insert(0,str(Path(__file__).resolve().parents[1]))
from furniture import build_furniture, normalize_recipe, validate_furniture, sweep_clearance, asset_hash

class FurnitureTests(unittest.TestCase):
 def test_presets_have_full_drawer_shells(self):
  for preset,count in [('cabinet',4),('workbench',1)]:
   s=build_furniture({'preset':preset})
   self.assertEqual([],validate_furniture(s))
   ds=[n for n in s['nodes'] if n['role']=='Drawer']
   self.assertEqual(count,len(ds))
   for d in ds:
    roles={p['role'] for p in s['parts'] if p['node']==d['id']}
    self.assertTrue({'DrawerFront','DrawerLeft','DrawerRight','DrawerBack','DrawerBottom','CupHandle'}<=roles)
    guide=next(x for x in s['sockets'] if x['node']==d['id'] and x['role']=='DrawerGuide')
    self.assertEqual([0,-1,0],guide['axis'])
    self.assertGreater(guide['coverage']['travel_m'],0)
 def test_deterministic_and_independent_wear(self):
  a=build_furniture({'seed':5}); b=build_furniture({'seed':5})
  self.assertEqual(asset_hash(a),asset_hash(b))
  c=build_furniture({'seed':6})
  self.assertEqual(a['nodes'],c['nodes'])
  self.assertEqual(a['sockets'],c['sockets'])
  self.assertNotEqual(a['parts'],c['parts'])
 def test_variable_dimensions_and_counts(self):
  for count in [2,4,6]:
   s=build_furniture({'drawer_count':count,'width':.88,'height':1.64,'depth':.62})
   self.assertEqual(count,len(s['metadata']['drawers']))
   self.assertEqual([],validate_furniture(s))
   self.assertTrue(sweep_clearance(s)['passed'])
 def test_optional_bench(self):
  s=build_furniture({'preset':'workbench','drawer':False,'shelf':False})
  self.assertEqual([],s['metadata']['drawers'])
  self.assertFalse(any(p['role']=='ShelfPlank' for p in s['parts']))
 def test_open_pose_is_rigid_only(self):
  a=build_furniture({}); b=build_furniture({'open_fraction':.45})
  self.assertEqual(a['parts'],b['parts'])
  d=a['metadata']['drawers'][-2]; n=next(n for n in b['nodes'] if n['id']==d['node'])
  self.assertAlmostEqual(n['translation'][1],d['closed_origin'][1]-.45*d['travel_m'])
 def test_invalid_recipes(self):
  for r in [{'width':float('nan')},{'drawer_count':0},{'drawer_count':True},{'depth':.1},{'open_fraction':2},{'preset':'bed'},{'surprise':1},{'height':.5,'drawer_count':8}]:
   with self.assertRaises((ValueError,TypeError)):normalize_recipe(r)
 def test_motion_sweep_and_mounts(self):
  for p in ['cabinet','workbench']:
   s=build_furniture({'preset':p,'open_fraction':.7})
   self.assertTrue(sweep_clearance(s)['passed'])
   self.assertGreater(sweep_clearance(s)['samples'],2)
   for g in s['grips']:
    self.assertIn(g['id'],s['metadata']['grip_supports'])
 def test_detail_is_geometry_with_face_materials(self):
  s=build_furniture({})
  self.assertTrue(any(p.get('groove_depth_m',0)>0 for p in s['parts']))
  self.assertTrue(any(len(set(p.get('face_materials',[])))>1 for p in s['parts']))
  self.assertTrue(any(p['role']=='LabelFrame' for p in s['parts']))

if __name__=='__main__':unittest.main()

class SweptEnvelopeTests(unittest.TestCase):
 def test_thin_obstruction_between_old_sample_positions_is_caught(self):
  s=build_furniture({});dr=s['metadata']['drawers'][0];o=dr['closed_origin'];w=dr['outer_hi'][0]-dr['outer_lo'][0]
  s['metadata']['static_envelopes'].append(dict(id='thin_blocker',lo=[o[0]-w/4,o[1]-.08,o[2]+.02],hi=[o[0]+w/4,o[1]-.0799,o[2]+.03]))
  self.assertFalse(sweep_clearance(s)['passed'])
 def test_runner_in_drawer_interior_is_not_silently_ignored(self):
  s=build_furniture({});dr=s['metadata']['drawers'][0];o=dr['closed_origin']
  s['metadata']['static_envelopes'].append(dict(id='runner.injected',lo=[o[0]-.30,o[1]+.02,o[2]+.03],hi=[o[0]+.30,o[1]+.3,o[2]+.05]))
  self.assertFalse(sweep_clearance(s)['passed'])
 def test_extreme_valid_dimensions(self):
  for recipe in [dict(width=.55,depth=.4,height=.65,drawer_count=2),dict(width=2.4,depth=.95,height=2.1,drawer_count=8),dict(preset='workbench',width=.55,depth=.4,height=.65),dict(preset='workbench',width=2.4,depth=.95,height=2.1)]:
   self.assertEqual([],validate_furniture(build_furniture(recipe)))

class BoardBoundsTests(unittest.TestCase):
 def test_long_axis_reorientation_preserves_requested_bounds(self):
  for preset in ['cabinet','workbench']:
   for p in build_furniture({'preset':preset})['parts']:
    if 'bounds' not in p:continue
    for v in p['vertices']:
     self.assertTrue(all(p['bounds'][0][i]-1e-8<=v[i]<=p['bounds'][1][i]+1e-8 for i in range(3)),p['id'])

class SupportRegressionTests(unittest.TestCase):
 def test_single_drawer_can_open(self):
  s=build_furniture({'drawer_count':1,'open_fraction':1});dr=s['metadata']['drawers'][0]
  n=next(n for n in s['nodes'] if n['id']==dr['node'])
  self.assertAlmostEqual(n['translation'][1],dr['closed_origin'][1]-dr['travel_m'])
 def test_bench_runners_have_positive_bearing_width(self):
  s=build_furniture({'preset':'workbench'})
  self.assertIsNotNone(sweep_clearance(s)['runner_gap_m'])
  self.assertAlmostEqual(sweep_clearance(s)['runner_gap_m'],.003)
 def test_shelf_has_end_bearings(self):
  s=build_furniture({'preset':'workbench'});parts={p['id']:p for p in s['parts']}
  for p in [p for p in s['parts'] if p['role']=='ShelfPlank']:
   self.assertTrue(all('shelf.cross.'+str(sx) in parts for sx in [-1,1]))
 def test_front_hardware_mounts_reach_front(self):
  s=build_furniture({});ps={p['id']:p for p in s['parts']}
  for d in s['metadata']['drawers']:
   y=min(v[1] for v in ps[d['node']+'.front']['vertices'])
   for role in ['HandleMount','LabelFrame','KeyEscutcheon']:
    for p in [p for p in s['parts'] if p['node']==d['node'] and p['role']==role]:
     self.assertGreaterEqual(max(v[1] for v in p['vertices']),y-1e-8,p['id'])
 def test_joinery_stays_on_short_fronts(self):
  s=build_furniture({'height':1.42,'drawer_count':8});ps={p['id']:p for p in s['parts']}
  for p in [p for p in s['parts'] if p['role']=='JoineryPlug']:
   top=max(v[2] for v in ps[p['node']+'.front']['vertices'])
   self.assertLessEqual(max(v[2] for v in p['vertices']),top)

class DirectedDetailTests(unittest.TestCase):
 def test_tops_have_visible_real_seams_and_local_splinter(self):
  for preset in ['cabinet','workbench']:
   s=build_furniture({'preset':preset});tops=sorted([p for p in s['parts'] if p['role']=='TopPlank'],key=lambda p:p['bounds'][0][1])
   for a,b in zip(tops,tops[1:]):self.assertGreaterEqual(b['bounds'][0][1]-a['bounds'][1][1],.0045)
   self.assertEqual(1,len([p for p in s['parts'] if p['role']=='AttachedSplinter']))
   self.assertTrue(any(p['role']=='TopFixing' for p in s['parts']))
 def test_cabinet_side_joinery_is_geometry(self):
  self.assertEqual(4,len([p for p in build_furniture({})['parts'] if p['role']=='SideRail']))
