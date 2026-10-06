import importlib.util,unittest,math,copy,json
from pathlib import Path
from core import validate_scene,asset_hash,clearance_sweep,grip_surface_distance,can_connect,handle_interface

class IndustrialTests(unittest.TestCase):
 def build(self,**r):
  self.assertIsNotNone(importlib.util.find_spec('industrial'),'Industrial procedural generator missing')
  from industrial import build_industrial
  return build_industrial(r)
 def test_solid_dimensions_closed_metal_geometry(self):
  s=self.build(preset='industrial_solid')
  self.assertEqual(validate_scene(s),[])
  self.assertFalse(any(p['material'].startswith('wood') for p in s['parts']))
  self.assertEqual(s['units'],'m')
  self.assertTrue(any(p['id']=='door.sheet' for p in s['parts']))
  self.assertEqual(s['metadata']['generator'],'JericoIndustrialPropsLab/industrial.py')
 def test_parameterized_and_deterministic(self):
  s=self.build(preset='industrial_solid',seed=29)
  self.assertEqual(asset_hash(s),asset_hash(self.build(preset='industrial_solid',seed=29)))
  for w,h,t in [(.65,1.5,.028),(1,2.1,.052),(2.4,3.2,.12)]:
   b=self.build(preset='industrial_solid',width=w,height=h,thickness=t)
   self.assertEqual(validate_scene(b),[])
   opening=next(x for x in b['sockets'] if x['role']=='FrameOpening')
   self.assertEqual(opening['coverage']['opening'],[w,h,t])
   self.assertEqual([p['id'] for p in s['parts']],[p['id'] for p in b['parts']])
 def test_right_hinge_local_axes_and_clearance(self):
  a=self.build(preset='industrial_solid',angle=0)
  b=self.build(preset='industrial_solid',angle=70)
  leaf=next(n for n in b['nodes'] if n['role']=='Leaf')
  self.assertGreater(leaf['translation'][0],0)
  self.assertAlmostEqual(leaf['rotation_z'],math.radians(70))
  self.assertEqual(a['parts'],b['parts'])
  self.assertEqual(clearance_sweep(b,.5)['collisions'],[])
  for sock in b['sockets']:
   if sock['role']=='Hinge':
    self.assertEqual(sock['normal'],[0,0,1])
    self.assertEqual(set(sock['support_ends']),{'leaf','frame'})
    self.assertTrue(sock['enabled'])
 def test_grips_and_handle_part_swap(self):
  for style in ['lever','pull']:
   s=self.build(preset='industrial_solid',handle=style)
   for grip in s['grips']:
    self.assertLess(grip_surface_distance(s,grip),1e-8)
   sock=next(x for x in s['sockets'] if x['role']=='HandleFront')
   self.assertTrue(can_connect(sock,handle_interface('lever')))
   self.assertTrue(can_connect(sock,handle_interface('pull')))
 def test_old_wood_recipe_unchanged(self):
  from presets import build_preset
  r=json.loads(Path('recipes/door_reinforced.json').read_text())
  self.assertEqual(asset_hash(build_preset(r)),'3e0d55be93a08009d3d0a502b1444d328fc7f7b73294a4937475c3e2340a090c')

if __name__=='__main__': unittest.main()

class IndustrialVentedAndWindowTests(unittest.TestCase):
 def build(self,**recipe):
  from industrial import build_industrial
  return build_industrial(recipe)
 def test_vented_has_actual_cut_openings_and_mesh_louvers(self):
  from damage_math import point_in_panel
  s=self.build(preset='industrial_vented')
  self.assertEqual(validate_scene(s),[])
  sheet=next(p for p in s['parts'] if p['id']=='door.sheet')
  for cut in s['metadata']['design_openings']:
   self.assertFalse(point_in_panel(sheet,cut['center']))
  self.assertGreater(len([p for p in s['parts'] if p['role']=='WireMesh']),10)
  self.assertGreater(len([p for p in s['parts'] if p['role']=='Louver']),5)
 def test_window_one_pane_three_states_stable_structure(self):
  states=[self.build(preset='industrial_window',glazing=s) for s in ['intact','broken','empty']]
  for s in states:
   self.assertEqual(validate_scene(s),[])
   self.assertFalse(s['grips'])
   self.assertFalse(any(n['role']=='Leaf' for n in s['nodes']))
  for s in states[1:]:
   self.assertEqual([p for p in states[0]['parts'] if p['material']!='glass'],[p for p in s['parts'] if p['material']!='glass'])
  self.assertEqual([p['id'] for p in states[0]['parts'] if p['material']=='glass'],['window.glass'])
  self.assertEqual([p['id'] for p in states[1]['parts'] if p['material']=='glass'],['window.glass'])
  self.assertFalse([p for p in states[2]['parts'] if p['material']=='glass'])
  self.assertFalse(next(s for s in states[2]['sockets'] if s['role']=='Glass')['enabled'])
 def test_broken_window_real_hole_and_clipped_glass_volume(self):
  from damage_math import point_in_panel
  s=self.build(preset='industrial_window',glazing='broken')
  p=next(p for p in s['parts'] if p['id']=='window.glass')
  hole=s['metadata']['glass_hole']['center_xz']
  self.assertFalse(point_in_panel(p,[hole[0],0,hole[1]]))
  self.assertIn('glass_edge',p['face_materials'])

class LowerVentRevisionTests(unittest.TestCase):
 def test_lower_vent_without_upper_window_and_more_hardware(self):
  from industrial import build_industrial
  s=build_industrial({'preset':'industrial_louvered','finish':'weathered_gray'})
  self.assertEqual(validate_scene(s),[])
  self.assertEqual([x['id'] for x in s['metadata']['design_openings']],['vent'])
  self.assertFalse(any(p['role']=='WireMesh' for p in s['parts']))
  self.assertTrue(any(p['id']=='frame.strike_plate' for p in s['parts']))
  self.assertTrue(any(p['id']=='door.lock.escutcheon' for p in s['parts']))
  self.assertEqual(s['recipe']['finish'],'weathered_gray')
