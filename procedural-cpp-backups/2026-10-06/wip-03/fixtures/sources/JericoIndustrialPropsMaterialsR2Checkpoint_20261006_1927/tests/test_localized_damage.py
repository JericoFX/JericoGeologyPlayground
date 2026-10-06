"""Selection, material-mask, and structural-support contracts for local damage."""
import copy
import importlib.util
import math
import pathlib
import sys
import unittest

sys.path.insert(0, str(pathlib.Path(__file__).resolve().parents[1]))
import core

if importlib.util.find_spec('localized_damage'):
    import localized_damage as ld
else:
    ld = None


def selection(**kwargs):
    value = {'part_id': 'panel', 'side': 'front',
             'region': {'shape': 'box', 'min': [0, -.1, 0], 'max': [1, .1, 2]}}
    value.update(kwargs)
    return value


def panel_scene():
    b = core.Builder(core.normalize_recipe({}))
    b.node('leaf', [0, 0, 0], 'Leaf', None)
    b.scene['metadata'].update(leaf_envelopes=[], frame_envelopes=[])
    p = b.box('panel', 'leaf', 0, 1, -.05, .05, 0, 2, 'painted_steel')
    p['panel_bounds'] = [0, 1, -.05, .05, 0, 2]
    b.box('untouched', 'leaf', 2, 3, -.05, .05, 0, 2, 'painted_steel')
    return b.scene


class SelectionTests(unittest.TestCase):
    def setUp(self):
        self.assertIsNotNone(ld, 'localized_damage selection implementation is missing')

    def test_selection_is_exact_in_part_side_and_physical_region(self):
        s = selection()
        self.assertEqual(ld.selection_weight('panel', 'front', [.5, -.05, 1], s), 1)
        for part_id, side, point in [('other', 'front', [.5, -.05, 1]),
                                      ('panel', 'back', [.5, .05, 1]),
                                      ('panel', 'front', [-.001, -.05, 1]),
                                      ('panel', 'front', [.5, -.11, 1]),
                                      ('panel', 'front', [.5, -.05, 2.001])]:
            with self.subTest(part_id=part_id, side=side, point=point):
                self.assertEqual(ld.selection_weight(part_id, side, point, s), 0)

    def test_box_mask_uses_smootherstep_on_interior_distance(self):
        s = selection(feather_m=.1)
        self.assertEqual(ld.selection_weight('panel', 'front', [0, 0, 1], s), 0)
        self.assertAlmostEqual(ld.selection_weight('panel', 'front', [.05, 0, 1], s), .5)
        self.assertEqual(ld.selection_weight('panel', 'front', [.5, 0, 1], s), 1)

    def test_sphere_mask_uses_true_distance_not_bounding_cube(self):
        s = selection(region={'shape': 'sphere', 'center': [0, 0, 0], 'radius': 1}, feather_m=.2)
        self.assertEqual(ld.selection_weight('panel', 'front', [0, 0, 0], s), 1)
        self.assertAlmostEqual(ld.selection_weight('panel', 'front', [.9, 0, 0], s), .5)
        self.assertEqual(ld.selection_weight('panel', 'front', [.8, .8, 0], s), 0)
        self.assertEqual(ld.selection_weight('panel', 'front', [1.001, 0, 0], s), 0)

    def test_protected_region_defaults_to_zero_and_override_must_be_boolean(self):
        protected = {'shape': 'box', 'min': [.4, -.1, .8], 'max': [.6, .1, 1.2]}
        s = selection(protected_regions=[protected])
        self.assertEqual(ld.selection_weight('panel', 'front', [.5, 0, 1], s), 0)
        s['override_protection'] = True
        self.assertEqual(ld.selection_weight('panel', 'front', [.5, 0, 1], s), 1)
        s['override_protection'] = 'true'
        with self.assertRaises(ValueError):
            ld.selection_weight('panel', 'front', [.5, 0, 1], s)

    def test_reject_nonfinite_and_invalid_regions(self):
        invalid = [selection(side='both'), selection(region={'shape': 'cube'}),
                   selection(region={'shape': 'box', 'min': [1, 0, 0], 'max': [0, 1, 1]}),
                   selection(region={'shape': 'sphere', 'center': [0, 0, 0], 'radius': -1}),
                   selection(region={'shape': 'sphere', 'center': [0, 0, float('inf')], 'radius': 1}),
                   selection(feather_m=-.1), selection(feather_m=10**400)]
        for s in invalid:
            with self.subTest(selection=s):
                with self.assertRaises(ValueError):
                    ld.selection_weight('panel', 'front', [.5, 0, 1], s)
        with self.assertRaises(ValueError):
            ld.selection_weight('panel', 'front', [.5, float('nan'), 1], selection())

    def test_edge_distance_and_mask_are_original_boundary_distance_only(self):
        part = panel_scene()['parts'][0]
        self.assertAlmostEqual(ld.edge_distance(part, [.02, -.05, 1]), .02)
        self.assertAlmostEqual(ld.edge_distance(part, [.5, -.05, 1.99]), .01)
        s = selection(width_m=.1, amount=.8)
        self.assertAlmostEqual(ld.edge_wear_weight(part, 'front', [.05, -.05, 1], s), .4)
        self.assertEqual(ld.edge_wear_weight(part, 'back', [.05, .05, 1], s), 0)
        self.assertEqual(ld.edge_wear_weight(part, 'front', [.5, -.05, 1], s), 0)
        self.assertEqual(ld.edge_wear_weight(part, 'front', [-.01, -.05, 1], s), 0)


    def test_edge_distance_includes_cut_contours_on_only_the_cut_face(self):
        import damage_math
        part = damage_math.carve_panel(panel_scene()['parts'][0], [[.3, .7], [.7, .7], [.7, 1.3], [.3, 1.3]], depth=.02)
        self.assertAlmostEqual(ld.edge_distance(part, [.28, -.05, 1]), .02)
        self.assertAlmostEqual(ld.edge_distance(part, [.28, .05, 1]), .28)
        self.assertAlmostEqual(ld.edge_distance(part, [.5, .05, 1]), .5)


