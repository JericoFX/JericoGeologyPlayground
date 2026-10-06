import unittest,json,hashlib,pathlib
from core import validate_scene,asset_hash
from industrial import build_industrial
class DamageEvidenceTests(unittest.TestCase):
 def test_notch_cavity_change_real_volume_and_preserve_unselected_geometry(self):
  recipes=pathlib.Path('recipes')
  self.assertTrue((recipes/'industrial_damage_left.json').exists(),'Missing reviewable damage recipe')
  r=json.loads((recipes/'industrial_damage_left.json').read_text());damaged=build_industrial(r)
  source=build_industrial(dict(r,operations=[]));self.assertEqual(validate_scene(damaged),[])
  a={p['id']:p for p in source['parts']};b={p['id']:p for p in damaged['parts']}
  selected={op['part_id'] for op in r['operations']}
  for id in a:
   if id not in selected:self.assertEqual(a[id],b[id],id)
   else:self.assertEqual(a[id]['vertices'],b[id]['vertices'][:len(a[id]['vertices'])])
  self.assertTrue(all(op['status']=='applied' for op in damaged['metadata']['damage_operations']))
  self.assertTrue(all(s['enabled'] for s in damaged['sockets']))
  self.assertNotEqual(asset_hash(source),asset_hash(damaged))
  p=b['door.sheet'];self.assertIn('cut_metal',p['face_materials'])
  self.assertTrue(p['cuts'][0]['through']);self.assertFalse(p['cuts'][1]['through'])
  y1=a['door.sheet']['panel_bounds'][3]
  # The second, cavity-only cutter stops before back face.
  self.assertLess(p['cuts'][1]['y_range'][1],y1)
