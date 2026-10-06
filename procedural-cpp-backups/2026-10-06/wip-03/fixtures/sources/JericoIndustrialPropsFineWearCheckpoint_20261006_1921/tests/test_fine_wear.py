"""Opt-in fine coating breakup, isolated from source volume and earlier materials."""
import copy
import hashlib
import importlib.util
import json
import pathlib
import unittest

try:
    import bpy
except ImportError:
    bpy = None


def fixture(enabled=None, material='painted_steel'):
    from core import Builder, normalize_recipe
    from localized_damage import apply_operations
    b = Builder(normalize_recipe({}))
    b.node('leaf', [0, 0, 0], 'Leaf', None)
    b.scene['metadata'].update(leaf_envelopes=[], frame_envelopes=[])
    p = b.box('panel', 'leaf', 0, .1, -.005, .005, 0, .1, material)
    p['panel_bounds'] = [0, .1, -.005, .005, 0, .1]
    op = dict(type='edge_wear', part_id='panel', side='front', width_m=.025,
              amount=1, feather_m=.001, samples_per_axis=5,
              region=dict(shape='box', min=[-.01,-.01,-.01], max=[.04,0,.055]))
    if enabled is not None:
        op['fine_breakup'] = enabled
    return apply_operations(b.scene, [op])


def mesh_digest(data):
    return hashlib.sha256(json.dumps([{k:p[k] for k in ('id','vertices','faces','uvs')} for p in data['parts']], sort_keys=True).encode()).hexdigest()


def graph(material):
    tree = material.node_tree
    nodes = list(tree.nodes)
    def value(v):
        return tuple(v) if hasattr(v, '__len__') and not isinstance(v, str) else v
    return ([ (n.bl_idname, getattr(n,'operation',None), getattr(n,'blend_type',None),
               [(i.name, value(i.default_value)) for i in n.inputs if hasattr(i,'default_value')]) for n in nodes],
            [(nodes.index(l.from_node),l.from_socket.identifier,nodes.index(l.to_node),l.to_socket.identifier) for l in tree.links])


class FineWearConfigurationTests(unittest.TestCase):
    def test_fine_breakup_rejects_non_boolean(self):
        for value in ('true', 1, {}, []):
            with self.subTest(value=value), self.assertRaisesRegex(ValueError, 'fine_breakup'):
                fixture(value)

    def test_fine_breakup_is_front_only(self):
        from localized_damage import apply_operations
        data = fixture()
        op = copy.deepcopy(data['parts'][0]['edge_wear'][0])
        op.update(side='back', fine_breakup=True)
        with self.assertRaisesRegex(ValueError, 'front'):
            apply_operations(data, [op])

    def test_toggle_retains_identical_source_mesh_and_absent_default(self):
        baseline, disabled, enabled = fixture(), fixture(False), fixture(True)
        self.assertNotIn('fine_breakup',baseline['parts'][0]['edge_wear'][0])
        self.assertEqual(mesh_digest(baseline),mesh_digest(enabled))
        self.assertEqual(mesh_digest(disabled),mesh_digest(enabled))
        self.assertIs(enabled['parts'][0]['edge_wear'][0]['fine_breakup'],True)


