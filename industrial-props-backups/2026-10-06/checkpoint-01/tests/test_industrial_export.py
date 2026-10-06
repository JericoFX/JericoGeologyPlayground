import unittest,tempfile,pathlib,json,sys,math
try:import bpy
except ImportError:bpy=None
class IndustrialRoutingTests(unittest.TestCase):
 def test_recipes_reach_industrial_generator(self):
  from recipe_router import build_recipe
  for preset in ['industrial_solid','industrial_vented','industrial_window']:
   s=build_recipe({'preset':preset})
   self.assertEqual(s['metadata']['preset'],preset)
 def test_all_supported_boundary_dimensions(self):
  from industrial import build_industrial
  from core import validate_scene
  for preset in ['industrial_vented','industrial_window']:
   for w,h,t in [(.65,.8,.028),(.65,3.2,.12),(2.4,.8,.028),(2.4,3.2,.12)]:
    for state in (['broken'] if preset=='industrial_window' else ['intact']):
     s=build_industrial({'preset':preset,'width':w,'height':h,'thickness':t,'glazing':state})
     self.assertEqual(validate_scene(s),[],(preset,w,h,t))
@unittest.skipIf(bpy is None,'Blender required')
class IndustrialExportTests(unittest.TestCase):
 def test_actual_metal_pbr_glb_obj_roundtrip(self):
  from industrial import build_industrial
  from export_assets import export_asset,read_glb_json
  s=build_industrial({'preset':'industrial_solid','angle':57})
  with tempfile.TemporaryDirectory() as d:
   r=export_asset(s,d,'industrial_test',texture_size=128,bake_samples=1)
   self.assertTrue(r['roundtrip']['passed'])
   from portable_validation import validate_glb_geometry
   actual_geometry=validate_glb_geometry(r['glb']);self.assertTrue(actual_geometry['passed'],actual_geometry)
   g=read_glb_json(r['glb'])
   self.assertEqual(g['asset']['version'],'2.0')
   self.assertTrue(g['images'])
   self.assertTrue(g['textures'])
   for mesh in g['meshes']:
    for p in mesh['primitives']:
     self.assertIn('NORMAL',p['attributes']);self.assertIn('TEXCOORD_0',p['attributes']);self.assertIn('TANGENT',p['attributes'])
   data=json.loads(pathlib.Path(r['metadata']).read_text())
   self.assertEqual(data['sockets'],s['sockets']);self.assertEqual(data['grips'],s['grips'])
   self.assertIn('vn ',pathlib.Path(r['obj']).read_text())
 def test_extreme_aspect_ratios_export_valid_tangent_frames(self):
  from industrial import build_industrial
  from export_assets import export_asset
  for preset,w,h in [('industrial_vented',2.4,.8),('industrial_vented',2.4,3.2),('industrial_window',.65,3.2)]:
   s=build_industrial({'preset':preset,'width':w,'height':h,'glazing':'broken' if preset=='industrial_window' else 'intact'})
   with tempfile.TemporaryDirectory() as d:
    r=export_asset(s,d,'extreme',texture_size=128,bake_samples=1)
    self.assertTrue(r['payload_validation']['passed'],r['payload_validation'])
    self.assertEqual(r['payload_validation']['zero_tangent_count'],0)
    self.assertTrue(r['roundtrip']['passed'])
