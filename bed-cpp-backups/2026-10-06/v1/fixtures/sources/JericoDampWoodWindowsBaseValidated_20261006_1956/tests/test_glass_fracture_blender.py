import unittest, importlib.util
try:import bpy
except ImportError:bpy=None
@unittest.skipUnless(bpy,'Blender integration only')
class GlassMaterialTests(unittest.TestCase):
 def test_glass_material_has_structured_dust_and_transmission(self):
  self.assertIsNotNone(importlib.util.find_spec('glass_materials'),'Original glass shader is required')
  from glass_materials import glass_material
  mat=glass_material({'glass_bounds':[-.591,.591,-.011,1.261]})
  self.assertTrue(mat.use_nodes)
  self.assertIn('Rebate dust accumulation',mat.node_tree.nodes)
  self.assertIn('Analytic scratch wear',mat.node_tree.nodes)
  self.assertGreater(mat.node_tree.nodes['Principled BSDF'].inputs['Transmission Weight'].default_value,.8)

@unittest.skipUnless(bpy,'Blender integration only')
class GlassExportTests(unittest.TestCase):
 def test_baked_portable_fracture_preserves_glass_and_tangent_frames(self):
  import tempfile,json
  from pathlib import Path
  from glass_fracture import build_fracture
  from export_assets import export_asset,read_glb_json
  from portable_validation import validate_glb_geometry
  recipe=json.loads(Path('recipes/industrial_window_broken.json').read_text());scene=build_fracture(recipe)
  with tempfile.TemporaryDirectory() as d:
   result=export_asset(scene,d,'fracture',128,1)
   self.assertTrue(result['roundtrip']['passed'])
   check=validate_glb_geometry(result['glb']);self.assertTrue(check['passed'],check)
   gltf=read_glb_json(result['glb'])
   glass=[m for m in gltf['materials'] if m.get('extensions',{}).get('KHR_materials_transmission')]
   self.assertTrue(glass)
   self.assertGreater(glass[0]['extensions']['KHR_materials_transmission']['transmissionFactor'],.8)
   for m in gltf['meshes']:
    for p in m['primitives']:
     for k in ['NORMAL','TANGENT','TEXCOORD_0']:self.assertIn(k,p['attributes'])
   metadata=json.loads(Path(result['metadata']).read_text());self.assertEqual(metadata['sockets'],scene['sockets'])

@unittest.skipUnless(bpy,'Blender integration only')
class GlassBakeIndexTests(unittest.TestCase):
 def test_actual_glass_shard_bake_copy_keeps_caps_and_fracture_wall_slots(self):
  import json
  from pathlib import Path
  from glass_fracture import build_fracture
  from blender_adapter import clear_scene,import_asset
  from export_assets import _joined_bake_mesh
  clear_scene();s=build_fracture(json.loads(Path('recipes/industrial_window_broken.json').read_text()));obs=import_asset(s)
  ob=obs['window.glass.shard.00'];expected=[p.material_index for p in ob.data.polygons]
  self.assertEqual(set(expected),{0,1})
  joined=_joined_bake_mesh([ob])
  self.assertEqual([p.material_index for p in joined.data.polygons],expected)
  self.assertEqual([p.material_index for p in ob.data.polygons],expected)