@unittest.skipIf(bpy is None, 'Blender required')
class FineWearAdapterTests(unittest.TestCase):
    def test_import_applies_fine_breakup_only_to_front_without_mutating_source(self):
        from blender_adapter import clear_scene, import_asset
        clear_scene()
        data = fixture(True)
        before = copy.deepcopy(data)
        ob = import_asset(data)['panel']
        front = [ob.data.materials[p.material_index] for p in ob.data.polygons if p.normal.y < -.999]
        back = [ob.data.materials[p.material_index] for p in ob.data.polygons if p.normal.y > .999]
        self.assertTrue(front and back)
        self.assertTrue(all(bool(m.get('fine_breakup')) for m in front))
        self.assertTrue(all(not m.get('fine_breakup') for m in back))
        self.assertEqual(data,before)

    def test_actual_fine_recipe_evaluated_atlas_rejects_no_degenerate_triangles(self):
        from blender_adapter import clear_scene, import_asset
        from industrial import build_industrial
        from export_assets import _prepare_atlas
        recipe=json.loads(pathlib.Path('recipes/fine_wear_after.json').read_text())
        data=build_industrial(recipe);before=copy.deepcopy(data)
        clear_scene();objects=import_asset(data)
        _prepare_atlas([objects['door.sheet']],128)
        self.assertEqual(data,before)
        mesh=objects['door.sheet'].data
        for face in mesh.polygons:
            a,b,c=[tuple(mesh.vertices[i].co) for i in face.vertices]
            u=[b[i]-a[i] for i in range(3)];v=[c[i]-a[i] for i in range(3)]
            cross=[u[1]*v[2]-u[2]*v[1],u[2]*v[0]-u[0]*v[2],u[0]*v[1]-u[1]*v[0]]
            self.assertGreater(sum(x*x for x in cross),1e-28)

    def test_debug_view_does_not_enable_material_refinement(self):
        from blender_adapter import clear_scene, import_asset
        clear_scene()
        ob=import_asset(fixture(True),debug_uv=True)['panel']
        self.assertTrue(all(not m.get('fine_breakup') for m in ob.data.materials))

    def test_disabled_and_missing_optin_have_identical_material_graphs(self):
        from blender_adapter import clear_scene, import_asset
        snapshots = []
        for enabled in (None, False):
            clear_scene()
            ob = import_asset(fixture(enabled))['panel']
            snapshots.append([graph(m) for m in ob.data.materials])
        self.assertEqual(*snapshots)

    def test_copies_material_and_preserves_linked_or_constant_inputs(self):
        self.assertIsNotNone(importlib.util.find_spec('fine_wear_adapter'), 'fine-wear adapter missing')
        from fine_wear_adapter import apply_fine_wear
        from blender_adapter import _material
        part = fixture(True)['parts'][0]
        for linked in (False, True):
            source, tree, bs = _material('Fine.test.'+str(linked),(.08,.09,.07),.83,.14)
            if linked:
                for channel in ('Base Color','Roughness','Metallic'):
                    node = tree.nodes.new('ShaderNodeRGB' if channel == 'Base Color' else 'ShaderNodeValue')
                    tree.links.new(node.outputs[0], bs.inputs[channel])
            before = graph(source)
            refined = apply_fine_wear(part,'front',source)
            self.assertIsNot(refined,source)
            self.assertEqual(graph(source),before)
            target = next(n for n in refined.node_tree.nodes if n.type=='BSDF_PRINCIPLED')
            for channel in ('Base Color','Roughness','Metallic'):
                self.assertTrue(target.inputs[channel].is_linked)
                mixer = target.inputs[channel].links[0].from_node
                self.assertEqual(mixer.inputs[1].is_linked,linked)
                if not linked:
                    expected=bs.inputs[channel].default_value
                    expected=tuple(expected) if channel=='Base Color' else (expected,expected,expected,1)
                    self.assertEqual(tuple(mixer.inputs[1].default_value),expected)
            self.assertIs(apply_fine_wear(part,'back',source),source)

    def test_temporary_bake_copy_preserves_each_polygon_material_index(self):
        from blender_adapter import clear_scene, import_asset
        from export_assets import _joined_bake_mesh
        clear_scene();source=import_asset(fixture(True))['panel']
        before=[poly.material_index for poly in source.data.polygons]
        self.assertGreater(max(before),0)
        joined=_joined_bake_mesh([source])
        self.assertEqual([poly.material_index for poly in joined.data.polygons],before)
        self.assertEqual([poly.material_index for poly in source.data.polygons],before)

    def test_baked_color_roughness_metal_change_inside_only_and_back_unchanged(self):
        from blender_adapter import clear_scene, import_asset
        from export_assets import _prepare_atlas, _bake_maps, _joined_bake_mesh
        images = []
        sample_sets = None
        size = 128
        for enabled in (False, True):
            clear_scene()
            ob=import_asset(fixture(enabled))['panel']
            _prepare_atlas([ob],size)
            sample_sets = {'inside':[], 'outside':[], 'back':[]}
            uv=ob.data.uv_layers['AtlasUV'].data
            for face in ob.data.polygons:
                # Dense barycentric sample grid, inset from atlas triangle edges.
                if abs(face.normal.y)<.999: continue
                vs=[ob.data.vertices[i].co for i in face.vertices]
                uvs=[uv[i].uv for i in face.loop_indices]
                for ai in range(1,12):
                    for bi in range(1,12-ai):
                        a,b=ai/12,bi/12;c=1-a-b
                        point=vs[0]*a+vs[1]*b+vs[2]*c
                        tex=uvs[0]*a+uvs[1]*b+uvs[2]*c
                        group=('back' if face.normal.y>.999 else
                               'inside' if .002<point.x<.018 and .008<point.z<.045 else
                               'outside' if point.x>.055 or point.z>.07 else None)
                        if group:
                            sample_sets[group].append(4*(min(size-1,int(tex.y*size))*size+min(size-1,int(tex.x*size))))
            joined=_joined_bake_mesh([ob])
            baked=_bake_maps([joined],size,1)
            images.append({k:list(baked[k].pixels) for k in ('basecolor','roughness','metallic')})
        for group, indices in sample_sets.items():
            self.assertTrue(indices,group)
            for channel in images[0]:
                deltas=[abs(images[0][channel][i+j]-images[1][channel][i+j]) for i in indices for j in range(3)]
                if group=='inside':self.assertGreater(max(deltas),.01,(group,channel))
                else:self.assertLess(max(deltas),1e-6,(group,channel))

if __name__=='__main__':
    unittest.main()
