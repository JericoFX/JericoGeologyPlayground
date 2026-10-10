"""Renderer isolation, patch framing, equal settings and provenance guards."""
import argparse
import copy
import importlib
import json
from pathlib import Path
import sys
import tempfile
import unittest

ROOT = Path(__file__).resolve().parents[1]
sys.path.insert(0, str(ROOT / 'tools'))


def recipe(variant='alveolar'):
    versions = {'support': ('heightfield-cliff-v11', 'massive_oblique'),
                'limestone': ('surface-families-v13', 'limestone_dissolution'),
                'alveolar': ('surface-families-v14', 'alveolar_blocks')}
    version, profile = versions[variant]
    return {'units': 'm', 'size': 2., 'relief': .55, 'backDepth': .3,
            'seed': 48, 'generatorVersion': version, 'profile': profile,
            'grid': {'cells': 256, 'width': 1025, 'height': 1025},
            'patchRadiusMeters': .22}


class AlveolarRendering(unittest.TestCase):
    def setUp(self):
        self.assertTrue((ROOT / 'tools/render_alveolar_studies.py').is_file(), 'Missing dedicated alveolar renderer')
        self.render = importlib.import_module('render_alveolar_studies')

    def test_matched_comparison_rejects_grid_and_generator_mismatch(self):
        entries = {name: {'recipe': recipe(name)} for name in self.render.VARIANTS}
        self.render.validate_group(entries, 48)
        bad = copy.deepcopy(entries)
        bad['support']['recipe']['grid']['width'] = 2049
        with self.assertRaisesRegex(ValueError, 'source resolution'):
            self.render.validate_group(bad, 48)
        bad = copy.deepcopy(entries)
        bad['support']['recipe']['profile'] = 'vertical_channels'
        with self.assertRaisesRegex(ValueError, 'profile'):
            self.render.validate_group(bad, 48)

    def test_rejects_study_geometry_that_would_mislabel_comparisons(self):
        entries = {name: {'recipe': recipe(name)} for name in self.render.VARIANTS}
        for entry in entries.values():
            entry['recipe']['size'] = 3.
        with self.assertRaisesRegex(ValueError, 'study dimensions'):
            self.render.validate_group(entries, 48)

    def test_detail_camera_uses_patch_position_and_matches_all_variants(self):
        anchor = {'patch_index': 0, 'center_uv': [.8, .2], 'center_meters': [.6, -.6],
                  'target': [.6, -.6, .4], 'patch_radius_meters': .22}
        args = argparse.Namespace(size=640, samples=32)
        settings = [self.render.settings_for(args, recipe(v), 'detail', 'neutral', 'studio', '4.3.2', anchor)
                    for v in self.render.VARIANTS]
        self.assertTrue(all(s == settings[0] for s in settings))
        self.assertEqual(settings[0]['camera']['target'], [.6, -.6, .4])
        self.assertGreater(settings[0]['camera']['ortho_scale'], .44)

    def test_lateral_lighting_changes_actual_lights_with_same_camera(self):
        args = argparse.Namespace(size=640, samples=32)
        anchor = {'patch_index': 0, 'center_uv': [.8, .2], 'center_meters': [.6, -.6],
                  'target': [.6, -.6, .4], 'patch_radius_meters': .22}
        a = self.render.settings_for(args, recipe(), 'detail', 'neutral', 'studio', '4.3.2', anchor)
        b = self.render.settings_for(args, recipe(), 'detail', 'neutral', 'lateral', '4.3.2', anchor)
        self.assertEqual(a['camera'], b['camera'])
        self.assertNotEqual(a['lights'], b['lights'])
        self.assertLess(b['lights'][0]['location'][2], a['lights'][0]['location'][2])
        self.assertEqual(a['material'], b['material'])
        self.assertEqual(a['source_geometry_changes'], [])

    def test_patch_anchor_requires_real_interior_events(self):
        self.assertEqual(self.render.patch_center({'events': {'patches': [{'x': .6, 'y': -.6, 'radius': .22, 'aspect': 1.1, 'angle': 0.}]}})['center_meters'], [.6, -.6])
        with self.assertRaises(ValueError):
            self.render.patch_center({'events': {'patches': []}})
        with self.assertRaises(ValueError):
            self.render.patch_center({'events': {'patches': [{'x': float('nan'), 'y': .5, 'radius': .22, 'aspect': 1., 'angle': 0.}]}})

    def test_obj_height_anchor_interpolates_original_front_vertices(self):
        with tempfile.TemporaryDirectory() as tmp:
            path = Path(tmp) / 'sample.obj'
            path.write_text('v -1 -1 0\nv 1 -1 1\nv -1 1 1\nv 1 1 2\nv 0 0 -0.3\n')
            self.assertAlmostEqual(self.render.sample_front_z(path, [0, 0], 2, 1), 1.)


