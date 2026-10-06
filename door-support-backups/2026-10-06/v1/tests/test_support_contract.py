"""Behavior contract, authored before implementation; no physics simulation."""
import copy
import hashlib
import importlib.util
import json
import math
from pathlib import Path
import unittest

ROOT = Path(__file__).resolve().parents[1]
FIXTURE = ROOT / 'fixtures/approved_door.json'
PINNED = '3e0d55be93a08009d3d0a502b1444d328fc7f7b73294a4937475c3e2340a090c'


class SupportContractTests(unittest.TestCase):
    def setUp(self):
        self.scene = json.loads(FIXTURE.read_text())
        spec = importlib.util.find_spec('support_contract')
        self.assertIsNotNone(spec, 'The geometric support contract has not been implemented')
        import support_contract
        self.api = support_contract
        self.contract = support_contract.DoorSupportContract(self.scene)

    def cut_mount(self, n, event=None, override=True):
        z = self.scene['sockets'][n+1]['translation'][2]
        return self.contract.apply_box_cut(event or f'cut-{n}', 'frame',
            [-0.570, -0.08, z-0.027], [-0.505, 0.08, z+0.027],
            expected_version=self.contract.version, override_protection=override)

    def test_cli_emits_engine_bridge_manifest(self):
        import subprocess
        import tempfile
        with tempfile.TemporaryDirectory() as output:
            run = subprocess.run(['python', str(ROOT / 'generate_contract.py'), '--output', output], cwd=ROOT, capture_output=True, text=True)
            self.assertEqual(run.returncode, 0, run.stderr)
            manifest = json.loads((Path(output)/'manifest.json').read_text())
            self.assertFalse(manifest['physics_executed'])
            for name, count in [('initial',3),('protected_cut',3),('one_support',1),('zero_support',0)]:
                snapshot = json.loads((Path(output)/(name+'.json')).read_text())
                self.assertEqual(snapshot['articulation']['supports_alive'],count)
                self.assertEqual(snapshot['articulation']['desired_active'],count>0)
                self.assertEqual(len(snapshot['articulation']['support_mount_ids']),count)
            self.assertIn('NO PHYSICS SIMULATION',run.stdout)

    def test_cutting_actual_strap_volume_invalidates_mount_without_removing_wood(self):
        z = 0.2782
        result = self.contract.apply_box_cut('remove-strap-volume','door.leaf',[0,0.0095,z-0.05],[1,0.0205,z+0.05],expected_version=0,override_protection=True)
        self.assertEqual(result['status'],'applied')
        self.assertTrue(self.contract.material_at('door.plank.0',[0.06,0.05,z]))
        state = self.contract.snapshot()
        self.assertFalse(state['mounts'][0]['supported'])
        self.assertEqual(state['articulation']['supports_alive'],2)

    def test_protection_rejects_subsample_hardware_cut(self):
        result = self.contract.apply_box_cut('tiny-strap','door.leaf',[0.60001,0.011,0.27801],[0.60002,0.012,0.27802],expected_version=0)
        self.assertEqual(result['status'],'rejected_protected_support')

    def test_polygon_not_aabb_decides_material_occupancy(self):
        self.assertFalse(self.contract.material_at('door.plank.0', [0.1260836337159,0.05,0.02]))
        self.assertTrue(self.contract.material_at('door.plank.0', [0.1260836337159,0.05,0.10]))

    def test_tiny_protected_cut_cannot_hide_between_bearing_samples(self):
        result = self.contract.apply_box_cut('tiny', 'frame', [-0.56001,-0.035,0.27801], [-0.55999,-0.034,0.27803], expected_version=0)
        self.assertEqual(result['status'],'rejected_protected_support')

    def test_fixture_hash_unchanged(self):
        payload = json.dumps(self.scene, sort_keys=True, separators=(',', ':')).encode()
        self.assertEqual(hashlib.sha256(payload).hexdigest(), PINNED)
        before = FIXTURE.read_bytes()
        self.cut_mount(0)
        self.assertEqual(FIXTURE.read_bytes(), before)
        self.assertEqual(self.contract.snapshot()['source_scene_hash'], PINNED)

    def test_no_damage_has_three_supports_and_matching_endpoints(self):
        state = self.contract.snapshot()
        self.assertEqual(state['articulation']['supports_alive'], 3)
        self.assertFalse(state['physics_executed'])
        self.assertEqual(len(state['mounts']), 3)
        for mount in state['mounts']:
            a, b = mount['endpoints']
            self.assertEqual(a['logical_body'], 'frame')
            self.assertEqual(b['logical_body'], 'door.leaf')
            self.assertNotEqual(a['id'], b['id'])
            for x, y in zip(a['world_transform']['translation'], b['world_transform']['translation']):
                self.assertAlmostEqual(x, y)
            self.assertAlmostEqual(a['local_transform']['translation'][0], -0.496)
            self.assertAlmostEqual(b['local_transform']['translation'][0], 0)
            self.assertTrue(a['connected_to_owner'])
            self.assertTrue(b['connected_to_owner'])
            self.assertGreater(a['bearing']['original_area_m2'], 0)
            self.assertGreater(b['bearing']['required_depth_m'], 0)

    def test_off_target_cut_does_not_affect_support(self):
        result = self.contract.apply_box_cut('off', 'frame', [10,10,10], [11,11,11], expected_version=0)
        self.assertEqual(result['status'], 'applied')
        self.assertEqual(self.contract.snapshot()['articulation']['supports_alive'], 3)

    def test_protected_mount_cut_rejected_transactionally(self):
        result = self.cut_mount(0, override=False)
        self.assertEqual(result['status'], 'rejected_protected_support')
        self.assertEqual(self.contract.version, 0)
        self.assertEqual(self.contract.snapshot()['damage']['cuts'], [])
        self.assertEqual(self.contract.snapshot()['articulation']['supports_alive'], 3)

    def test_override_removes_occupied_material_without_changing_aabb(self):
        self.assertTrue(self.contract.material_at('frame.left', [-0.535,0,0.2782]))
        original = copy.deepcopy(self.contract.snapshot()['source_part_bounds']['frame.left'])
        self.cut_mount(0)
        state = self.contract.snapshot()
        self.assertFalse(self.contract.material_at('frame.left', [-0.535,0,0.2782]))
        self.assertEqual(state['source_part_bounds']['frame.left'], original)
        self.assertFalse(state['mounts'][0]['supported'])
        self.assertEqual(state['articulation']['supports_alive'], 2)
        self.assertEqual(state['mounts'][0]['endpoints'][0]['bearing']['retained_area_m2'], 0)

    def test_one_hinge_survives_three_to_one_then_removed_once(self):
        self.cut_mount(0)
        self.cut_mount(1)
        self.assertEqual(self.contract.snapshot()['articulation']['supports_alive'], 1)
        self.assertEqual(self.contract.snapshot()['articulation']['state'], 'ideal_hinge_planned')
        self.assertEqual(self.contract.snapshot()['planned_commands'], [])
        result = self.cut_mount(2, event='last')
        self.assertEqual([c['command'] for c in result['commands']], ['remove_constraint', 'activate_body'])
        self.assertEqual(self.contract.snapshot()['articulation']['state'], 'detached_planned')
        z = self.scene['sockets'][3]['translation'][2]
        retry = self.contract.apply_box_cut('last', 'frame', [-0.570,-0.08,z-0.027], [-0.505,0.08,z+0.027], expected_version=2, override_protection=True)
        self.assertEqual(retry['status'], 'duplicate')
        self.assertEqual(retry['commands'], [])
        self.cut_mount(2, event='again')
        self.assertEqual(len(self.contract.snapshot()['planned_commands']), 2)

    def test_intact_island_loses_required_owner_connection(self):
        # The cut is above mount 2. All bearing volumes stay intact, but the
        # whole lower jamb is disconnected from the head's explicit owner seed.
        result = self.contract.apply_box_cut('sever-jamb', 'frame', [-0.70,-0.1,1.97], [-0.50,0.1,1.99], expected_version=0, override_protection=True)
        state = self.contract.snapshot()
        self.assertEqual(result['status'], 'applied')
        self.assertEqual(state['articulation']['supports_alive'], 0)
        for mount in state['mounts']:
            endpoint = mount['endpoints'][0]
            self.assertGreater(endpoint['bearing']['retained_area_m2'], 0)
            self.assertFalse(endpoint['connected_to_owner'])
            self.assertNotEqual(endpoint['support_component_ids'], [endpoint['required_owner_component_id']])

    def test_protection_rejects_floating_protected_island(self):
        result = self.contract.apply_box_cut('protect-connectivity', 'frame', [-0.70,-0.1,1.97], [-0.50,0.1,1.99], expected_version=0)
        self.assertEqual(result['status'], 'rejected_protected_support')
        self.assertEqual(self.contract.snapshot()['articulation']['supports_alive'], 3)

    def test_insufficient_depth_removes_support(self):
        z = 0.2782
        self.contract.apply_box_cut('deep-pocket', 'frame', [-0.57,-0.02,z-0.03], [-0.50,0.07,z+0.03], expected_version=0, override_protection=True)
        endpoint = self.contract.snapshot()['mounts'][0]['endpoints'][0]
        self.assertFalse(endpoint['supported'])
        self.assertGreater(endpoint['bearing']['original_area_m2'], 0)

    def test_removed_actual_handle_part_disables_only_its_grip(self):
        result = self.contract.remove_part('remove-front-lever', 'door.handle.front.lever', expected_version=0, override_protection=True)
        grips = {g['id']:g for g in self.contract.snapshot()['grips']}
        self.assertEqual(result['status'], 'applied')
        self.assertFalse(grips['door.handle.front.grip']['enabled'])
        self.assertTrue(grips['door.handle.back.grip']['enabled'])
        self.assertEqual(grips['door.handle.front.grip']['source_contact_part_id'], 'door.handle.front.lever')
        self.assertEqual(grips['door.handle.front.grip']['position'], self.scene['grips'][0]['position'])

    def test_removed_socket_support_part_disables_both_grips(self):
        self.contract.remove_part('remove-handle-wood', 'door.plank.4', expected_version=0, override_protection=True)
        self.assertTrue(all(not g['enabled'] for g in self.contract.snapshot()['grips']))

    def test_invalid_cuts_versions_and_ids_are_rejected(self):
        for minimum, maximum in [([0,0,0],[0,1,1]), ([0,0,0],[-1,1,1]), ([0,0,0],[1,1,math.inf]), ([0,math.nan,0],[1,1,1]), ([0,0],[1,1,1])]:
            with self.subTest(minimum=minimum, maximum=maximum):
                with self.assertRaises(ValueError):
                    self.contract.apply_box_cut('bad','frame',minimum,maximum,expected_version=0)
        with self.assertRaises(ValueError):
            self.contract.apply_box_cut('unknown','fake',[0,0,0],[1,1,1],expected_version=0)
        with self.assertRaises(ValueError):
            self.contract.remove_part('missing','unknown-part',expected_version=0)
        with self.assertRaises(ValueError):
            self.contract.apply_box_cut('stale','frame',[10,10,10],[11,11,11],expected_version=3)
        self.contract.apply_box_cut('same','frame',[10,10,10],[11,11,11],expected_version=0)
        with self.assertRaises(ValueError):
            self.contract.apply_box_cut('same','frame',[12,12,12],[13,13,13],expected_version=1)

    def test_deterministic_canonical_output_and_compile(self):
        self.assertEqual(self.contract.canonical_json(), self.api.DoorSupportContract(self.scene).canonical_json())
        self.assertEqual(self.api.compile_contract(self.scene), self.contract.snapshot())
        self.assertEqual(json.loads(self.contract.canonical_json()), self.contract.snapshot())
        altered = copy.deepcopy(self.scene)
        altered['units'] = 'cm'
        with self.assertRaises(ValueError):
            self.api.DoorSupportContract(altered)

if __name__ == '__main__':
    unittest.main()
