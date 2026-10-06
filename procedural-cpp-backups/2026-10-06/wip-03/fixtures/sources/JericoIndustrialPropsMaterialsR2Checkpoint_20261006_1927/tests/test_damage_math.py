"""Independent geometric contracts for original, stdlib-only panel subtraction."""
import collections
import copy
import importlib.util
import math
import pathlib
import sys
import unittest

sys.path.insert(0, str(pathlib.Path(__file__).resolve().parents[1]))
import core

if importlib.util.find_spec('damage_math'):
    import damage_math as dm
else:
    dm = None


def panel(material='painted_metal', bounds=(0, 1, -.05, .05, 0, 2)):
    builder = core.Builder(core.normalize_recipe({}))
    return builder.box('panel', 'leaf', *bounds, material=material, role='Panel')


def signed_volume(part):
    result = 0.
    vertices = part['vertices']
    for face in part['faces']:
        a = vertices[face[0]]
        for i in range(1, len(face)-1):
            b, c = vertices[face[i]], vertices[face[i+1]]
            result += core._dot(a, core._cross(b, c))/6
    return result


def y_intersections(part, x, z):
    """Independent triangle-ray check of actual caps, without using cut metadata."""
    result = []
    for face in part['faces']:
        for k in range(1, len(face)-1):
            a, b, c = [part['vertices'][i] for i in (face[0], face[k], face[k+1])]
            denominator = (b[2]-c[2])*(a[0]-c[0])+(c[0]-b[0])*(a[2]-c[2])
            if abs(denominator) < 1e-14:
                continue
            u = ((b[2]-c[2])*(x-c[0])+(c[0]-b[0])*(z-c[2]))/denominator
            v = ((c[2]-a[2])*(x-c[0])+(a[0]-c[0])*(z-c[2]))/denominator
            if min(u, v, 1-u-v) >= -1e-9:
                value = u*a[1]+v*b[1]+(1-u-v)*c[1]
                if not any(abs(value-old) < 1e-9 for old in result):
                    result.append(value)
    return sorted(result)


