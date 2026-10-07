import sys,unittest
from pathlib import Path
sys.path.insert(0,str(Path(__file__).resolve().parents[1]))
try:import bpy
except ImportError:bpy=None

@unittest.skipIf(bpy is None,'requires Blender Python')
class BlenderFurnitureTests(unittest.TestCase):
 def setUp(self):
  from blender_adapter import clear_scene
  clear_scene()
 def test_real_face_material_slots_and_bevel_index(self):
  from furniture import build_furniture
  from blender_adapter import import_asset
  s=build_furniture({});obs=import_asset(s)
  for p in s['parts']:
   ob=obs[p['id']]
   if p.get('face_materials'):
    self.assertEqual(2,len(ob.data.materials))
    self.assertEqual(set(p['face_materials']),{'wood','wood_edge'})
    self.assertEqual({0,1},{f.material_index for f in ob.data.polygons})
    bevel=next(x for x in ob.modifiers if x.type=='BEVEL')
    self.assertEqual(1,bevel.material)
 def test_drawer_node_and_metric_uv_survive_adapter(self):
  from furniture import build_furniture
  from blender_adapter import import_asset
  s=build_furniture({'preset':'workbench','open_fraction':.58});obs=import_asset(s)
  for p in s['parts']:
   ob=obs[p['id']]
   self.assertEqual(p['node'],ob.parent['stable_id'])
   self.assertEqual(len(ob.data.loops),len(ob.data.uv_layers['MetricUV'].data))
  self.assertLess(obs['drawer.0'].location.y,-.34)
 def test_closed_open_materials_use_same_seed(self):
  from furniture import build_furniture
  from blender_adapter import import_asset
  a=import_asset(build_furniture({}),'a');b=import_asset(build_furniture({'open_fraction':.5}),'b')
  for key in ['drawer.0.front','drawer.2.front']:
   def offsets(o):return [tuple(n.inputs[1].default_value) for n in o.data.materials[0].node_tree.nodes if n.type=='VECT_MATH' and n.operation=='ADD']
   self.assertEqual(offsets(a[key]),offsets(b[key]))
  self.assertNotEqual(a['drawer.0.front'].data.materials[0],a['drawer.2.front'].data.materials[0])
