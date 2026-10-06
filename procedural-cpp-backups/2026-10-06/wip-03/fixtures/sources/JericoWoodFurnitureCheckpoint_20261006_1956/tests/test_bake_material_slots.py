"""Regression: joining bake copies must retain per-face material assignment."""
import pathlib
import sys
import unittest
sys.path.insert(0, str(pathlib.Path(__file__).resolve().parents[1]))
try:
    import bpy
except ImportError:
    bpy = None

@unittest.skipIf(bpy is None, 'requires Blender Python')
class BakeMaterialSlotsTests(unittest.TestCase):
    def setUp(self):
        from blender_adapter import clear_scene
        clear_scene()
        mesh = bpy.data.meshes.new('Two-face material fixture')
        mesh.from_pydata([(0,0,0),(1,0,0),(1,1,0),(0,1,0),
                          (2,0,0),(3,0,0),(3,1,0),(2,1,0)], [],
                         [(0,1,2,3),(4,5,6,7)])
        mesh.update()
        self.obj = bpy.data.objects.new('Two material source', mesh)
        bpy.context.collection.objects.link(self.obj)
        for name, color in [('Red face',(1,0,0,1)),('Green face',(0,1,0,1))]:
            material = bpy.data.materials.new(name)
            material.use_nodes = True
            material.node_tree.nodes.get('Principled BSDF').inputs['Base Color'].default_value = color
            mesh.materials.append(material)
        mesh.polygons[0].material_index = 0
        mesh.polygons[1].material_index = 1
        uv = mesh.uv_layers.new(name='AtlasUV')
        for face, left in zip(mesh.polygons, [.05,.55]):
            for loop, value in zip(face.loop_indices, [(left,.1),(left+.4,.1),(left+.4,.9),(left,.9)]):
                uv.data[loop].uv = value

    def test_joined_copy_preserves_polygon_material_indices_and_source(self):
        from export_assets import _joined_bake_mesh
        originals = list(self.obj.data.materials)
        joined = _joined_bake_mesh([self.obj])
        self.assertEqual([p.material_index for p in joined.data.polygons], [0,1])
        self.assertEqual([p.material_index for p in self.obj.data.polygons], [0,1])
        self.assertEqual(list(self.obj.data.materials), originals)
        self.assertIsNot(joined.data.materials[0], originals[0])
        self.assertIsNot(joined.data.materials[1], originals[1])

    def test_joined_bake_preserves_each_faces_color(self):
        from export_assets import _joined_bake_mesh, _bake_maps
        joined = _joined_bake_mesh([self.obj])
        images = _bake_maps([joined],64,1)
        pixels = images['basecolor'].pixels
        def sample(x,y):
            start = (y*64+x)*4
            return tuple(pixels[start:start+3])
        self.assertEqual(sample(16,32), (1.,0.,0.), 'First face must bake red')
        self.assertEqual(sample(48,32), (0.,1.,0.), 'Second face must bake green, not first-slot red')
