"""Fixed single glazing contract and real, supported broken-pane geometry."""
import collections
import copy
import importlib.util
import json
import math
import pathlib
import sys
import unittest

ROOT = pathlib.Path(__file__).resolve().parents[1]
sys.path.insert(0, str(ROOT))
import core


def signed_volume(part):
    return sum(core._dot(part['vertices'][f[0]], core._cross(part['vertices'][f[i]], part['vertices'][f[i+1]]))/6
               for f in part['faces'] for i in range(1, len(f)-1))


def projected_hit(part, x, z):
    """Intersect a front-to-back ray against mesh faces, with no alpha proxy."""
    p = (x, z)
    def cross(a, b, c):
        return (b[0]-a[0])*(c[1]-a[1])-(b[1]-a[1])*(c[0]-a[0])
    for f in part['faces']:
        for i in range(1, len(f)-1):
            points = [(part['vertices'][k][0], part['vertices'][k][2]) for k in (f[0], f[i], f[i+1])]
            area = cross(*points)
            if abs(area) < 1e-14:
                continue
            signs = [cross(points[j], points[(j+1)%3], p) for j in range(3)]
            if min(signs) >= -1e-12 or max(signs) <= 1e-12:
                return True
    return False


class SinglePaneTests(unittest.TestCase):
    def build(self, **recipe):
        self.assertIsNotNone(importlib.util.find_spec('singlepane'), 'The fixed single-pane generator must exist')
        from singlepane import build_singlepane
        return build_singlepane(recipe)

    def test_default_schema_fixed_opening_and_one_intact_pane(self):
        scene = self.build()
        self.assertEqual(scene['units'], 'm')
        self.assertEqual(scene['basis'], {'handedness':'right', 'up':'+Z', 'front':'-Y'})
        self.assertEqual(scene['recipe']['kind'], 'window')
        self.assertEqual(scene['metadata']['glazing_state'], 'intact')
        self.assertEqual(scene['metadata']['dimensions']['clear_opening'], [1.12, 1.28, .06])
        opening = next(s for s in scene['sockets'] if s['role']=='FrameOpening')
        self.assertEqual(opening['coverage']['opening'], [1.12, 1.28, .06])
        self.assertEqual(opening['coverage']['capabilities'], ['fixed_glazing'])
        glass = [p for p in scene['parts'] if p['material']=='glass']
        self.assertEqual(len(glass), 1)
        self.assertEqual(len(glass[0]['vertices']), 8)
        self.assertTrue(projected_hit(glass[0], 0, .64))
        self.assertFalse(any(p['role'] in ('Mullion','CasementFrame','HingeStrap') for p in scene['parts']))
        self.assertFalse(any(n['role']=='Leaf' for n in scene['nodes']))
        self.assertEqual(scene['grips'], [])
        self.assertIn('not applicable', scene['metadata']['grasp_policy'].lower())

    def test_frame_and_socket_ids_are_stable_across_glazing_states(self):
        scenes = [self.build(glazing=state, seed=83) for state in ('intact','broken','empty')]
        frames = [[p for p in s['parts'] if p['material']!='glass'] for s in scenes]
        self.assertEqual(frames[0], frames[1])
        self.assertEqual(frames[0], frames[2])
        for s in scenes[1:]:
            self.assertEqual(s['nodes'], scenes[0]['nodes'])
            self.assertEqual([v['id'] for v in s['sockets']], [v['id'] for v in scenes[0]['sockets']])
            self.assertEqual(s['metadata']['dimensions'], scenes[0]['metadata']['dimensions'])
        self.assertEqual([p['id'] for p in scenes[0]['parts'] if p['material']=='glass'],
                         [p['id'] for p in scenes[1]['parts'] if p['material']=='glass'])

    def test_glass_socket_enabled_for_intact_and_broken_only(self):
        for state in ('intact','broken','empty'):
            scene = self.build(glazing=state)
            socket = next(s for s in scene['sockets'] if s['role']=='Glass')
            self.assertEqual(socket['enabled'], state!='empty')
            self.assertEqual(scene['metadata']['glazing_state'], state)
            self.assertEqual(socket['coverage']['opening'], [1.12,1.28])
            self.assertLessEqual(socket['coverage']['thickness'], .006)
            self.assertEqual(socket['coverage']['max_thickness'], .004)
            self.assertTrue(socket['coverage']['support_part_ids'])
            candidate = dict(interface='pane.rect.v1',role='Glass',footprint=socket['coverage']['footprint'],thickness=.004)
            self.assertEqual(core.can_connect(socket, candidate), state!='empty')
            for thickness in (.005,.006,.007):
                candidate['thickness'] = thickness
                self.assertFalse(core.can_connect(socket, candidate))
            if state=='empty':
                self.assertFalse(any(p['material']=='glass' for p in scene['parts']))
                self.assertEqual(scene['metadata']['retained_glass_fraction'], 0)

    def test_real_jagged_hole_and_retained_supported_rim(self):
        scene = self.build(glazing='broken')
        glass = next(p for p in scene['parts'] if p['material']=='glass')
        metadata = scene['metadata']
        cx, cz = metadata['glass_hole']['center_xz']
        self.assertFalse(projected_hit(glass, cx, cz))
        self.assertFalse(projected_hit(glass, 0, .64))
        self.assertTrue(projected_hit(glass, -.553, .64))
        self.assertTrue(projected_hit(glass, .553, .64))
        self.assertTrue(projected_hit(glass, 0, .007))
        self.assertTrue(projected_hit(glass, 0, 1.273))
        self.assertGreater(metadata['retained_glass_fraction'], .15)
        self.assertLess(metadata['retained_glass_fraction'], .85)
        hole = metadata['glass_hole']['polygon_xz']
        self.assertGreaterEqual(len(hole), 10)
        self.assertLessEqual(len(hole), 16)
        distances = [math.hypot(x-cx,z-cz) for x,z in hole]
        self.assertGreater(max(distances)/min(distances), 1.5)
        # A closed shell around one hole has Euler characteristic zero.
        edges = {tuple(sorted((a,b))) for f in glass['faces'] for a,b in zip(f,f[1:]+f[:1])}
        self.assertEqual(len(glass['vertices'])-len(edges)+len(glass['faces']), 0)
        # No disconnected triangles or floating islands.
        adjacency = collections.defaultdict(set)
        for a,b in edges:
            adjacency[a].add(b); adjacency[b].add(a)
        reached = {0}; pending = [0]
        while pending:
            for v in adjacency[pending.pop()]-reached:
                reached.add(v); pending.append(v)
        self.assertEqual(len(reached), len(glass['vertices']))

    def test_fracture_is_offset_with_few_long_shards_and_uneven_angles(self):
        for seed in (17,29,73,83):
            scene = self.build(glazing='broken',seed=seed)
            hole = scene['metadata']['glass_hole']
            cx,cz = hole['center_xz']
            width,height,_ = scene['metadata']['dimensions']['glass_size']
            self.assertGreater(abs(cx),width*.065)
            self.assertGreater(abs(cz-1.28/2),height*.025)
            x0,x1,z0,z1 = -width/2,width/2,(1.28-height)/2,(1.28+height)/2
            angles, fractions = [],[]
            for x,z in hole['polygon_xz']:
                dx,dz = x-cx,z-cz
                distance = math.hypot(dx,dz)
                dx,dz = dx/distance,dz/distance
                tx = ((x1 if dx>0 else x0)-cx)/dx if abs(dx)>1e-14 else math.inf
                tz = ((z1 if dz>0 else z0)-cz)/dz if abs(dz)>1e-14 else math.inf
                fractions.append(distance/min(tx,tz))
                angles.append(math.atan2(dz,dx)%math.tau)
            self.assertGreaterEqual(len(fractions),10)
            self.assertLessEqual(len(fractions),16)
            self.assertGreaterEqual(sum(f<.50 for f in fractions),2)
            self.assertLessEqual(sum(f<.50 for f in fractions),4)
            self.assertGreater(sum(f>=.68 for f in fractions),len(fractions)*.65)
            gaps = [(b-a)%math.tau for a,b in zip(angles,angles[1:]+angles[:1])]
            self.assertGreater(max(gaps)/min(gaps),1.8)

    def test_broken_hole_topology_and_volume_across_seeds_and_size_extremes(self):
        def orient(a,b,c):
            return (b[0]-a[0])*(c[1]-a[1])-(b[1]-a[1])*(c[0]-a[0])
        for seed in range(32):
            for width,height in ((.65,3.2),(2.4,.8)):
                scene = self.build(glazing='broken',seed=seed,width=width,height=height)
                self.assertEqual(core.validate_scene(scene),[],(seed,width,height))
                metadata = scene['metadata']
                hole = metadata['glass_hole']['polygon_xz']
                segments = list(zip(hole,hole[1:]+hole[:1]))
                for i,(a,b) in enumerate(segments):
                    for j,(c,d) in enumerate(segments):
                        if j<=i+1 or (i==0 and j==len(segments)-1):
                            continue
                        crosses = orient(a,b,c)*orient(a,b,d)<0 and orient(c,d,a)*orient(c,d,b)<0
                        self.assertFalse(crosses,(seed,width,height,i,j))
                glass = next(p for p in scene['parts'] if p['material']=='glass')
                gw,gh,gt = metadata['dimensions']['glass_size']
                expected = (gw*gh-metadata['glass_hole']['area_m2'])*gt
                self.assertAlmostEqual(signed_volume(glass),expected,places=11)
                self.assertFalse(projected_hit(glass,0,height/2))
                self.assertTrue(projected_hit(glass,-width/2+.004,height/2))
                self.assertTrue(projected_hit(glass,width/2-.004,height/2))
                self.assertTrue(projected_hit(glass,0,.004))
                self.assertTrue(projected_hit(glass,0,height-.004))

    def test_rebate_and_beads_support_pane_without_main_timber_penetration(self):
        scene = self.build()
        parts = {p['id']:p for p in scene['parts']}
        pane = next(p for p in scene['parts'] if p['material']=='glass')
        xmin,xmax = min(v[0] for v in pane['vertices']),max(v[0] for v in pane['vertices'])
        zmin,zmax = min(v[2] for v in pane['vertices']),max(v[2] for v in pane['vertices'])
        self.assertLess(xmin, -.56); self.assertGreater(xmax, .56)
        self.assertLess(zmin, 0); self.assertGreater(zmax, 1.28)
        self.assertLess(max(v[0] for v in parts['frame.left']['vertices']), xmin)
        self.assertGreater(min(v[0] for v in parts['frame.right']['vertices']), xmax)
        self.assertLess(max(v[2] for v in parts['frame.sill']['vertices']), zmin)
        self.assertGreater(min(v[2] for v in parts['frame.head']['vertices']), zmax)
        for side in ('left','right','top','bottom'):
            front,back = [parts['frame.bead.'+face+'.'+side] for face in ('front','back')]
            self.assertAlmostEqual(max(v[1] for v in front['vertices']),min(v[1] for v in pane['vertices']))
            self.assertAlmostEqual(min(v[1] for v in back['vertices']),max(v[1] for v in pane['vertices']))
        for id in ('frame.left','frame.right'):
            self.assertAlmostEqual(max(v[0] for v in parts[id]['vertices'])-min(v[0] for v in parts[id]['vertices']),.135)
        for socket in scene['sockets']:
            for id in socket['coverage'].get('support_part_ids',[]):
                self.assertIn(id,parts)

    def test_clearance_accepts_an_empty_leaf_collection(self):
        result = core.clearance_sweep({'nodes':[], 'metadata':{'leaf_envelopes':[], 'frame_envelopes':[]}})
        self.assertEqual(result['method'], 'not_applicable_fixed_frame')
        self.assertFalse(result['applicable'])
        self.assertEqual(result['samples'], 0)
        self.assertEqual(result['collisions'], [])

    def test_fixed_clearance_is_explicitly_not_applicable(self):
        scene = self.build()
        self.assertEqual(scene['metadata']['leaf_envelopes'], [])
        result = core.clearance_sweep(scene)
        self.assertEqual(result['method'], 'not_applicable_fixed_frame')
        self.assertFalse(result['applicable'])
        self.assertEqual(result['samples'], 0)
        self.assertEqual(result['collisions'], [])
        self.assertEqual(scene['metadata']['clearance']['method'], 'not_applicable_fixed_frame')

    def test_manifold_outward_meshes_finite_metric_uvs_and_validator(self):
        for state in ('intact','broken','empty'):
            scene = self.build(glazing=state)
            self.assertEqual(core.validate_scene(scene), [])
            for part in scene['parts']:
                self.assertGreater(signed_volume(part), 1e-12, part['id'])
                directed = collections.Counter()
                for face,uvs in zip(part['faces'],part['uvs']):
                    for a,b in zip(face,face[1:]+face[:1]): directed[a,b] += 1
                    self.assertTrue(all(math.isfinite(n) for pair in uvs for n in pair))
                    self.assertEqual(len(face),len(uvs))
                    for k in range(1,len(face)):
                        xyz_dist = math.dist(part['vertices'][face[0]],part['vertices'][face[k]])
                        uv_dist = math.dist(uvs[0],uvs[k])
                        self.assertAlmostEqual(xyz_dist,uv_dist,places=9,msg=part['id'])
                self.assertTrue(all(n==1 and directed[b,a]==1 for (a,b),n in directed.items()),part['id'])

    def test_seed_determinism_size_stability_and_input_preservation(self):
        recipe = dict(kind='window',preset='single_pane_fixed',glazing='broken',seed=56,width=.8,height=1.,thickness=.035)
        original = copy.deepcopy(recipe)
        self.assertIsNotNone(importlib.util.find_spec('singlepane'), 'The fixed single-pane generator must exist')
        from singlepane import build_singlepane
        scene = build_singlepane(recipe)
        self.assertEqual(recipe,original)
        self.assertEqual(core.asset_hash(scene),core.asset_hash(build_singlepane(recipe)))
        self.assertNotEqual(core.asset_hash(scene),core.asset_hash(build_singlepane(dict(recipe,seed=57))))
        resized = build_singlepane(dict(recipe,width=2.4,height=3.2,thickness=.12))
        for key in ('parts','nodes','sockets','grips'):
            self.assertEqual([p['id'] for p in scene[key]],[p['id'] for p in resized[key]])
        self.assertEqual(core.validate_scene(resized),[])
        self.assertEqual(core.validate_scene(self.build(width=.65,height=.8,thickness=.028,glazing='broken')),[])

    def test_reject_invalid_inputs_and_moving_angle(self):
        for key,value in [('glazing','smashed'),('glazing',None),('preset','reinforced_timber'),('kind','door'),('width',float('nan')),('height',float('inf')),('thickness',0),('width',True),('seed',1.5),('angle',1),('unknown',1)]:
            with self.subTest(key=key,value=value):
                with self.assertRaises((ValueError,TypeError)):
                    self.build(**{key:value})

    def test_six_shipped_recipes_cover_states_and_sizes(self):
        paths = sorted((ROOT/'recipes').glob('singlepane_*.json'))
        self.assertEqual(len(paths),6)
        recipes = [json.loads(p.read_text()) for p in paths]
        self.assertEqual(collections.Counter(r['glazing'] for r in recipes),dict(intact=2,broken=2,empty=2))
        self.assertEqual(len({(r['width'],r['height']) for r in recipes}),2)
        for recipe in recipes:
            self.assertEqual(core.validate_scene(self.build(**recipe)),[])


if __name__ == '__main__':
    unittest.main()