def cut_operation(**kwargs):
    value = selection(type='cut', seed=29, jaggedness=.2,
                      region={'shape': 'box', 'min': [.15, -.06, .7], 'max': [.45, .06, 1.3]})
    value.update(kwargs)
    return value


def with_mounts():
    scene = panel_scene()
    for side, y in [('front', -.045), ('back', .045)]:
        socket_id = 'mount.'+side
        scene['sockets'].append({'id': socket_id, 'node': 'leaf', 'enabled': True,
             'translation': [.3, y, 1], 'coverage': {'support_part_ids': ['panel'], 'footprint': [.1, .2]},
             'support_ends': {'mount': {'node': 'leaf', 'part_ids': ['panel'],
                                      'samples': [[.3, y, 1]], 'required_depth_m': .004}}})
        scene['grips'].append({'id': 'grip.'+side, 'socket_id': socket_id, 'enabled': True})
    return scene


class OperationsTests(unittest.TestCase):
    def setUp(self):
        self.assertIsNotNone(ld, 'localized_damage implementation is missing')
        self.assertTrue(hasattr(ld, 'apply_operations'), 'apply_operations is missing')
        import damage_math
        self.dm = damage_math

    def apply(self, operation=None, scene=None):
        return ld.apply_operations(scene or panel_scene(), [operation or cut_operation()])

    def test_cut_is_immutable_deterministic_and_changes_only_selected_part(self):
        scene = panel_scene()
        original = copy.deepcopy(scene)
        operation = cut_operation()
        operation_original = copy.deepcopy(operation)
        result = self.apply(operation, scene)
        self.assertEqual(scene, original)
        self.assertEqual(operation, operation_original)
        self.assertEqual(result, self.apply(operation, scene))
        self.assertEqual(result['parts'][1], scene['parts'][1])
        self.assertNotEqual(result['parts'][0], scene['parts'][0])
        self.assertEqual(result['parts'][0]['vertices'][:8], scene['parts'][0]['vertices'])
        self.assertEqual(result['metadata']['damage_operations'][-1]['status'], 'applied')
        self.assertEqual(core.validate_scene(result), [])

    def test_missing_depth_is_shallow_and_only_explicit_none_is_through(self):
        shallow = self.apply()['parts'][0]
        self.assertFalse(self.dm.point_in_panel(shallow, [.3, -.049, 1]))
        self.assertTrue(self.dm.point_in_panel(shallow, [.3, .049, 1]))
        self.assertAlmostEqual(shallow['cuts'][-1]['depth'], .025)
        through = self.apply(cut_operation(depth=None))['parts'][0]
        self.assertFalse(self.dm.point_in_panel(through, [.3, .049, 1]))
        self.assertTrue(through['cuts'][-1]['through'])

    def test_front_and_back_cavities_are_different_and_preserve_opposite_face(self):
        front = self.apply(cut_operation(side='front', depth=.02))['parts'][0]
        back = self.apply(cut_operation(side='back', depth=.02))['parts'][0]
        self.assertFalse(self.dm.point_in_panel(front, [.3, -.049, 1]))
        self.assertTrue(self.dm.point_in_panel(front, [.3, .049, 1]))
        self.assertTrue(self.dm.point_in_panel(back, [.3, -.049, 1]))
        self.assertFalse(self.dm.point_in_panel(back, [.3, .049, 1]))
        self.assertNotEqual(front, back)

    def test_left_and_right_damage_are_physically_distinct(self):
        left = self.apply(cut_operation(depth=None))['parts'][0]
        right = self.apply(cut_operation(depth=None, region={'shape': 'box', 'min': [.55, -.06, .7], 'max': [.85, .06, 1.3]}))['parts'][0]
        self.assertFalse(self.dm.point_in_panel(left, [.3, 0, 1]))
        self.assertTrue(self.dm.point_in_panel(left, [.7, 0, 1]))
        self.assertTrue(self.dm.point_in_panel(right, [.3, 0, 1]))
        self.assertFalse(self.dm.point_in_panel(right, [.7, 0, 1]))

    def test_cut_depth_is_clipped_to_box_y_and_exterior_occupancy_is_identical(self):
        operation = cut_operation(depth=.04, region={'shape': 'box', 'min': [.15, -.05, .7], 'max': [.45, -.035, 1.3]})
        result = self.apply(operation)['parts'][0]
        self.assertAlmostEqual(result['cuts'][-1]['depth'], .015)
        original = panel_scene()['parts'][0]
        for x in [.01, .14, .5, .99]:
            for z in [.01, .69, 1., 1.31, 1.99]:
                for y in [-.049, -.03, .049]:
                    point = [x, y, z]
                    self.assertEqual(self.dm.point_in_panel(result, point), self.dm.point_in_panel(original, point))
        self.assertTrue(self.dm.point_in_panel(result, [.3, -.034, 1]))

    def test_sphere_cutter_prism_is_conservatively_inscribed(self):
        region = {'shape': 'sphere', 'center': [.5, -.05, 1.], 'radius': .2}
        result = self.apply(cut_operation(region=region, depth=None))['parts'][0]
        cut = result['cuts'][-1]
        for x, z in cut['polygon_xz']:
            for y in cut['y_range']:
                self.assertLessEqual(math.dist([x, y, z], region['center']), .2+1e-12)
        self.assertFalse(self.dm.point_in_panel(result, [.5, 0, 1]))
        self.assertTrue(self.dm.point_in_panel(result, [.69, .04, 1]))

    def test_disjoint_face_region_does_not_create_unrequested_internal_damage(self):
        scene = panel_scene()
        operation = cut_operation(region={'shape': 'box', 'min': [.2, -.01, .8], 'max': [.4, .01, 1.2]})
        result = self.apply(operation, scene)
        self.assertEqual(result['parts'], scene['parts'])
        self.assertEqual(result['metadata']['damage_operations'][-1]['status'], 'outside')

    def test_default_support_protection_rejects_cut_and_keeps_every_part(self):
        scene = with_mounts()
        result = self.apply(cut_operation(depth=None), scene)
        self.assertEqual(result['parts'], scene['parts'])
        self.assertEqual(result['sockets'], scene['sockets'])
        self.assertEqual(result['grips'], scene['grips'])
        report = result['metadata']['damage_operations'][-1]
        self.assertEqual(report['status'], 'protected')
        self.assertIn('mount.front', report['protected_socket_ids'])

    def test_override_invalidates_only_mount_whose_material_sample_is_lost(self):
        scene = with_mounts()
        result = self.apply(cut_operation(depth=.02, override_protection=True), scene)
        self.assertFalse(result['sockets'][0]['enabled'])
        self.assertTrue(result['sockets'][1]['enabled'])
        self.assertFalse(result['grips'][0]['enabled'])
        self.assertTrue(result['grips'][1]['enabled'])
        self.assertTrue(result['sockets'][0]['support_validation']['reasons'])
        self.assertIn('sample', result['sockets'][0]['support_validation']['reasons'][0])
        self.assertEqual(result['parts'][1], scene['parts'][1])
        self.assertEqual(len(result['parts']), len(scene['parts']))
        self.assertTrue(scene['sockets'][0]['enabled'])

    def test_override_detects_support_volume_loss_between_sample_and_corners(self):
        scene = with_mounts()
        operation = cut_operation(depth=.002, override_protection=True,
            region={'shape': 'box', 'min': [.301, -.051, 1.000], 'max': [.303, -.047, 1.002]})
        result = self.apply(operation, scene)
        part = result['parts'][0]
        self.assertTrue(self.dm.point_in_panel(part, [.3, -.045, 1]))
        for x in [.296, .304]:
            for y in [-.05, -.041]:
                for z in [.996, 1.004]:
                    self.assertTrue(self.dm.point_in_panel(part, [x, y, z]))
        self.assertFalse(result['sockets'][0]['enabled'])
        self.assertTrue(result['sockets'][1]['enabled'])
        self.assertIn('volume', ' '.join(result['sockets'][0]['support_validation']['reasons']))

    def test_volume_check_does_not_confuse_polygon_aabb_with_real_intersection(self):
        scene = with_mounts()
        scene['parts'][0] = self.dm.carve_panel(scene['parts'][0],
            [[.302, 1.008], [.308, 1.002], [.309, 1.009]], depth=.003)
        operation = cut_operation(depth=.01, override_protection=True,
            region={'shape': 'box', 'min': [.6, -.06, .7], 'max': [.8, .06, 1.3]})
        result = self.apply(operation, scene)
        self.assertTrue(result['sockets'][0]['enabled'])
        self.assertTrue(result['sockets'][1]['enabled'])

    def test_support_volume_must_be_inside_original_solid_bounds(self):
        scene = with_mounts()
        scene['sockets'][0]['support_ends']['mount']['samples'][0][0] = .002
        result = self.apply(cut_operation(depth=.01, override_protection=True), scene)
        self.assertTrue(self.dm.point_in_panel(result['parts'][0], [.002, -.045, 1]))
        self.assertFalse(result['sockets'][0]['enabled'])
        self.assertIn('volume', ' '.join(result['sockets'][0]['support_validation']['reasons']))

    def test_override_rechecks_every_socket_end_and_correct_node(self):
        scene = with_mounts()
        scene['nodes'].append({'id': 'remote', 'parent': None, 'translation': [2, 0, 0], 'rotation_z': 0, 'role': 'Support'})
        scene['sockets'].append({'id': 'secondary', 'node': 'leaf', 'enabled': True,
            'coverage': {'support_part_ids': ['untouched']},
            'support_ends': {'remote_end': {'node': 'remote', 'part_ids': ['untouched'],
                'samples': [[.5, 0, 1]], 'required_depth_m': .004}}})
        scene['sockets'].append({'id': 'already_unsupported', 'node': 'leaf', 'enabled': True,
            'coverage': {'support_part_ids': ['untouched']},
            'support_ends': {'remote_end': {'node': 'leaf', 'part_ids': ['untouched'],
                'samples': [[99, 0, 1]], 'required_depth_m': .004}}})
        result = self.apply(cut_operation(depth=.02, override_protection=True), scene)
        self.assertTrue(result['sockets'][2]['enabled'])
        self.assertFalse(result['sockets'][3]['enabled'])
        self.assertEqual(result['sockets'][2]['support_validation']['checked_sample_count'], 1)
        self.assertEqual(result['sockets'][3]['support_validation']['checked_sample_count'], 1)

    def test_coverage_without_samples_still_protects_named_supports(self):
        scene = panel_scene()
        scene['sockets'].append({'id': 'opening', 'node': 'leaf', 'enabled': True,
                                'coverage': {'support_part_ids': ['panel']}})
        result = self.apply(cut_operation(depth=None), scene)
        self.assertEqual(result['parts'], scene['parts'])
        self.assertEqual(result['metadata']['damage_operations'][-1]['status'], 'protected')

    def test_edge_wear_is_surface_only_and_keeps_mesh_exact(self):
        scene = panel_scene()
        operation = selection(type='edge_wear', width_m=.1, amount=.7, seed=4,
            region={'shape': 'box', 'min': [-.01, -.06, .4], 'max': [.13, -.04, 1.6]})
        result = self.apply(operation, scene)
        selected = result['parts'][0]
        for key, value in scene['parts'][0].items():
            self.assertEqual(selected[key], value)
        self.assertEqual(result['parts'][1], scene['parts'][1])
        config = selected['edge_wear'][0]
        self.assertEqual(config['side'], 'front')
        self.assertEqual(config['method'], 'original_perimeter_distance_mask')
        self.assertEqual(len(config['vertex_weights']), len(selected['vertices']))
        self.assertTrue(any(sample['weight'] > 0 for sample in config['samples']))
        self.assertTrue(all(sample['side'] == 'front' and sample['point'][1] == -.05 for sample in config['samples']))
        self.assertEqual(result, self.apply(operation, scene))

    def test_edge_wear_rejects_unsupported_materials_without_mutation(self):
        operation = selection(type='edge_wear', width_m=.1)
        for material in ('glass', 'glass_edge', 'wood', 'cut_metal', 'grime', None):
            scene = panel_scene()
            scene['parts'][0]['material'] = material
            original = copy.deepcopy(scene)
            with self.subTest(material=material):
                with self.assertRaisesRegex(ValueError, 'edge_wear.*material.*painted_steel'):
                    self.apply(operation, scene)
                self.assertEqual(scene, original)
                self.assertGreater(ld.edge_wear_weight(scene['parts'][0], 'front', [.05, -.05, 1], operation), 0)

    def test_edge_wear_accepts_only_supported_metal_bases(self):
        operation = selection(type='edge_wear', width_m=.1)
        for material in ('painted_steel', 'dark_steel', 'iron', 'rust'):
            scene = panel_scene()
            scene['parts'][0]['material'] = material
            with self.subTest(material=material):
                result = self.apply(operation, scene)
                self.assertEqual(result['parts'][0]['material'], material)
                self.assertTrue(result['parts'][0]['edge_wear'])
                self.assertEqual(result['metadata']['damage_operations'][-1]['status'], 'applied')

    def test_edge_wear_protection_zeroes_mount_samples(self):
        scene = with_mounts()
        operation = selection(type='edge_wear', width_m=1., amount=1., samples_per_axis=21)
        result = self.apply(operation, scene)
        config = result['parts'][0]['edge_wear'][0]
        self.assertTrue(config['protected_regions'])
        self.assertEqual(ld.edge_wear_weight(result['parts'][0], 'front', [.3, -.045, 1], config), 0)
        self.assertEqual(result['sockets'], scene['sockets'])

    def test_reject_invalid_operations_budgets_and_missing_parts_before_mutation(self):
        bad = [cut_operation(type='shatter'), cut_operation(part_id='missing'),
               cut_operation(part_id=['panel']),
               cut_operation(depth=0), cut_operation(depth=True), cut_operation(depth=.1),
               cut_operation(depth=float('nan')), cut_operation(jaggedness=1),
               cut_operation(seed=True), cut_operation(vertices=65),
               cut_operation(override_protection=1),
               selection(type='edge_wear', samples_per_axis=65),
               selection(type='edge_wear', width_m=0),
               selection(type='edge_wear', amount=2)]
        for operation in bad:
            scene = panel_scene()
            original = copy.deepcopy(scene)
            with self.subTest(operation=operation):
                with self.assertRaises(ValueError):
                    self.apply(operation, scene)
                self.assertEqual(scene, original)
        for operations in [[cut_operation()]*65, [cut_operation()]*13, 'cut']:
            with self.subTest(operations_type=type(operations).__name__):
                with self.assertRaises(ValueError):
                    ld.apply_operations(panel_scene(), operations)

    def test_wear_before_a_cut_refreshes_face_outline_and_sampling_weights(self):
        scene = panel_scene()
        wear = selection(type='edge_wear', width_m=.1)
        result = ld.apply_operations(scene, [wear, cut_operation(depth=.02)])
        part = result['parts'][0]
        config = part['edge_wear'][0]
        self.assertEqual(len(config['vertex_weights']), len(part['vertices']))
        self.assertGreater(len(config['boundary_segments_xz']), 4)
        for sample in config['samples']:
            self.assertAlmostEqual(sample['weight'], ld.edge_wear_weight(part, 'front', sample['point'], config))

    def test_declared_support_end_without_samples_is_protected_conservatively(self):
        scene = panel_scene()
        scene['sockets'].append({'id': 'unsampled', 'node': 'leaf', 'enabled': True,
            'coverage': {}, 'support_ends': {'mount': {'node': 'leaf', 'part_ids': ['panel']}}})
        result = self.apply(cut_operation(depth=None), scene)
        self.assertEqual(result['parts'], scene['parts'])
        self.assertEqual(result['metadata']['damage_operations'][-1]['status'], 'protected')

    def test_override_front_cut_keeps_industrial_back_handle_and_hinges_valid(self):
        from industrial import build_industrial
        scene = build_industrial({'preset': 'industrial_solid'})
        sock = next(s for s in scene['sockets'] if s['id'] == 'door.handle.front.socket')
        x, y, z = sock['support_ends']['mount']['samples'][1]
        part = next(p for p in scene['parts'] if p['id'] == 'door.sheet')
        y0, y1 = part['panel_bounds'][2:4]
        operation = cut_operation(part_id='door.sheet', depth=.012, override_protection=True,
            region={'shape': 'box', 'min': [x-.065, y0-.002, z-.15], 'max': [x+.065, y1+.002, z+.15]})
        result = self.apply(operation, scene)
        sockets = {s['id']: s for s in result['sockets']}
        self.assertFalse(sockets['door.handle.front.socket']['enabled'])
        self.assertTrue(sockets['door.handle.back.socket']['enabled'])
        self.assertTrue(all(s['enabled'] for s in result['sockets'] if s['role'] == 'Hinge'))
        self.assertEqual(core.validate_scene(result), [])

    def test_empty_operations_return_deepcopy_without_added_damage(self):
        scene = panel_scene()
        result = ld.apply_operations(scene, [])
        self.assertEqual(result, scene)
        self.assertIsNot(result, scene)
        self.assertIsNot(result['parts'][0], scene['parts'][0])


