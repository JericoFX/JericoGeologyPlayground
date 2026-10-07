"""Run with Blender for texture bake, GLB parsing and import round-trip checks."""
import importlib.util
import json
import math
import pathlib
import struct
import sys
import tempfile
import unittest

sys.path.insert(0, str(pathlib.Path(__file__).resolve().parents[1]))
try:
    import bpy
except ImportError:
    bpy = None


def fixture_scene():
    vertices = [[0,0,0],[1,0,0],[1,.05,0],[0,.05,0],
                [0,0,1],[1,0,1],[1,.05,1],[0,.05,1]]
    faces = [[0,3,2,1],[4,5,6,7],[0,1,5,4],[1,2,6,5],[2,3,7,6],[3,0,4,7]]
    parts = []
    for identity, node, material in [('frame_piece','root','rust'), ('leaf_piece','leaf','wood')]:
        parts.append(dict(id=identity,node=node,role='Leaf',material=material,
                          vertices=vertices,faces=faces,uvs=[[[0,0],[1,0],[1,1],[0,1]] for _ in faces],bevel=.001))
    # Cylindrical hardware end caps are n-gons: exporters must triangulate
    # these before computing portable tangent-space normal directions.
    sides=5
    vertices=[[.1*math.cos(i*2*math.pi/sides),.1*math.sin(i*2*math.pi/sides),z] for z in [0,1] for i in range(sides)]
    faces=[list(reversed(range(sides))),list(range(sides,2*sides))]+[[i,(i+1)%sides,sides+(i+1)%sides,sides+i] for i in range(sides)]
    parts[0].update(vertices=vertices,faces=faces,uvs=[[[0,0] for _ in face] for face in faces])
    return dict(schema_version=1,units='m',basis=dict(handedness='right',up='+Z',front='-Y'),
                recipe=dict(kind='door',width=1.,height=1.,thickness=.05,seed=17,angle=25),
                nodes=[dict(id='root',parent=None,role='Frame',translation=[0,0,0],rotation_z=0),
                       dict(id='leaf',parent='root',role='Leaf',translation=[0,-.1,.1],rotation_z=math.radians(25))],
                parts=parts,sockets=[dict(id='handle',node='leaf',role='HandleFront',enabled=True)],grips=[])


class PortableFormatTests(unittest.TestCase):
    def test_recipe_preset_is_explicit_and_baseline_unchanged(self):
        import export_assets
        from core import build_asset
        self.assertTrue(hasattr(export_assets,'build_export_scene'),'Preset-aware recipe dispatch must exist')
        recipe=dict(kind='door',seed=23)
        self.assertEqual(export_assets.build_export_scene(recipe),build_asset(recipe))
        self.assertEqual(export_assets.build_export_scene(recipe,'mine_timber'),build_asset(recipe))
        reinforced=export_assets.build_export_scene(recipe,'reinforced_timber')
        self.assertEqual(reinforced['recipe']['preset'],'reinforced_timber')
        self.assertGreater(len(reinforced['parts']),len(build_asset(recipe)['parts']))
        self.assertNotIn('preset',recipe)

    def test_freeze_preserves_announced_bytes(self):
        import export_assets
        with tempfile.TemporaryDirectory() as directory:
            folder = pathlib.Path(directory)
            source = folder/'source.bin'
            source.write_bytes(b'first immutable artifact')
            first = export_assets._freeze(source,folder,'asset')
            same = export_assets._freeze(source,folder,'asset')
            self.assertEqual(first,same)
            source.write_bytes(b'updated artifact')
            second = export_assets._freeze(source,folder,'asset')
            self.assertNotEqual(first,second)
            self.assertEqual(first.read_bytes(),b'first immutable artifact')
            self.assertEqual(second.read_bytes(),b'updated artifact')

    def test_glb_parser_rejects_wrong_magic(self):
        import export_assets
        with tempfile.TemporaryDirectory() as directory:
            path = pathlib.Path(directory)/'invalid.glb'
            path.write_bytes(struct.pack('<4sII',b'NOPE',2,12))
            with self.assertRaises(ValueError):
                export_assets.read_glb_json(path)


