import unittest
try:import bpy
except ImportError:bpy=None
@unittest.skipIf(bpy is None,'Blender is required')
class DampWoodBlenderTests(unittest.TestCase):
 def test_wood_and_cut_faces_use_dedicated_nonmetal_materials(self):
  from damp_wood import build_damp_window
  from blender_adapter import clear_scene,import_asset
  clear_scene();data=build_damp_window(dict(family='old'));objects=import_asset(data)
  for part in data['parts']:
   if not part.get('wood_finish'):continue
   obj=objects[part['id']]
   self.assertTrue(all(m.name.startswith('DampWood.') for m in obj.data.materials))
   for poly,key in zip(obj.data.polygons,part.get('face_materials',['damp_wood']*len(part['faces']))):
    name=obj.data.materials[poly.material_index].name
    self.assertEqual('.exposed.' in name,key=='wood_exposed')
    bs=obj.data.materials[poly.material_index].node_tree.nodes.get('Principled BSDF')
    self.assertEqual(bs.inputs['Metallic'].default_value,0)
    self.assertTrue(bs.inputs['Base Color'].is_linked)
    self.assertTrue(bs.inputs['Roughness'].is_linked)
 def test_finish_moisture_reduces_exported_principled_roughness(self):
  from damp_wood import build_damp_window
  from blender_adapter import clear_scene,import_asset
  values=[]
  for wetness in (0,.9):
   clear_scene();o=import_asset(build_damp_window(dict(family='new',wetness=wetness)))['frame.left']
   material=o.data.materials[0]
   self.assertIn('wetness',material)
   values.append(material['roughness_reference'])
  self.assertLess(values[1],values[0]-.2)
 def test_bake_copy_preserves_all_wood_face_material_indices(self):
  from damp_wood import build_damp_window
  from blender_adapter import clear_scene,import_asset
  from export_assets import _joined_bake_mesh
  clear_scene();obj=import_asset(build_damp_window(dict(family='old')))['frame.left']
  expected=[p.material_index for p in obj.data.polygons]
  self.assertEqual(set(expected),{0,1})
  joined=_joined_bake_mesh([obj])
  self.assertEqual([p.material_index for p in joined.data.polygons],expected)
  self.assertEqual([p.material_index for p in obj.data.polygons],expected)
 def test_finish_includes_local_wet_joints_and_fine_grain(self):
  from damp_wood import build_damp_window
  from blender_adapter import clear_scene,import_asset
  clear_scene();obj=import_asset(build_damp_window(dict(family='new')))['frame.left']
  names=obj.data.materials[0].node_tree.nodes
  self.assertIn('Wet joint accumulation',names)
  self.assertIn('Fine lengthwise grain',names)
 def test_fine_old_wood_survives_portable_atlas_without_zero_area_faces(self):
  from damp_wood import build_damp_window
  from blender_adapter import clear_scene,import_asset
  from export_assets import _prepare_atlas
  clear_scene();objects=import_asset(build_damp_window(dict(family='old')))
  meshes=[o for o in objects.values() if o.type=='MESH']
  try:_prepare_atlas(meshes,1024)
  except ValueError as exc:self.fail('Fine timber cuts must remain exportable: '+str(exc))
  for obj in meshes:
   for poly in obj.data.polygons:
    a,b,c=[obj.data.vertices[i].co for i in poly.vertices]
    self.assertGreater((b-a).cross(c-a).length,1e-14)
 def test_fine_old_wood_glb_preserves_valid_binary_tangent_frames(self):
  import tempfile
  from damp_wood import build_damp_window
  from export_assets import export_asset
  from portable_validation import validate_glb_geometry
  with tempfile.TemporaryDirectory() as directory:
   try:result=export_asset(build_damp_window(dict(family='old',glazing='broken')),directory,'old_regression',128,1)
   except ValueError as exc:self.fail('Physical timber checks must have a valid portable tangent basis: '+str(exc))
   self.assertTrue(result['roundtrip']['passed'])
   self.assertTrue(validate_glb_geometry(result['glb'])['passed'])