def crack_operation(**kwargs):
    value = selection(type='crack', length_m=.4, width_m=.006, depth_m=.02,
                      seed=41, branches=2,
                      region={'shape': 'box', 'min': [.25, -.06, .5], 'max': [.75, .06, 1.5]})
    value.update(kwargs)
    return value


class CrackOperationsTests(unittest.TestCase):
    def setUp(self):
        self.assertIsNotNone(ld)
        import damage_math
        self.dm = damage_math

    def apply(self, operation=None, scene=None):
        return ld.apply_operations(scene or panel_scene(), [operation or crack_operation()])

    def test_narrow_crack_makes_closed_real_cavities_with_stepped_depth(self):
        scene = panel_scene()
        result = self.apply()
        part = result['parts'][0]
        self.assertEqual(core.validate_scene(result), [])
        self.assertEqual(result['parts'][1], scene['parts'][1])
        self.assertEqual(part['vertices'][:8], scene['parts'][0]['vertices'])
        self.assertEqual(len(part['cuts']), 4)
        self.assertEqual([round(c['depth'], 6) for c in part['cuts']], [.013, .02, .009, .009])
        report = result['metadata']['damage_operations'][-1]
        self.assertEqual(report['type'], 'crack')
        self.assertEqual(report['status'], 'applied')
        self.assertTrue(report['geometry_changed'])
        self.assertFalse(report['through'])
        self.assertEqual(report['profile_count'], 4)
        path = report['profiles'][0]['centerline_xz']
        x, z = path[len(path)//2]
        self.assertFalse(self.dm.point_in_panel(part, [x, -.049, z]))
        self.assertTrue(self.dm.point_in_panel(part, [x, -.029, z]))
        self.assertTrue(self.dm.point_in_panel(part, [x, .049, z]))

    def test_crack_is_reproducible_and_does_not_mutate_inputs(self):
        scene, operation = panel_scene(), crack_operation(branches=1)
        original_scene, original_operation = copy.deepcopy(scene), copy.deepcopy(operation)
        a, b = self.apply(operation, scene), self.apply(operation, scene)
        self.assertEqual(a, b)
        self.assertEqual(scene, original_scene)
        self.assertEqual(operation, original_operation)
        self.assertNotEqual(a['parts'][0]['cuts'], self.apply(crack_operation(branches=1, seed=7))['parts'][0]['cuts'])

    def test_crack_front_and_back_have_exact_opposite_face_preservation(self):
        front = self.apply(crack_operation(branches=0))
        back = self.apply(crack_operation(branches=0, side='back'))
        path = front['metadata']['damage_operations'][-1]['profiles'][0]['centerline_xz']
        x, z = path[len(path)//2]
        self.assertFalse(self.dm.point_in_panel(front['parts'][0], [x, -.049, z]))
        self.assertTrue(self.dm.point_in_panel(front['parts'][0], [x, .049, z]))
        self.assertTrue(self.dm.point_in_panel(back['parts'][0], [x, -.049, z]))
        self.assertFalse(self.dm.point_in_panel(back['parts'][0], [x, .049, z]))

    def test_all_crack_profiles_and_depth_are_inside_box_with_exterior_unchanged(self):
        scene, operation = panel_scene(), crack_operation()
        result = self.apply(operation, scene)
        region = operation['region']
        for cut in result['parts'][0]['cuts']:
            for x, z in cut['polygon_xz']:
                for y in cut['y_range']:
                    self.assertTrue(all(low-1e-10 <= p <= high+1e-10 for p, low, high in zip([x, y, z], region['min'], region['max'])))
        for x in [.01, .2, .8, .99]:
            for z in [.01, .49, 1, 1.51, 1.99]:
                for y in [-.049, 0, .049]:
                    self.assertEqual(self.dm.point_in_panel(result['parts'][0], [x, y, z]), self.dm.point_in_panel(scene['parts'][0], [x, y, z]))

    def test_sphere_contains_complete_profiles_on_both_y_planes(self):
        region = {'shape': 'sphere', 'center': [.5, -.05, 1.], 'radius': .3}
        result = self.apply(crack_operation(region=region))
        for cut in result['parts'][0]['cuts']:
            for x, z in cut['polygon_xz']:
                for y in cut['y_range']:
                    self.assertLessEqual(math.dist([x, y, z], region['center']), region['radius']+1e-10)

    def test_crack_rejects_any_profile_or_depth_outside_region_without_scaling(self):
        invalid = [crack_operation(length_m=1.2),
                   crack_operation(region={'shape': 'box', 'min': [.25, -.06, .5], 'max': [.75, -.04, 1.5]}),
                   crack_operation(center_xz=[.26, 1.], direction_xz=[1, 0]),
                   crack_operation(region={'shape': 'sphere', 'center': [.5, -.05, 1.], 'radius': .15})]
        for operation in invalid:
            scene = panel_scene()
            original = copy.deepcopy(scene)
            with self.subTest(operation=operation):
                with self.assertRaisesRegex(ValueError, 'crack.*region|Crack.*region'):
                    self.apply(operation, scene)
                self.assertEqual(scene, original)

    def test_region_rejects_a_branch_even_when_main_profile_fits(self):
        from crack_math import crack_profiles
        profiles = crack_profiles([.5, 1.], .4, .006, .02, 41)
        main = profiles[0]['polygon_xz']
        region = {'shape': 'box',
                  'min': [min(p[0] for p in main)-.001, -.06, .5],
                  'max': [max(p[0] for p in main)+.001, .06, 1.5]}
        with self.assertRaisesRegex(ValueError, 'crack profile branch.*region'):
            self.apply(crack_operation(region=region, center_xz=[.5, 1.]))

    def test_branch_only_keepout_prevents_the_whole_crack(self):
        from crack_math import crack_profiles
        profiles = crack_profiles([.5, 1.], .4, .006, .02, 41)
        x, z = profiles[2]['centerline_xz'][-1]
        main = profiles[0]['polygon_xz']
        self.assertTrue(x < min(p[0] for p in main)-.003 or x > max(p[0] for p in main)+.003)
        scene = panel_scene()
        operation = crack_operation(protected_regions=[{'shape': 'sphere', 'center': [x, -.045, z], 'radius': .002}])
        result = self.apply(operation, scene)
        self.assertEqual(result['parts'], scene['parts'])
        self.assertEqual(result['metadata']['damage_operations'][-1]['status'], 'protected')

    def test_crack_support_protection_rejects_all_profiles_as_one_operation(self):
        scene = with_mounts()
        operation = crack_operation(center_xz=[.3, 1.], length_m=.12, width_m=.024, branches=0)
        result = self.apply(operation, scene)
        self.assertEqual(result['parts'], scene['parts'])
        self.assertEqual(result['sockets'], scene['sockets'])
        self.assertEqual(result['metadata']['damage_operations'][-1]['status'], 'protected')
        self.assertIn('mount.front', result['metadata']['damage_operations'][-1]['protected_socket_ids'])

    def test_crack_override_revalidates_support_volume_and_disables_grip(self):
        scene = with_mounts()
        result = self.apply(crack_operation(center_xz=[.3, 1.], length_m=.12, width_m=.024, branches=0, override_protection=True), scene)
        self.assertFalse(result['sockets'][0]['enabled'])
        self.assertFalse(result['grips'][0]['enabled'])
        self.assertTrue(result['sockets'][1]['enabled'])
        self.assertEqual(result['sockets'][0]['support_validation']['method'], 'conservative_authored_support_volumes')

    def test_default_crack_depth_is_shallow_and_no_through_is_allowed(self):
        operation = crack_operation(branches=0)
        operation.pop('depth_m')
        result = self.apply(operation)
        self.assertAlmostEqual(max(c['depth'] for c in result['parts'][0]['cuts']), .025)
        self.assertTrue(all(not cut['through'] for cut in result['parts'][0]['cuts']))
        for depth in [None, 0, -.01, .1, .2, True]:
            with self.subTest(depth=depth):
                with self.assertRaises(ValueError):
                    self.apply(crack_operation(depth_m=depth))

    def test_crack_rejects_invalid_parameters_and_expanded_cut_budget(self):
        for changes in [dict(width_m=.0001), dict(width_m=-.01), dict(width_m=True),
                        dict(length_m=.01), dict(length_m=float('nan')), dict(branches=3),
                        dict(segments=13), dict(direction_xz=[0, 0]), dict(center_xz=[0, 0, 0])]:
            with self.subTest(changes=changes):
                with self.assertRaises(ValueError):
                    self.apply(crack_operation(**changes))
        with self.assertRaisesRegex(ValueError, 'budget'):
            ld.apply_operations(panel_scene(), [crack_operation()]*4)

    def test_crack_refreshes_existing_edge_wear_outlines(self):
        scene = panel_scene()
        result = ld.apply_operations(scene, [selection(type='edge_wear', width_m=.1), crack_operation(branches=0)])
        part = result['parts'][0]
        config = part['edge_wear'][0]
        self.assertEqual(len(config['vertex_weights']), len(part['vertices']))
        self.assertGreater(len(config['boundary_segments_xz']), 4)


if __name__ == '__main__':
    unittest.main()