class PanelSubtractionTests(unittest.TestCase):
    def setUp(self):
        self.assertIsNotNone(dm, 'damage_math must implement original panel subtraction')
        self.cut = [[.3, .7], [.7, .7], [.7, 1.3], [.3, 1.3]]

    def assert_shell(self, part):
        scene = dict(schema_version=1, units='m', nodes=[dict(id='leaf', parent=None, translation=[0, 0, 0], rotation_z=0)], parts=[part], sockets=[], grips=[], metadata=dict(leaf_envelopes=[], frame_envelopes=[], grip_supports={}))
        self.assertEqual(core.validate_scene(scene), [])
        self.assertEqual(len(part['faces']), len(part['face_materials']))
        self.assertEqual(len(part['faces']), len(part['face_provenance']))
        self.assertGreater(signed_volume(part), 0)
        for face, uv in zip(part['faces'], part['uvs']):
            for i, j in zip(range(len(face)), list(range(1, len(face)))+[0]):
                distance3 = math.dist(part['vertices'][face[i]], part['vertices'][face[j]])
                self.assertAlmostEqual(distance3, math.dist(uv[i], uv[j]), places=8)

    def test_through_rectangle_removes_real_volume_and_preserves_original_vertices(self):
        original = panel()
        snapshot = copy.deepcopy(original)
        result = dm.carve_panel(original, self.cut)
        self.assertEqual(original, snapshot)
        self.assertEqual(result['vertices'][:len(original['vertices'])], original['vertices'])
        for key in ['id', 'node', 'material', 'role', 'bevel']:
            self.assertEqual(result[key], original[key])
        self.assertEqual(result['panel_bounds'], [0, 1, -.05, .05, 0, 2])
        self.assertAlmostEqual(signed_volume(result), .2-.4*.6*.1, places=10)
        self.assertEqual(y_intersections(result, .431, .937), [])
        self.assertEqual(y_intersections(result, .131, .937), [-.05, .05])
        self.assertFalse(dm.point_in_panel(result, [.431, 0, .937]))
        self.assertTrue(dm.point_in_panel(result, [.131, 0, .937]))
        self.assertIn('cut_metal', result['face_materials'])
        self.assert_shell(result)

    def test_front_depth_preserves_back_surface_and_remaining_thickness(self):
        result = dm.carve_panel(panel(), self.cut, depth=.02)
        self.assertAlmostEqual(signed_volume(result), .2-.4*.6*.02, places=10)
        hits = y_intersections(result, .431, .937)
        self.assertEqual(len(hits), 2)
        self.assertAlmostEqual(hits[0], -.03)
        self.assertAlmostEqual(hits[1], .05)
        self.assertFalse(dm.point_in_panel(result, [.431, -.045, .937]))
        self.assertTrue(dm.point_in_panel(result, [.431, .045, .937]))
        self.assert_shell(result)

    def test_back_depth_preserves_front_surface(self):
        result = dm.carve_panel(panel(), self.cut, side='back', depth=.03)
        hits = y_intersections(result, .431, .937)
        self.assertEqual(len(hits), 2)
        self.assertAlmostEqual(hits[0], -.05)
        self.assertAlmostEqual(hits[1], .02)
        self.assertTrue(dm.point_in_panel(result, [.431, -.045, .937]))
        self.assertFalse(dm.point_in_panel(result, [.431, .045, .937]))
        self.assert_shell(result)

    def test_edge_notch_and_corner_clip_are_closed(self):
        for polygon, area in [([[-.1, .7], [.2, .7], [.2, 1.3], [-.1, 1.3]], .12),
                              ([[-.1, -.1], [.2, -.1], [.2, .3], [-.1, .3]], .06)]:
            for side, depth in [('front', None), ('front', .02), ('back', .02)]:
                with self.subTest(polygon=polygon, side=side, depth=depth):
                    result = dm.carve_panel(panel(), polygon, side=side, depth=depth)
                    self.assertAlmostEqual(signed_volume(result), .2-area*(depth or .1), places=10)
                    self.assert_shell(result)

    def test_concave_cutter_is_not_replaced_by_convex_hull(self):
        polygon = [[.2, .5], [.8, .5], [.8, 1.5], [.6, 1.5], [.6, .8], [.4, .8], [.4, 1.5], [.2, 1.5]]
        result = dm.carve_panel(panel(), polygon)
        self.assertAlmostEqual(signed_volume(result), .2-(.6*.3+.4*.7)*.1, places=10)
        self.assertEqual(y_intersections(result, .511, 1.117), [-.05, .05])
        self.assertEqual(y_intersections(result, .311, 1.117), [])
        self.assert_shell(result)

    def test_sequential_cuts_keep_prior_holes_and_overlap_only_once(self):
        first = dm.carve_panel(panel(), self.cut)
        second_cut = [[.5, 1.1], [.9, 1.1], [.9, 1.6], [.5, 1.6]]
        result = dm.carve_panel(first, second_cut)
        self.assertEqual(result['vertices'][:len(first['vertices'])], first['vertices'])
        self.assertEqual(len(result['cuts']), 2)
        self.assertAlmostEqual(signed_volume(result), .2-(.24+.2-.04)*.1, places=9)
        self.assertEqual(y_intersections(result, .611, 1.211), [])
        self.assertEqual(y_intersections(result, .311, .811), [])
        self.assert_shell(result)

    def test_crossing_front_back_cavities_form_through_opening_only_at_overlap(self):
        first = dm.carve_panel(panel(), self.cut, depth=.065)
        polygon = [[.5, 1.1], [.9, 1.1], [.9, 1.6], [.5, 1.6]]
        result = dm.carve_panel(first, polygon, side='back', depth=.065)
        self.assertAlmostEqual(signed_volume(result), .2-(.24+.2)*.065+.04*.03, places=9)
        self.assertEqual(y_intersections(result, .611, 1.211), [])
        self.assertEqual(len(y_intersections(result, .311, .811)), 2)
        self.assert_shell(result)

    def test_three_disjoint_irregular_cuts_do_not_create_artificial_slivers(self):
        result = panel('metal', (0, 1, -.025, .025, 0, 2))
        cuts = [([.3, .4], [.12, .17]), ([.7, 1.5], [.13, .2]), ([.5, 1.], [.15, .21])]
        for index, (center, radii) in enumerate(cuts):
            result = dm.carve_panel(result, dm.irregular_polygon(center, radii, 18+index))
        for center, _ in cuts:
            self.assertEqual(y_intersections(result, *center), [])
        self.assert_shell(result)

    def test_near_edge_vertex_preserves_exact_cut_without_artificial_sliver(self):
        original = panel('metal', (-.976, 0., .028, .08, .012, 2.088))
        polygon = dm.irregular_polygon([(-.985-.958)/2, (1.293+1.3488)/2],
                                       [(.985-.958)/2, (1.3488-1.293)/2],
                                       165, vertices=9, jaggedness=.57)
        # Independent single-half-plane clipping for exact expected volume.
        clipped = []
        for a, b in zip(polygon, polygon[1:]+polygon[:1]):
            if a[0] >= -.976:
                clipped.append(a)
            if (a[0] >= -.976) != (b[0] >= -.976):
                t = (-.976-a[0])/(b[0]-a[0])
                clipped.append([-.976, a[1]+t*(b[1]-a[1])])
        area = abs(sum(a[0]*b[1]-b[0]*a[1] for a, b in zip(clipped, clipped[1:]+clipped[:1])))/2
        try:
            result = dm.carve_panel(original, polygon, depth=.005)
        except ValueError as error:
            self.fail('Valid near-edge cut must not fail on an artificial partition: '+str(error))
        self.assertEqual(result['vertices'][:len(original['vertices'])], original['vertices'])
        self.assertEqual(result['cuts'][-1]['polygon_xz'], polygon)
        self.assertAlmostEqual(signed_volume(result), signed_volume(original)-area*.005, places=11)
        self.assert_shell(result)

    def test_six_millimetre_depth_edge_chips_after_design_opening(self):
        original = panel('metal', (-.976, 0., .028, .08, .012, 2.088))
        result = dm.carve_panel(original, [[-.75152, .273], [-.22448, .273], [-.22448, .7245], [-.75152, .7245]])
        chips = [(-.952, .274, .3484, .006, 113),
                 (-.949, .5795, .6632, .009, 126),
                 (-.956, .75, .812, .006, 139),
                 (-.94, .886, .9976, .01, 152),
                 (-.958, 1.293, 1.3488, .005, 165),
                 (-.953, 1.8755, 1.9468, .008, 178)]
        for xmax, zmin, zmax, depth, seed in chips:
            polygon = dm.irregular_polygon([(-.985+xmax)/2, (zmin+zmax)/2],
                                           [(xmax+.985)/2, (zmax-zmin)/2],
                                           seed, vertices=9, jaggedness=.57)
            try:
                result = dm.carve_panel(result, polygon, depth=depth)
            except ValueError as error:
                self.fail('Disjoint small edge chips must compose: '+str(error))
        self.assertEqual(len(result['cuts']), 7)
        self.assertEqual(result['vertices'][:len(original['vertices'])], original['vertices'])
        self.assertEqual(y_intersections(result, -.531, .413), [])
        hits = y_intersections(result, -.931, 1.713)
        self.assertEqual(len(hits), 2)
        self.assertAlmostEqual(hits[0], .028, places=12)
        self.assertAlmostEqual(hits[1], .08, places=12)
        self.assert_shell(result)

    def test_glass_interior_has_glass_edge_material(self):
        result = dm.carve_panel(panel('glass', (0, 1, -.003, .003, 0, 2)), self.cut)
        self.assertIn('glass_edge', result['face_materials'])
        self.assertNotIn('cut_metal', result['face_materials'])
        self.assert_shell(result)

    def test_original_and_carved_occupancy_reject_outside_points(self):
        for part in [panel(), dm.carve_panel(panel(), self.cut, depth=.02)]:
            for point in [[-1, 0, 1], [.5, .2, 1], [.5, 0, 3]]:
                self.assertFalse(dm.point_in_panel(part, point))
            with self.assertRaises(ValueError):
                dm.point_in_panel(part, [0, float('nan'), 0])

    def test_occupancy_rejects_sloped_nonrectangular_panel(self):
        sloped = panel()
        cosine, sine = math.cos(.3), math.sin(.3)
        for point in sloped['vertices']:
            x, y = point[:2]
            point[0], point[1] = x*cosine-y*sine, x*sine+y*cosine
        with self.assertRaisesRegex(ValueError, 'rectangular'):
            dm.point_in_panel(sloped, [.4, .1, 1.])

    def test_irregular_profiles_are_repeatable_bounded_and_carve_all_sides(self):
        profile = dm.irregular_polygon([.4, 1.], [.2, .4], 19)
        self.assertEqual(profile, dm.irregular_polygon([.4, 1.], [.2, .4], 19))
        self.assertNotEqual(profile, dm.irregular_polygon([.4, 1.], [.2, .4], 20))
        self.assertEqual(len(profile), 13)
        self.assertTrue(all(abs(x-.4) <= .2 and abs(z-1.) <= .4 for x, z in profile))
        for seed in range(12):
            polygon = dm.irregular_polygon([.05 if seed % 2 else .5, .1 if seed % 3 else 1.], [.3, .5], seed, vertices=13, jaggedness=.45)
            for side, depth in [('front', None), ('front', .021), ('back', .026)]:
                with self.subTest(seed=seed, side=side, depth=depth):
                    result = dm.carve_panel(panel(), polygon, side=side, depth=depth)
                    self.assertEqual(result, dm.carve_panel(panel(), polygon, side=side, depth=depth))
                    self.assert_shell(result)

    def test_invalid_and_unsupported_inputs_fail_without_mutation(self):
        bad_polygons = [[], [[0, 0], [1, 1]], [[.1, .1], [.2, .2], [.3, .3]],
                        [[.1, .1], [.8, .8], [.1, .8], [.8, .1]],
                        [[.1, .1], [.8, .1], [.8, .1], [.1, .8]],
                        [[.1, .1], [.8, .1], [float('nan'), .8]],
                        [[2, 2], [3, 2], [3, 3], [2, 3]],
                        [[-1, -1], [2, -1], [2, 3], [-1, 3]]]
        for polygon in bad_polygons:
            original = panel()
            snapshot = copy.deepcopy(original)
            with self.subTest(polygon=polygon):
                with self.assertRaises(ValueError):
                    dm.carve_panel(original, polygon)
                self.assertEqual(original, snapshot)
        for depth in [0, -.1, .1, .2, True, 'deep', float('nan')]:
            with self.subTest(depth=depth):
                with self.assertRaises(ValueError):
                    dm.carve_panel(panel(), self.cut, depth=depth)
        with self.assertRaises(ValueError):
            dm.carve_panel(panel(), self.cut, side='left')
        deformed = panel()
        deformed['vertices'][0][0] += .01
        with self.assertRaises(ValueError):
            dm.carve_panel(deformed, self.cut)
        many = [[.5+.2*math.cos(i*2*math.pi/129), 1+.3*math.sin(i*2*math.pi/129)] for i in range(129)]
        with self.assertRaises(ValueError):
            dm.carve_panel(panel(), many)
        for args in [([0, 0], [0, 1], 1), ([0, 0], [1, 1], True)]:
            with self.assertRaises(ValueError):
                dm.irregular_polygon(*args)


if __name__ == '__main__':
    unittest.main()
