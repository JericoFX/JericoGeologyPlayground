import unittest
try:
 import bpy
except ImportError:
 bpy=None

@unittest.skipIf(bpy is None,'requires Blender Python')
class SinglePaneViewTests(unittest.TestCase):
 def test_single_glazing_has_substantial_transmission(self):
  from blender_adapter import build_materials
  mats=build_materials();bs=mats['glass'].node_tree.nodes['Principled BSDF']
  self.assertGreaterEqual(bs.inputs['Transmission Weight'].default_value,.85)
  self.assertLessEqual(bs.inputs['Roughness'].default_value,.18)

if __name__=='__main__':unittest.main()
