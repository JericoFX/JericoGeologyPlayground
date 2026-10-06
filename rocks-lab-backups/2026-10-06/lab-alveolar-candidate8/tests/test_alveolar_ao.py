"""AO render uses geometry visibility, preserves matched scene and source data."""
import copy
import importlib
from pathlib import Path
import sys
import tempfile
import unittest

ROOT = Path(__file__).resolve().parents[1]
sys.path.insert(0, str(ROOT / 'tools'))


class AlveolarAO(unittest.TestCase):
    def setUp(self):
        self.assertTrue((ROOT / 'tools/render_alveolar_ao.py').is_file(),
                        'Missing isolated AO renderer')
        self.ao = importlib.import_module('render_alveolar_ao')
        self.directory = tempfile.TemporaryDirectory()
        self.addCleanup(self.directory.cleanup)
        root = Path(self.directory.name)
        self.old_root, self.old_source = self.ao.ROOT, self.ao.SOURCE
        self.ao.ROOT, self.ao.SOURCE = root, root / 'renders'
        self.addCleanup(self.restore_roots)
        for name in ('source', 'renders', 'tools'):
            (root / name).mkdir()
        (root / 'source/heightfield.obj').write_text('v 0 0 0\nv 1 0 0\nv 0 1 0\nf 1 2 3\n')
        (root / 'source/recipe.json').write_text('{"seed":48,"units":"m"}\n')
        (root / 'source/metrics.json').write_text('{"accepted":true}\n')
        (root / 'renders/baseline.png').write_bytes(b'hash-test-fixture')
        (root / 'tools/render_alveolar_studies.py').write_text('# synthetic renderer fixture\n')
        settings = {'camera': {'location': [1., -2., 7.], 'ortho_scale': 3.24},
                    'lights': [{'power_watts': 1680.}], 'samples': 32,
                    'material': {'base_color': [.3, .3, .3], 'roughness': .87}}
        self.record = {
            'settings': settings, 'settings_sha256': self.ao.base.stable_hash(settings),
            'obj': 'source/heightfield.obj', 'image': 'baseline.png',
            'png_sha256': self.ao.base.digest(root / 'renders/baseline.png'),
            'source_hashes': {p.name: self.ao.base.digest(p) for p in (root / 'source').iterdir()},
            'auxiliary_sources': [],
            'renderer_dependencies': {'render_alveolar_studies.py': self.ao.base.digest(root / 'tools/render_alveolar_studies.py')},
            'mesh_stats': [{'vertices': 3, 'triangles': 1, 'imported_mesh_sha256': 'fixture'}],
            'imported_geometry_normals_transforms_unchanged': True}

    def restore_roots(self):
        self.ao.ROOT, self.ao.SOURCE = self.old_root, self.old_source

    def test_ao_settings_change_only_explicit_ao_property(self):
        original = copy.deepcopy(self.record['settings'])
        off = self.ao.settings_with_ao(original, 0., .12, 32)
        on = self.ao.settings_with_ao(original, .30, .12, 32)
        self.assertEqual(self.record['settings'], original)
        self.assertEqual({k: v for k, v in off.items() if k != 'ambient_occlusion'}, original)
        self.assertEqual({k: v for k, v in on.items() if k != 'ambient_occlusion'}, original)
        self.assertEqual(on['ambient_occlusion']['distance_meters'], .12)
        self.assertEqual(on['ambient_occlusion']['strength'], .30)
        self.assertEqual(on['ambient_occlusion']['samples'], 32)
        self.assertEqual(on['ambient_occlusion']['method'], 'Cycles ShaderNodeAmbientOcclusion ray visibility')

    def test_rejects_invalid_strength_radius_or_samples(self):
        for strength, radius, samples in [(-.1,.12,32),(1.1,.12,32),(.3,0.,32),
                                         (.3,float('nan'),32),(.3,.12,0),(.3,.12,1.5),
                                         (float('nan'),.12,32)]:
            with self.assertRaises(ValueError):
                self.ao.settings_with_ao(self.record['settings'], strength, radius, samples)

    def test_baseline_verification_checks_geometry_and_existing_image(self):
        self.ao.verify_baseline(self.record)
        bad = copy.deepcopy(self.record)
        bad['source_hashes']['heightfield.obj'] = '0' * 64
        with self.assertRaisesRegex(ValueError, 'Source'):
            self.ao.verify_baseline(bad)
        bad = copy.deepcopy(self.record)
        bad['png_sha256'] = '0' * 64
        with self.assertRaisesRegex(ValueError, 'image'):
            self.ao.verify_baseline(bad)

    def test_pair_validation_catches_geometry_and_camera_changes(self):
        off = {'settings': self.ao.settings_with_ao(self.record['settings'], 0., .12, 32),
               'source_hashes': self.record['source_hashes'],
               'mesh_stats': self.record['mesh_stats'],
               'imported_geometry_normals_transforms_unchanged': True}
        on = copy.deepcopy(off)
        on['settings']['ambient_occlusion']['strength'] = .3
        self.ao.validate_pair(off, on)
        on['settings']['camera']['ortho_scale'] *= 1.1
        with self.assertRaisesRegex(ValueError, 'Only AO'):
            self.ao.validate_pair(off, on)
        on = copy.deepcopy(off)
        on['source_hashes']['heightfield.obj'] = 'changed'
        with self.assertRaisesRegex(ValueError, 'geometry'):
            self.ao.validate_pair(off, on)


if __name__ == '__main__':
    unittest.main()