class AlveolarComposition(unittest.TestCase):
    def setUp(self):
        self.assertTrue((ROOT / 'tools/compose_alveolar_studies.py').is_file(), 'Missing dedicated alveolar compositor')
        self.compose = importlib.import_module('compose_alveolar_studies')

    def test_comparison_settings_require_full_equality(self):
        records = [{'settings': {'camera': [1], 'lights': [2]}, 'renderer_dependencies': {'renderer': 'a'}} for _ in range(3)]
        self.compose.validate_row(records)
        records[1]['settings']['lights'] = [3]
        with self.assertRaisesRegex(ValueError, 'settings'):
            self.compose.validate_row(records)

    def test_comparison_rejects_mismatched_renderer_dependencies(self):
        records = [{'settings': {}, 'renderer_dependencies': {'renderer': 'a'}} for _ in range(3)]
        records[1]['renderer_dependencies']['renderer'] = 'b'
        with self.assertRaisesRegex(ValueError, 'renderer'):
            self.compose.validate_row(records)

    def fixture(self, root):
        from PIL import Image
        render = importlib.import_module('render_alveolar_studies')
        render_dir = root / 'render'
        (render_dir / 'provenance').mkdir(parents=True)
        for variant in render.VARIANTS:
            source = root / variant
            source.mkdir()
            r = recipe(variant)
            (source / 'heightfield.obj').write_text('v 0 0 0\n')
            (source / 'recipe.json').write_text(json.dumps(r))
            (source / 'metrics.json').write_text(json.dumps({'accepted': True, 'triangles': 1}))
            hashes = {f.name: render.digest(f) for f in source.iterdir()}
            for row in render.ROWS:
                stem = render.stem_for(48, variant, *row)
                image = render_dir / (stem + '.png')
                Image.new('RGB', (64, 64), '#85958f').save(image)
                settings = {'row': row}
                record = {'image': image.name, 'png_sha256': render.digest(image),
                          'settings': settings, 'settings_sha256': render.stable_hash(settings),
                          'seed': 48, 'variant': variant, 'view': row[0], 'mode': row[1], 'lighting': row[2],
                          'obj': str((source / 'heightfield.obj').relative_to(ROOT)),
                          'source_hashes': hashes, 'auxiliary_sources': [], 'recipe': r,
                          'renderer_dependencies': {'renderer': 'same'},
                          'mesh_stats': [{'triangles': 1}],
                          'imported_geometry_normals_transforms_unchanged': True}
                (render_dir / 'provenance' / (stem + '.json')).write_text(json.dumps(record))
        return render_dir

    def test_composition_verifies_sources_and_preserves_dimensions(self):
        from PIL import Image
        (ROOT / 'renders').mkdir(exist_ok=True)
        with tempfile.TemporaryDirectory(dir=ROOT / 'renders') as tmp:
            root = Path(tmp)
            render_dir = self.fixture(root)
            output = root / 'comparison'
            output.mkdir()
            manifest = self.compose.compose(render_dir, output, 48)
            self.assertEqual(len(manifest['sources']), 12)
            self.assertEqual(manifest['layout']['tile_width'], 64)
            self.assertEqual(manifest['layout']['tile_height'], 64)
            with Image.open(ROOT / manifest['image']) as result:
                self.assertEqual(result.size, (manifest['layout']['width'], manifest['layout']['height']))
            (root / 'support' / 'heightfield.obj').write_text('changed')
            with self.assertRaisesRegex(ValueError, 'Stale source'):
                self.compose.record_for(render_dir, 48, 'support', ('main', 'neutral', 'studio'))

    def test_composition_rejects_mislabeled_record(self):
        (ROOT / 'renders').mkdir(exist_ok=True)
        with tempfile.TemporaryDirectory(dir=ROOT / 'renders') as tmp:
            render_dir = self.fixture(Path(tmp))
            name = render_dir / 'provenance/alveolar-48-support-main-neutral-studio.json'
            record = json.loads(name.read_text())
            record['variant'] = 'alveolar'
            name.write_text(json.dumps(record))
            with self.assertRaisesRegex(ValueError, 'identity'):
                self.compose.record_for(render_dir, 48, 'support', ('main', 'neutral', 'studio'))

    def test_layout_preserves_source_pixels_before_jpeg_encoding(self):
        layout = self.compose.layout_for(640, 640, 3, 4)
        self.assertEqual(layout['tile_width'], 640)
        self.assertEqual(layout['tile_height'], 640)
        self.assertGreaterEqual(layout['width'], 3 * 640)
        self.assertGreaterEqual(layout['height'], 4 * 640)


if __name__ == '__main__':
    unittest.main()
