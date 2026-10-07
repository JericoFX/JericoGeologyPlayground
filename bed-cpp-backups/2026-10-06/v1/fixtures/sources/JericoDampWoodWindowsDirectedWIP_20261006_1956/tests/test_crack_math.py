"""Behavioral contracts for original swept, finite-width modeled tears."""
import copy
import importlib.util
import math
import pathlib
import random
import sys
import unittest

sys.path.insert(0, str(pathlib.Path(__file__).resolve().parents[1]))
import core
import damage_math as dm

cm = None
if importlib.util.find_spec('crack_math'):
    import crack_math as cm


def volume(part):
    result = 0.
    for face in part['faces']:
        a = part['vertices'][face[0]]
        for i in range(1, len(face)-1):
            b, c = (part['vertices'][face[j]] for j in (i, i+1))
            result += core._dot(a, core._cross(b, c))/6
    return result


class CrackProfileTests(unittest.TestCase):
    def setUp(self):
        self.assertIsNotNone(cm, 'crack_math must supply original modeled-tear profiles')

    def profiles(self, **changes):
        options = dict(center_xz=[.5, 1.], length=.48, width=.006,
                       depth=.025, seed=41)
        options.update(changes)
        return cm.crack_profiles(**options)

    def test_repeatable_seed_without_mutating_inputs_or_global_random(self):
        center = [.5, 1.]
        direction = [.3, .9]
        before = copy.deepcopy((center, direction))
        random.seed(345)
        state = random.getstate()
        a = self.profiles(center_xz=center, direction_xz=direction)
        self.assertEqual(random.getstate(), state)
        self.assertEqual((center, direction), before)
        self.assertEqual(a, self.profiles(center_xz=center, direction_xz=direction))
        self.assertNotEqual(a, self.profiles(center_xz=center, direction_xz=direction, seed=42))

    def test_length_width_and_depth_each_control_geometry(self):
        a = self.profiles(branches=0)
        long = self.profiles(length=.72, branches=0)
        wide = self.profiles(width=.012, branches=0)
        deep = self.profiles(depth=.04, branches=0)
        self.assertAlmostEqual(a[0]['centerline_xz'][-1][1]-a[0]['centerline_xz'][0][1], .48)
        self.assertAlmostEqual(long[0]['centerline_xz'][-1][1]-long[0]['centerline_xz'][0][1], .72)
        self.assertGreater(dm._area(wide[0]['polygon_xz']), 1.8*dm._area(a[0]['polygon_xz']))
        self.assertEqual([p['polygon_xz'] for p in a], [p['polygon_xz'] for p in deep])
        self.assertGreater(max(p['depth'] for p in deep), max(p['depth'] for p in a))

    def test_direction_is_normalized_and_rotates_the_actual_slit(self):
        a = self.profiles(branches=0, direction_xz=(0, 7))
        self.assertEqual(a, self.profiles(branches=0))
        b = self.profiles(branches=0, direction_xz=(1, 0))
        self.assertAlmostEqual(b[0]['centerline_xz'][-1][0]-b[0]['centerline_xz'][0][0], .48)
        self.assertAlmostEqual(b[0]['centerline_xz'][-1][1], b[0]['centerline_xz'][0][1])

    def test_branches_overlap_main_and_extend_outside_it(self):
        for count in (0, 1, 2):
            profiles = self.profiles(branches=count)
            self.assertLessEqual(len(profiles), 5)
            self.assertEqual([p['role'] for p in profiles].count('main'), 1)
            self.assertEqual([p['role'] for p in profiles].count('branch'), count)
            main = next(p for p in profiles if p['role'] == 'main')
            for branch in (p for p in profiles if p['role'] == 'branch'):
                root, tip = branch['centerline_xz'][0], branch['centerline_xz'][-1]
                self.assertTrue(dm._contains(main['polygon_xz'], root, boundary=False))
                self.assertFalse(dm._contains(main['polygon_xz'], tip))
                self.assertTrue(dm._contains(branch['polygon_xz'], root))

    def test_depth_varies_along_same_connected_slit(self):
        profiles = self.profiles()
        main = next(p for p in profiles if p['role'] == 'main')
        deep = next(p for p in profiles if p['role'] == 'deep_section')
        self.assertAlmostEqual(main['depth'], .025*.65)
        self.assertAlmostEqual(deep['depth'], .025)
        self.assertLess(dm._area(deep['polygon_xz']), dm._area(main['polygon_xz']))
        self.assertTrue(all(dm._contains(main['polygon_xz'], point) for point in deep['centerline_xz']))
        self.assertTrue(all(p['depth'] == .025*.45 for p in profiles if p['role'] == 'branch'))

    def test_angular_centerline_has_taper_and_bounded_miters(self):
        main = self.profiles(branches=0)[0]
        line, polygon = main['centerline_xz'], main['polygon_xz']
        self.assertTrue(any(abs(dm._cross2(a, b, c)) > 1e-6
                            for a, b, c in zip(line, line[1:], line[2:])))
        radii = [min(math.dist(vertex, point) for point in line) for vertex in polygon]
        self.assertLessEqual(max(radii), .006*.5*1.5)
        self.assertGreaterEqual(min(radii), .001*.5-1e-12)
        self.assertGreater(max(radii), 1.5*min(radii))

    def test_profiles_are_simple_ccw_finite_and_bounded_at_short_and_long_scales(self):
        for seed in range(40):
            for width, length in ((.001, .004), (.001, .55), (.006, .024), (.012, .7)):
                profiles = self.profiles(seed=seed, width=width, length=length, segments=12,
                                         direction_xz=(.34, -.86))
                self.assertLessEqual(len(profiles), 5)
                for p in profiles:
                    self.assertIn(p['role'], ('main', 'branch', 'deep_section'))
                    self.assertTrue(math.isfinite(p['depth']) and p['depth'] > 0)
                    self.assertLessEqual(len(p['polygon_xz']), 64)
                    self.assertEqual(dm._profile(p['polygon_xz']), p['polygon_xz'])
                    self.assertGreater(dm._area(p['polygon_xz']), 1e-12)
                    self.assertGreaterEqual(len(p['centerline_xz']), 2)
                    self.assertTrue(all(math.isfinite(x) for point in p['polygon_xz'] for x in point))

    def test_invalid_inputs_fail_clearly(self):
        cases = [dict(center_xz=(0, float('nan'))), dict(center_xz=[0]),
                 dict(length=.003, width=.001), dict(length=float('inf')),
                 dict(width=.00099), dict(width=True), dict(width=-1),
                 dict(depth=0), dict(depth=float('nan')), dict(depth=True), dict(depth=5e-324),
                 dict(seed=True), dict(seed=3.4), dict(branches=-1),
                 dict(branches=3), dict(branches=True), dict(segments=1),
                 dict(segments=13), dict(segments=3.5),
                 dict(direction_xz=(0, 0)), dict(direction_xz=(float('inf'), 1))]
        for changes in cases:
            with self.subTest(changes=changes):
                with self.assertRaises(ValueError):
                    self.profiles(**changes)

    def test_narrow_slits_remove_real_volume_and_stay_positive_manifold(self):
        for width in (.001, .003, .006):
            for seed in (7, 41):
                with self.subTest(width=width, seed=seed):
                    builder = core.Builder(core.normalize_recipe({}))
                    part = builder.box('panel', 'leaf', 0, 1, -.026, .026, 0, 2,
                                       material='painted_metal', role='Panel')
                    before = volume(part)
                    for profile in self.profiles(width=width, seed=seed):
                        previous = volume(part)
                        part = dm.carve_panel(part, profile['polygon_xz'], side='front', depth=profile['depth'])
                        self.assertGreater(previous-volume(part), 1e-12)
                        self.assertLess(part['cut_math']['cells'], 1800)
                        scene = dict(schema_version=1, units='m',
                                     nodes=[dict(id='leaf', parent=None, translation=[0, 0, 0], rotation_z=0)],
                                     parts=[part], sockets=[], grips=[],
                                     metadata=dict(leaf_envelopes=[], frame_envelopes=[], grip_supports={}))
                        self.assertEqual(core.validate_scene(scene), [])
                    self.assertGreater(volume(part), 0)
                    self.assertLess(volume(part), before)
                    self.assertEqual(len(part['cuts']), 4)

    def test_rotated_one_millimetre_branches_do_not_create_tiny_partition_artifacts(self):
        for seed, length, depth in ((5, .48, .025), (1, .12, .02), (3, .004, .02), (5, .12, .02)):
            with self.subTest(seed=seed, length=length):
                builder = core.Builder(core.normalize_recipe({}))
                part = builder.box('panel', 'leaf', 0, 1, -.026, .026, 0, 2,
                                   material='painted_metal', role='Panel')
                profiles = self.profiles(width=.001, length=length, depth=depth, seed=seed,
                                         direction_xz=(math.sin(seed), math.cos(seed)))
                for profile in profiles:
                    part = dm.carve_panel(part, profile['polygon_xz'], depth=profile['depth'])
                self.assertGreater(volume(part), 0)
                scene = dict(schema_version=1, units='m',
                             nodes=[dict(id='leaf', parent=None, translation=[0, 0, 0], rotation_z=0)],
                             parts=[part], sockets=[], grips=[],
                             metadata=dict(leaf_envelopes=[], frame_envelopes=[], grip_supports={}))
                self.assertEqual(core.validate_scene(scene), [])


if __name__ == '__main__':
    unittest.main()
