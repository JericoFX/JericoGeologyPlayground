"""Run with blender -b --python tests/test_blender.py."""
import sys
import unittest
from pathlib import Path
sys.path.insert(0,str(Path(__file__).resolve().parents[1]))
import importlib.util
try:
    import bpy
except ImportError:
    bpy=None

@unittest.skipIf(bpy is None,"requires Blender Python")
class AdapterTests(unittest.TestCase):
    def test_adapter_exists(self):
        self.assertIsNotNone(importlib.util.find_spec('blender_adapter'),'Blender scene adapter must exist')

    def test_mesh_import_preserves_meter_vertices_and_uv(self):
        if importlib.util.find_spec('blender_adapter') is None:
            self.skipTest('adapter not yet implemented')
        from blender_adapter import import_asset, clear_scene
        clear_scene()
        scene={'nodes':[{'id':'root','parent':None,'translation':[0,0,0],'rotation_z':0,'role':'Frame'}], 'parts':[{'id':'panel','node':'root','material':'wood','vertices':[[0,0,0],[2,0,0],[2,0,1],[0,0,1]],'faces':[[0,1,2,3]],'uvs':[[[0,0],[2,0],[2,1],[0,1]]],'bevel':0,'role':'Leaf'}], 'recipe':{'seed':42},'sockets':[],'grips':[]}
        objects=import_asset(scene,'test')
        obj=objects['panel']
        self.assertEqual(len(obj.data.vertices),4)
        self.assertAlmostEqual(obj.data.vertices[2].co.x,2)
        self.assertAlmostEqual(obj.data.uv_layers.active.data[2].uv.x,2)
        self.assertEqual(obj.parent.name,'test.root')

    def test_materials_are_procedural_and_rough(self):
        if importlib.util.find_spec('blender_adapter') is None:
            self.skipTest('adapter not yet implemented')
        from blender_adapter import build_materials
        mats=build_materials()
        for key in ['wood','wood_x','wood_rough','wood_rough_x','iron','rust','glass','grime']:
            self.assertIn(key,mats)
            self.assertTrue(mats[key].use_nodes)
            self.assertFalse(any(n.type=='TEX_IMAGE' for n in mats[key].node_tree.nodes))
        self.assertGreaterEqual(mats['wood'].node_tree.nodes.get('Principled BSDF').inputs['Roughness'].default_value,.7)
        ramp=next(n for n in mats['wood'].node_tree.nodes if n.type=='VALTORGB')
        self.assertLessEqual(max(max(e.color[:3]) for e in ramp.color_ramp.elements),.10)

    def test_stage_renders_with_available_cycles_build(self):
        from blender_adapter import clear_scene,setup_stage
        clear_scene()
        setup_stage(resolution=(16,16),samples=1)
        try:
            bpy.ops.render.render()
        except RuntimeError as exc:
            self.fail(str(exc))

    def test_import_hierarchy_applies_fixed_pivot(self):
        if importlib.util.find_spec('blender_adapter') is None:
            self.skipTest('adapter not yet implemented')
        from blender_adapter import import_asset,clear_scene
        from math import pi
        from mathutils import Vector
        clear_scene()
        data={'nodes':[{'id':'root','parent':None,'translation':[1,0,0],'rotation_z':0,'role':'Frame'},{'id':'leaf','parent':'root','translation':[0,0,0],'rotation_z':-pi/2,'role':'Leaf'}], 'parts':[], 'recipe':{'seed':42},'sockets':[],'grips':[]}
        objects=import_asset(data,'test')
        bpy.context.view_layer.update()
        p=objects['leaf'].matrix_world@Vector((2,0,1))
        self.assertAlmostEqual(p.x,1,places=5)
        self.assertAlmostEqual(p.y,-2,places=5)

if __name__=='__main__':
    suite=unittest.defaultTestLoader.loadTestsFromTestCase(AdapterTests)
    res=unittest.TextTestRunner(verbosity=2).run(suite)
    if not res.wasSuccessful():
        sys.exit(1)
