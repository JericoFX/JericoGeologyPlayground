"""Behavioral regression tests for separate age/wetness and portable wood cuts."""
import copy, importlib.util, math, unittest
from pathlib import Path

class DampWoodTests(unittest.TestCase):
 def module(self):
  self.assertIsNotNone(importlib.util.find_spec('damp_wood'), 'damp wood window generator must exist')
  import damp_wood
  return damp_wood
 def build(self,**kwargs): return self.module().build_damp_window(dict(family='old',glazing='broken',seed=29,**kwargs))
 def test_age_and_wetness_are_independent(self):
  m=self.module()
  dry=m.build_damp_window(dict(family='new',wetness=0,age=.12))
  wet=m.build_damp_window(dict(family='new',wetness=.9,age=.12))
  self.assertEqual([p['vertices'] for p in dry['parts']],[p['vertices'] for p in wet['parts']])
  self.assertEqual(dry['sockets'],wet['sockets'])
  self.assertNotEqual(dry['metadata']['wood_finish'],wet['metadata']['wood_finish'])
 def test_six_states_have_stable_frame_and_interfaces(self):
  m=self.module()
  for family in ('old','new'):
   scenes=[m.build_damp_window(dict(family=family,glazing=s,seed=29)) for s in ('intact','broken','empty')]
   nonglass=lambda s:[p for p in s['parts'] if p['role']!='GlassPane']
   self.assertEqual(nonglass(scenes[0]),nonglass(scenes[1]));self.assertEqual(nonglass(scenes[1]),nonglass(scenes[2]))
   self.assertEqual(len([p for p in scenes[0]['parts'] if p['role']=='GlassPane']),1)
   self.assertGreater(len([p for p in scenes[1]['parts'] if p['role']=='GlassPane']),8)
   self.assertFalse([p for p in scenes[2]['parts'] if p['role']=='GlassPane'])
   self.assertEqual(scenes[0]['grips'],[])
   self.assertFalse([p for s in scenes for p in s['parts'] if 'mullion' in p['id']])
 def test_old_has_real_bounded_cuts_and_new_is_less_damaged(self):
  m=self.module();old=m.build_damp_window(dict(family='old'));new=m.build_damp_window(dict(family='new'))
  def removed(s):return sum(p.get('wood_damage',{}).get('removed_volume_m3',0) for p in s['parts'])
  self.assertGreater(removed(old),removed(new)*5);self.assertGreater(removed(new),0)
  for scene in (old,new):
   for p in scene['parts']:
    if p.get('wood_damage'):
     self.assertGreater(p['cut_math']['remaining_volume'],p['wood_damage']['initial_volume_m3']*.97)
     self.assertNotIn('cut_metal',p['face_materials'])
     self.assertIn('wood_exposed',p['face_materials'])
     self.assertEqual(len(p['faces']),len(p['uvs']))
 def test_wetness_and_age_validation(self):
  m=self.module()
  for key in ('age','wetness'):
   for v in (-.1,1.1,float('nan'),True,'wet'):
    with self.assertRaises(ValueError):m.build_damp_window({key:v})
  with self.assertRaises(ValueError):m.build_damp_window(dict(family='rotten'))
 def test_deterministic_and_no_new_physics(self):
  m=self.module();r=dict(family='old',glazing='broken',seed=29)
  self.assertEqual(m.build_damp_window(r),m.build_damp_window(r))
  self.assertFalse(m.build_damp_window(r)['metadata'].get('simulation',False))
 def test_wood_support_samples_are_untouched(self):
  m=self.module();from damage_math import point_in_panel
  s=m.build_damp_window(dict(family='old')); parts={p['id']:p for p in s['parts']}
  for record in s['metadata']['wood_support_samples']:
   self.assertTrue(point_in_panel(parts[record['part_id']],record['point']))
 def test_closed_oriented_positive_volume(self):
  m=self.module();from damage_math import _bounds
  for family in ('old','new'):
   for p in m.build_damp_window(dict(family=family))['parts']:
    if p.get('wood_damage'):_bounds(p,check_mesh=True)

if __name__=='__main__':unittest.main()

class DampWoodRouteTests(unittest.TestCase):
 def test_existing_json_recipe_route_builds_damp_family(self):
  from recipe_router import build_recipe
  try:scene=build_recipe(dict(preset='damp_wood_single_pane',family='new',wetness=.72,age=.09,glazing='empty'))
  except ValueError as e:self.fail('Damp wood recipes must route through the existing JSON generator: '+str(e))
  self.assertEqual(scene['metadata']['wood_finish'],dict(family='new',wetness=.72,age=.09))

class DampWoodExtremeRejectionTests(unittest.TestCase):
 def test_known_extreme_fracture_rejects_without_clamp_or_input_mutation(self):
  from damp_wood import build_damp_window
  recipe=dict(family='old',glazing='broken',width=.65,height=3.2,seed=29)
  original=copy.deepcopy(recipe)
  with self.assertRaisesRegex(ValueError,'simple|intersect|profile|Profile|triangulat|outline|Cutter'):
   build_damp_window(recipe)
  self.assertEqual(recipe,original)
