import collections
import copy
import json
import math
import pathlib
import subprocess
import sys
import tempfile
import unittest

sys.path.insert(0, str(pathlib.Path(__file__).resolve().parents[1]))
try:
    import core
except ImportError:
    core = None


def volume(part):
    vs = part['vertices']
    result = 0.0
    for face in part['faces']:
        a = vs[face[0]]
        for i in range(1, len(face)-1):
            b, c = vs[face[i]], vs[face[i+1]]
            result += (a[0]*(b[1]*c[2]-b[2]*c[1]) + a[1]*(b[2]*c[0]-b[0]*c[2]) + a[2]*(b[0]*c[1]-b[1]*c[0]))/6
    return result


class CoreTests(unittest.TestCase):
    def setUp(self):
        self.assertIsNotNone(core, 'The procedural core must exist')

    def test_schema_dimensions_units(self):
        for kind, w, h, t in [('door', 1.0, 2.1, .052), ('window', 1.4, 1.25, .045)]:
            scene = core.build_asset(dict(kind=kind, width=w, height=h, thickness=t))
            self.assertEqual(scene['schema_version'], 1)
            self.assertEqual(scene['units'], 'm')
            self.assertEqual(scene['basis'], dict(handedness='right', up='+Z', front='-Y'))
            opening = next(s for s in scene['sockets'] if s['role']=='FrameOpening')
            self.assertEqual(opening['coverage']['opening'], [w, h, t])
            self.assertEqual(core.validate_scene(scene), [])

    def test_repeat_hash_and_stable_ids_across_sizes(self):
        for kind in ['door', 'window']:
            a = core.build_asset(dict(kind=kind, seed=23))
            self.assertEqual(core.asset_hash(a), core.asset_hash(core.build_asset(dict(kind=kind, seed=23))))
            b = core.build_asset(dict(kind=kind, width=1.6, height=2.4))
            for collection in ['nodes', 'parts', 'sockets', 'grips']:
                self.assertEqual([p['id'] for p in a[collection]], [p['id'] for p in b[collection]])
            self.assertNotEqual(core.asset_hash(a), core.asset_hash(core.build_asset(dict(kind=kind, seed=24))))

    def test_invalid_parameters(self):
        for field, value in [('width', 0), ('height', -1), ('thickness', 0), ('width', float('nan')), ('height', float('inf')), ('thickness', 'deep'), ('width', True), ('seed', 1.2), ('kind', 'gate'), ('damage', 'destroyed'), ('handle', 'knob'), ('angle', 120)]:
            with self.subTest(field=field, value=value):
                with self.assertRaises((ValueError, TypeError)):
                    core.build_asset({field:value})

    def test_mesh_manifold_outward_uv(self):
        for kind in ['door', 'window']:
            for damage in ['aged', 'deteriorated', 'broken']:
                scene=core.build_asset(dict(kind=kind, damage=damage))
                for p in scene['parts']:
                    edges=collections.Counter()
                    oriented=collections.Counter()
                    self.assertGreater(volume(p), 1e-12, p['id'])
                    self.assertEqual(len(p['faces']),len(p['uvs']))
                    for f, uv in zip(p['faces'],p['uvs']):
                        self.assertEqual(len(f),len(uv))
                        for i,j in zip(f,f[1:]+f[:1]):
                            edges[tuple(sorted((i,j)))]+=1
                            oriented[i,j]+=1
                        a=p['vertices'][f[0]]
                        for j in range(1,len(f)-1):
                            b,c=p['vertices'][f[j]],p['vertices'][f[j+1]]
                            ab=[b[k]-a[k] for k in range(3)]
                            ac=[c[k]-a[k] for k in range(3)]
                            cross=[ab[1]*ac[2]-ab[2]*ac[1],ab[2]*ac[0]-ab[0]*ac[2],ab[0]*ac[1]-ab[1]*ac[0]]
                            self.assertGreater(sum(v*v for v in cross),1e-20,(p['id'],f))
                        self.assertTrue(all(math.isfinite(n) for pair in uv for n in pair))
                    self.assertTrue(all(count==2 for count in edges.values()),p['id'])
                    self.assertTrue(all(oriented[j,i]==count for (i,j),count in oriented.items()),p['id'])

    def test_handle_generic_swap_and_fit_rejections(self):
        scene=core.build_asset({})
        socket=next(s for s in scene['sockets'] if s['role']=='HandleFront')
        for style in ['lever','pull']:
            candidate=core.handle_interface(style)
            self.assertTrue(core.can_connect(socket,candidate))
        bad=core.handle_interface('pull'); bad['role']='Glass'
        self.assertFalse(core.can_connect(socket,bad))
        bad=core.handle_interface('pull'); bad['interface']='mount.handle.v2'
        self.assertFalse(core.can_connect(socket,bad))
        bad=core.handle_interface('pull'); bad['footprint']=[10,10]
        self.assertFalse(core.can_connect(socket,bad))
        bad=core.handle_interface('pull'); bad['required_capabilities']=['electric']
        self.assertFalse(core.can_connect(socket,bad))
        disabled=copy.deepcopy(socket); disabled['enabled']=False
        self.assertFalse(core.can_connect(disabled,core.handle_interface('pull')))

    def test_world_transform_and_open_leaf_pivots(self):
        for kind, angle in [('door',65),('window',65)]:
            a=core.build_asset(dict(kind=kind))
            b=core.build_asset(dict(kind=kind,angle=angle))
            for node in a['nodes']:
                self.assertEqual(core.world_point(a,node['id'],[0,0,0]),core.world_point(b,node['id'],[0,0,0]))
            moving=next(n for n in b['nodes'] if n['role']=='Leaf')
            self.assertNotEqual(core.world_point(a,moving['id'],[.2,.1,.5]),core.world_point(b,moving['id'],[.2,.1,.5]))
        scene={'nodes':[dict(id='a',parent=None,translation=[1,2,3],rotation_z=math.pi/2),dict(id='b',parent='a',translation=[2,0,0],rotation_z=0)]}
        p=core.world_point(scene,'b',[1,0,0])
        for value,expected in zip(p,[1,5,3]): self.assertAlmostEqual(value,expected)

    def test_sweep_sat_clearances(self):
        self.assertTrue(core.rectangles_overlap([(0,0),(1,0),(1,1),(0,1)],[(.5,.5),(2,.5),(2,2),(.5,2)]))
        self.assertFalse(core.rectangles_overlap([(0,0),(1,0),(1,1),(0,1)],[(1.01,0),(2,0),(2,1),(1.01,1)]))
        for kind in ['door','window']:
            for w,h,t in [(0.75,1.0,.035),(1.0,2.1,.052),(1.6,2.4,.08)]:
                result=core.clearance_sweep(core.build_asset(dict(kind=kind,width=w,height=h,thickness=t)),step_degrees=.25)
                self.assertEqual(result['collisions'],[],(kind,w,h,t,result))
                self.assertGreater(result['samples'],300)

    def test_grips_on_support_and_damage_disables(self):
        for kind in ['door','window']:
            for style in ['lever','pull']:
                scene=core.build_asset(dict(kind=kind,handle=style))
                for grip in scene['grips']:
                    self.assertTrue(grip['enabled'])
                    self.assertLessEqual(core.grip_surface_distance(scene,grip),1e-8)
                broken=core.build_asset(dict(kind=kind,handle=style,damage='broken'))
                self.assertTrue(any(not s['enabled'] for s in broken['sockets']))
                self.assertTrue(any(not g['enabled'] for g in broken['grips']))
                for grip in broken['grips']:
                    if grip['enabled']: self.assertLessEqual(core.grip_surface_distance(broken,grip),1e-8)
                    else: self.assertFalse(next(s for s in broken['sockets'] if s['id']==grip['socket_id'])['enabled'])
                self.assertLess(len(broken['parts']),len(scene['parts']))

    def test_full_supported_dimension_boundaries_clear_frame(self):
        for kind in ['door', 'window']:
            for w in [.65, 2.4]:
                for h in [.8, 3.2]:
                    for t in [.028, .12]:
                        with self.subTest(kind=kind,width=w,height=h,thickness=t):
                            scene=core.build_asset(dict(kind=kind,width=w,height=h,thickness=t))
                            self.assertEqual(core.clearance_sweep(scene,.25)['collisions'],[])
                            self.assertEqual(core.validate_scene(scene),[])

    def test_planks_have_varied_widths_and_real_splintered_damage(self):
        scene=core.build_asset(dict(damage='deteriorated'))
        planks=[p for p in scene['parts'] if p['role']=='Plank']
        widths=[round(max(v[0] for v in p['vertices'])-min(v[0] for v in p['vertices']),5) for p in planks]
        self.assertGreater(len(set(widths)),3)
        self.assertTrue(any(len(p['vertices'])>=24 for p in planks))
        self.assertEqual(core.validate_scene(scene),[])

    def test_mount_coverage_matches_supporting_wood(self):
        for kind in ['door','window']:
            scene=core.build_asset(dict(kind=kind,width=.65))
            for socket in scene['sockets']:
                if socket['role'] not in ['HandleFront','HandleBack']: continue
                supports=socket['coverage'].get('support_part_ids',[])
                self.assertTrue(supports)
                support=next(p for p in scene['parts'] if p['id']==supports[0])
                width=max(v[0] for v in support['vertices'])-min(v[0] for v in support['vertices'])
                self.assertLessEqual(socket['coverage']['footprint'][0],width)
                self.assertTrue(core.can_connect(socket,core.handle_interface('lever',socket['role'])))
                self.assertTrue(core.can_connect(socket,core.handle_interface('pull',socket['role'])))

    def test_nonfinite_component_projection_rejected(self):
        socket=next(s for s in core.build_asset({})['sockets'] if s['role']=='HandleFront')
        for projection in [float('nan'),float('inf'),-1,True]:
            component=core.handle_interface('lever'); component['projection']=projection
            self.assertFalse(core.can_connect(socket,component))

    def test_validation_detects_tampered_sockets_and_transforms(self):
        scene=core.build_asset({})
        scene['sockets'][0]['translation'][0]=float('nan')
        self.assertTrue(any('nonfinite' in error for error in core.validate_scene(scene)))
        scene=core.build_asset({})
        next(n for n in scene['nodes'] if n['role']=='Leaf')['translation'][0]+=.1
        self.assertTrue(any('collision' in error.lower() for error in core.validate_scene(scene)))

    def test_frame_joinery_does_not_overlap_at_head(self):
        for kind in ['door','window']:
            scene=core.build_asset(dict(kind=kind))
            parts={p['id']:p for p in scene['parts']}
            head_bottom=min(v[2] for v in parts['frame.head']['vertices'])
            for side in ['left','right']:
                jamb_top=max(v[2] for v in parts['frame.'+side]['vertices'])
                self.assertLessEqual(jamb_top,head_bottom)
                envelope=next(e for e in scene['metadata']['frame_envelopes'] if e['id']=='frame.'+side)
                self.assertEqual(envelope['z'][1],jamb_top)

    def test_socket_and_grip_bases_are_explicit_right_handed(self):
        scene=core.build_asset({})
        for record in scene['sockets']+scene['grips']:
            basis=record.get('basis')
            self.assertIsNotNone(basis,record['id'])
            x,y,z=[basis[k] for k in ['x','y','z']]
            for vector in [x,y,z]: self.assertAlmostEqual(sum(v*v for v in vector),1)
            for a,b in [(x,y),(x,z),(y,z)]: self.assertAlmostEqual(sum(v*w for v,w in zip(a,b)),0)
            cross=[x[1]*y[2]-x[2]*y[1],x[2]*y[0]-x[0]*y[2],x[0]*y[1]-x[1]*y[0]]
            self.assertAlmostEqual(sum(a*b for a,b in zip(cross,z)),1)
        front=next(s for s in scene['sockets'] if s['role']=='HandleFront')
        back=next(s for s in scene['sockets'] if s['role']=='HandleBack')
        self.assertEqual(front['normal'],[0,-1,0])
        self.assertEqual(back['normal'],[0,1,0])
        self.assertAlmostEqual(back['rotation_z'],math.pi)
        for grip in scene['grips']:
            self.assertEqual(grip['approach'],[-v for v in grip['basis']['z']])
            socket=next(s for s in scene['sockets'] if s['id']==grip['socket_id'])
            self.assertEqual(grip['basis']['z'],socket['normal'])

    def test_coplanar_cap_uvs_have_consistent_metric_projection(self):
        scene=core.build_asset(dict(damage='deteriorated'))
        plank=next(p for p in scene['parts'] if p['role']=='Plank')
        min_y=min(v[1] for v in plank['vertices']); seen={}
        for face,uvs in zip(plank['faces'],plank['uvs']):
            if not all(abs(plank['vertices'][i][1]-min_y)<1e-10 for i in face): continue
            for i,uv in zip(face,uvs):
                if i in seen:
                    for a,b in zip(seen[i],uv): self.assertAlmostEqual(a,b)
                seen[i]=uv
                self.assertAlmostEqual(uv[0],plank['vertices'][i][0])
                self.assertAlmostEqual(uv[1],plank['vertices'][i][2])

    def test_all_shipped_recipes_are_valid(self):
        from recipe_router import build_recipe
        paths=list((pathlib.Path(__file__).resolve().parents[1]/'recipes').glob('*.json'))
        self.assertGreaterEqual(len(paths),6)
        kinds=collections.Counter()
        for path in paths:
            recipe=json.loads(path.read_text()); scene=build_recipe(recipe)
            self.assertEqual(core.validate_scene(scene),[],path.name)
            if scene['recipe']['damage']!='broken': kinds[scene['recipe']['kind']]+=1
        self.assertGreaterEqual(kinds['door'],2)
        self.assertGreaterEqual(kinds['window'],2)

    def test_window_lever_handles_do_not_cross_center_seam(self):
        scene=core.build_asset(dict(kind='window',handle='lever'))
        for part in scene['parts']:
            if '.handle.' not in part['id']: continue
            for vertex in part['vertices']:
                world=core.world_point(scene,part['node'],vertex)
                if part['node']=='window.left': self.assertLess(world[0],0,part['id'])
                else: self.assertGreater(world[0],0,part['id'])

    def test_grips_land_on_face_centers_surviving_edge_bevels(self):
        for kind in ['door','window']:
            for style in ['lever','pull']:
                scene=core.build_asset(dict(kind=kind,handle=style))
                parts={p['id']:p for p in scene['parts']}
                for grip in scene['grips']:
                    part=parts[scene['metadata']['grip_supports'][grip['id']]]
                    centers=[[sum(part['vertices'][i][axis] for i in f)/len(f) for axis in range(3)] for f in part['faces'] if len(f)==4]
                    distances=[sum((a-b)**2 for a,b in zip(center,grip['position'])) for center in centers]
                    self.assertLess(min(distances),1e-20,grip['id'])

    def test_narrow_thick_window_leaves_do_not_intersect(self):
        for angle in [5,15,30,45,60,85]:
            scene=core.build_asset(dict(kind='window',width=.65,height=.8,thickness=.12,angle=angle))
            rectangles=[]
            for node in ['window.left','window.right']:
                part=next(p for p in scene['parts'] if p['id']==node+'.stile.inner')
                xs=[v[0] for v in part['vertices']]; ys=[v[1] for v in part['vertices']]
                rectangle=[[min(xs),min(ys),0],[max(xs),min(ys),0],[max(xs),max(ys),0],[min(xs),max(ys),0]]
                rectangles.append([core.world_point(scene,node,p)[:2] for p in rectangle])
            self.assertFalse(core.rectangles_overlap(*rectangles),angle)

    def test_leaf_leaf_sweep_detects_overlap(self):
        scene=core.build_asset(dict(kind='window'))
        envelope=scene['metadata']['leaf_envelopes'][0]
        for point in envelope['rect']:
            if point[0]>0: point[0]+=.10
        result=core.clearance_sweep(scene)
        self.assertTrue(any('other_node' in c for c in result['collisions']))

    def test_malformed_component_thickness_ranges_fail_closed(self):
        socket=next(s for s in core.build_asset({})['sockets'] if s['role']=='HandleFront')
        for limits in [[],[.028],None,['bad',.12],[.028,float('nan')],[.12,.028]]:
            component=core.handle_interface('lever');component['thickness_range']=limits
            self.assertFalse(core.can_connect(socket,component),limits)

    def test_glazing_and_hinge_interface_limits_are_enforced(self):
        scene=core.build_asset(dict(kind='window'))
        glass=next(s for s in scene['sockets'] if s['role']=='Glass')
        component=dict(interface='pane.rect.v1',role='Glass',footprint=glass['coverage']['footprint'],thickness=.004)
        self.assertTrue(core.can_connect(glass,component))
        component['thickness']=.5
        self.assertFalse(core.can_connect(glass,component))
        del component['thickness']
        self.assertFalse(core.can_connect(glass,component))
        hinge=next(s for s in scene['sockets'] if s['role']=='Hinge')
        component=dict(interface='hinge.pin.v1',role='Hinge',footprint=[.03,.06],pin_diameter=.028)
        self.assertTrue(core.can_connect(hinge,component))
        for diameter in [.018,.05,float('nan'),None]:
            component['pin_diameter']=diameter
            self.assertFalse(core.can_connect(hinge,component))

    def test_cli_writes_valid_json(self):
        with tempfile.TemporaryDirectory() as directory:
            output=pathlib.Path(directory)/'asset.json'
            cli=pathlib.Path(__file__).resolve().parents[1]/'generate.py'
            result=subprocess.run([sys.executable,str(cli),'--kind','window','--width','1.4','--out',str(output)],capture_output=True,text=True)
            self.assertEqual(result.returncode,0,result.stderr)
            self.assertEqual(core.validate_scene(json.loads(output.read_text())),[])

if __name__=='__main__': unittest.main()