@unittest.skipIf(bpy is None, 'requires Blender Python')
class ExportTests(unittest.TestCase):
    def test_export_helper_exists(self):
        self.assertIsNotNone(importlib.util.find_spec('export_assets'), 'Portable export helper must exist')

    def test_joined_bake_preserves_authoring_color_and_normal_maps(self):
        import export_assets
        from blender_adapter import clear_scene, import_asset
        self.assertTrue(hasattr(export_assets,'_joined_bake_mesh'),'Efficient bake helper must exist')
        clear_scene()
        objects=import_asset(fixture_scene(),'joined_fixture')
        meshes=[obj for obj in objects.values() if obj.type=='MESH']
        export_assets._prepare_atlas(meshes,128)
        reference=export_assets._bake_maps(meshes,128,1)
        expected={name:list(image.pixels) for name,image in reference.items()}
        joined=export_assets._joined_bake_mesh(meshes)
        self.assertEqual(len(joined.data.polygons),sum(len(obj.data.polygons) for obj in meshes))
        actual=export_assets._bake_maps([joined],128,1)
        for name in ['basecolor','roughness','metallic','normal']:
            errors=[abs(a-b) for a,b in zip(expected[name],actual[name].pixels)]
            self.assertLess(sum(errors)/len(errors),.002,name)
        self.assertEqual(objects['leaf_piece'].parent,objects['leaf'])
        bpy.data.objects.remove(joined,do_unlink=True)

    def test_portable_export_and_roundtrip(self):
        if importlib.util.find_spec('export_assets') is None:
            self.skipTest('export helper not yet implemented')
        import export_assets
        from mathutils import Vector
        scene = fixture_scene()
        with tempfile.TemporaryDirectory() as directory:
            result = export_assets.export_asset(scene,directory,'fixture',texture_size=128,bake_samples=1)
            files = {key:pathlib.Path(result[key]) for key in ['glb','obj','metadata']}
            self.assertTrue(all(path.exists() and path.stat().st_size > 0 for path in files.values()))
            for path in files.values():
                self.assertRegex(path.stem, r'fixture_[0-9a-f]{12}$')
            gltf = export_assets.read_glb_json(files['glb'])
            self.assertEqual(gltf['asset']['version'],'2.0')
            self.assertFalse(gltf.get('cameras'))
            self.assertEqual(len(gltf['meshes']),2)
            self.assertTrue(gltf['accessors'])
            for mesh in gltf['meshes']:
                for primitive in mesh['primitives']:
                    self.assertIn('TEXCOORD_0',primitive['attributes'])
                    self.assertIn('TEXCOORD_1',primitive['attributes'])
                    self.assertIn('TANGENT',primitive['attributes'])
                    for accessor_id in primitive['attributes'].values():
                        self.assertGreater(gltf['accessors'][accessor_id]['count'],0)
            self.assertEqual(len(gltf['images']),3)
            nodes = {n['extras']['stable_id']:n for n in gltf['nodes']}
            self.assertEqual(set(nodes), {'root','leaf','frame_piece','leaf_piece'})
            self.assertIn(gltf['nodes'].index(nodes['leaf_piece']),nodes['leaf']['children'])
            for material in gltf['materials']:
                self.assertIn('baseColorTexture',material['pbrMetallicRoughness'])
                self.assertIn('metallicRoughnessTexture',material['pbrMetallicRoughness'])
                self.assertIn('normalTexture',material)
            metadata = json.loads(files['metadata'].read_text())
            self.assertEqual(metadata['basis'],scene['basis'])
            self.assertEqual(metadata['nodes'],scene['nodes'])
            self.assertEqual(metadata['export']['glb_basis']['up'],'+Y')
            self.assertEqual(metadata['export']['obj_basis'],scene['basis'])
            self.assertEqual(metadata['export']['material_fidelity'],'baked_pbr_atlas')
            self.assertTrue(metadata['export']['roundtrip']['passed'])
            obj = files['obj'].read_text()
            self.assertIn('\nv ',obj)
            self.assertIn('\nvt ',obj)
            self.assertIn('\nf ',obj)
            self.assertIn('mtllib ',obj)
            self.assertGreater(len(list(pathlib.Path(directory).glob('*.png'))),2)
            # A genuinely baked wood/rust map has visible color variation.
            image = bpy.data.images.load(result['textures']['basecolor'],check_existing=False)
            values = list(image.pixels)[0::4]
            self.assertGreater(max(values)-min(values),.04)
            self.assertGreater(max(values),.05)
            # GLB axes must convert back to the original Blender Z-up pivot.
            bpy.ops.object.select_all(action='SELECT')
            bpy.ops.object.delete(use_global=False)
            bpy.ops.import_scene.gltf(filepath=str(files['glb']))
            imported = {obj.get('stable_id'):obj for obj in bpy.context.scene.objects}
            self.assertAlmostEqual(imported['leaf'].location.z,.1,places=5)
            self.assertAlmostEqual(imported['leaf'].matrix_local.to_euler().z,math.radians(25),places=5)
            self.assertEqual(imported['leaf_piece'].parent,imported['leaf'])


if __name__ == '__main__':
    suite = unittest.TestSuite([unittest.defaultTestLoader.loadTestsFromTestCase(PortableFormatTests),unittest.defaultTestLoader.loadTestsFromTestCase(ExportTests)])
    result = unittest.TextTestRunner(verbosity=2).run(suite)
    if not result.wasSuccessful(): sys.exit(1)
