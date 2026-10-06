import json,pathlib,unittest,importlib.util
try:import bpy
except ImportError:bpy=None
@unittest.skipIf(bpy is None,'requires Blender Python')
class BlenderDamageTests(unittest.TestCase):
 def test_real_cut_preserves_outside_vertices_and_closed_mesh(self):
  self.assertIsNotNone(importlib.util.find_spec('blender_damage'),'Real Boolean damage adapter required')
  from blender_adapter import clear_scene,import_asset
  from blender_damage import apply_frame_cuts
  scene=json.loads(pathlib.Path('fixtures/approved_door.json').read_text());clear_scene();objects=import_asset(scene,'test')
  cut={'body':'frame','minimum':[-.570,-.08,.2512],'maximum':[-.505,.08,.3052]}
  reports=apply_frame_cuts(objects,[cut])
  report=next(r for r in reports if r['part_id']=='frame.left')
  self.assertGreater(report['removed_volume_m3'],0)
  self.assertEqual(report['nonmanifold_edges'],0)
  self.assertEqual(report['outside_vertices_missing'],0)
  self.assertEqual(report['zero_area_triangles'],0)
  self.assertGreater(report['interior_faces'],0)
  self.assertTrue(report['finite_uvs'])
 def test_no_cut_is_byte_identical_mesh(self):
  if importlib.util.find_spec('blender_damage') is None:self.fail('Damage adapter required')
  from blender_adapter import clear_scene,import_asset
  from blender_damage import apply_frame_cuts
  scene=json.loads(pathlib.Path('fixtures/approved_door.json').read_text());clear_scene();objects=import_asset(scene,'test')
  obj=objects['frame.left'];before=[tuple(v.co) for v in obj.data.vertices]
  self.assertEqual(apply_frame_cuts(objects,[]),[])
  self.assertEqual(before,[tuple(v.co) for v in obj.data.vertices])
if __name__=='__main__':unittest.main()
