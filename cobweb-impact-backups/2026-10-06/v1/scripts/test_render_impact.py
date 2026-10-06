#!/usr/bin/env python3
"""Renderer-only validation tests; no Blender or model dependencies."""
import importlib.util
import json
import pathlib
import tempfile
import unittest

class ImpactRendererTests(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        cls.module_path = pathlib.Path(__file__).with_name('render_impact.py')
        cls.renderer = None
        if cls.module_path.exists():
            spec = importlib.util.spec_from_file_location('render_impact', cls.module_path)
            cls.renderer = importlib.util.module_from_spec(spec)
            spec.loader.exec_module(cls.renderer)

    def setUp(self):
        self.assertIsNotNone(self.renderer, 'The impact renderer must exist')
        self.frame = {'phase': 'detached', 'collision_active': False,
                      'resistance_active': False, 'remnant_count': 0,
                      'particles': [], 'rock': {'center': [-1.3, 1.5, 2],
                      'radius': .3, 'axis': [0, 0, 1], 'angle': .2}}

    def test_detached_frame_accepts_real_absence(self):
        with tempfile.TemporaryDirectory() as temp:
            counts = self.renderer.inspect_geometry(pathlib.Path(temp), self.frame)
            self.assertEqual(counts['carrier']['faces'], 0)
            self.assertFalse(counts['carrier']['present'])

    def test_detached_frame_rejects_existing_faces(self):
        with tempfile.TemporaryDirectory() as temp:
            path = pathlib.Path(temp)
            (path/'mesh.obj').write_text('v 0 0 0\nv 1 0 0\nv 0 1 0\nvt 0 0\nvt 1 0\nvt 0 1\nf 1/1 2/2 3/3\n')
            (path/'coverage.tga').write_bytes(b'coverage')
            with self.assertRaisesRegex(ValueError, 'Detached.*faces'):
                self.renderer.inspect_geometry(path, self.frame)

    def test_detached_frame_rejects_active_contacts(self):
        self.frame['collision_active'] = True
        with self.assertRaisesRegex(ValueError, 'Detached.*collision'):
            self.renderer.validate_frame(self.frame)

    def test_intact_frame_requires_real_faces(self):
        self.frame['phase'] = 'intact'
        with tempfile.TemporaryDirectory() as temp:
            with self.assertRaisesRegex(ValueError, 'Intact.*faces'):
                self.renderer.inspect_geometry(pathlib.Path(temp), self.frame)

    def test_rock_must_be_finite_and_positive(self):
        for key, value in [('radius', 0), ('angle', float('nan')), ('center', [0, 0, float('inf')]), ('axis', [0,0,0])]:
            original = self.frame['rock'][key]
            self.frame['rock'][key] = value
            with self.assertRaises(ValueError):
                self.renderer.validate_frame(self.frame)
            self.frame['rock'][key] = original

    def test_empty_mesh_is_not_a_hidden_panel(self):
        with tempfile.TemporaryDirectory() as temp:
            path = pathlib.Path(temp)
            (path/'mesh.obj').write_text('# panel was detached\n')
            counts = self.renderer.inspect_geometry(path, self.frame)
            self.assertEqual(counts['carrier']['vertices'], 0)
            self.assertEqual(counts['carrier']['faces'], 0)
            self.assertFalse(counts['carrier']['present'])

if __name__ == '__main__':
    unittest.main()
