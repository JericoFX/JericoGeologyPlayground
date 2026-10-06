#!/usr/bin/env python3
"""Physics renderer contracts; pass --blender to verify actual scene objects.

python3 scripts/test_physics_renderer.py --blender
"""
import importlib.util
import json
import pathlib
import subprocess
import sys
import unittest

HERE = pathlib.Path(__file__).resolve().parent
RENDERER = HERE / 'render_physics.py'
BLENDER = '--blender' in sys.argv
if BLENDER:
    sys.argv.remove('--blender')


def load_renderer():
    spec = importlib.util.spec_from_file_location('render_physics', RENDERER)
    module = importlib.util.module_from_spec(spec)
    spec.loader.exec_module(module)
    return module


class RendererContract(unittest.TestCase):
    def test_default_is_production_carrier_and_diagnostics_are_explicit(self):
        m = load_renderer()
        a = m.parse_args(['--input', 'rest', '--output', 'rest.png'])
        b = m.parse_args(['--input', 'tear', '--output', 'tear.png'])
        self.assertEqual(a.representation, 'carrier')
        self.assertEqual(m.camera_config(a), m.camera_config(b))
        self.assertEqual((a.width, a.height), (1100, 900))
        for rep in ('carrier', 'curves', 'combined'):
            self.assertEqual(m.parse_args(['--input', 'x', '--output', 'y', '--representation', rep]).representation, rep)

    def test_nonfinite_export_is_rejected(self):
        with self.assertRaisesRegex(ValueError, 'finite'):
            load_renderer().validate_frame({'particles': [{'id': 1, 'p': [1, float('nan'), 2], 'rest': [1, 2, 3]}]})

    def test_duplicate_particle_ids_are_rejected(self):
        p = {'id': 1, 'p': [0, 0, 0]}
        with self.assertRaisesRegex(ValueError, 'Duplicate'):
            load_renderer().validate_frame({'particles': [p, p]})

    def test_cut_edges_are_not_rendered_by_graph_fallback(self):
        frame = {'edges': [{'id': 5, 'active': True}, {'id': 6, 'active': False}]}
        self.assertEqual([e['id'] for e in load_renderer().active_edges(frame)], [5])

    def test_numeric_class_ids_override_width_heuristics(self):
        m = load_renderer()
        for kind, name in enumerate(('tenuous', 'normal', 'monstrous')):
            self.assertEqual(m.class_name(kind, 0.01), name)
            self.assertEqual(m.class_name(name, 0.01), name)

    def test_anchor_zero_is_a_fixed_world_anchor(self):
        m = load_renderer()
        self.assertTrue(m.is_anchor({'anchor': 0}))
        self.assertFalse(m.is_anchor({'anchor': -1}))
        self.assertTrue(m.is_anchor({'anchor': True}))
        self.assertFalse(m.is_anchor({'anchor': False}))

    @unittest.skipUnless(BLENDER, 'Use --blender for native scene contracts')
    def test_actual_representation_objects_and_style_are_isolated(self):
        code = r'''
import pathlib, tempfile, struct, sys, bpy
sys.path.insert(0, SCRIPT_DIR)
import render_physics as r
with tempfile.TemporaryDirectory() as td:
    p=pathlib.Path(td)
    (p/'mesh.obj').write_text('v -2 0 1\nv -2 1 1\nv -2 1 2\nv -2 0 2\nvt 0 0\nvt 1 0\nvt 1 1\nvt 0 1\nf 1/1 2/2 3/3\nf 1/1 3/3 4/4\n')
    (p/'coverage.tga').write_bytes(struct.pack('<BBBHHBHHHHBB',0,0,2,0,0,0,0,0,4,4,32,8)+bytes([150,150,150,255])*16)
    frame={'tick':417,'particles':[{'id':0,'p':[-2,0,1],'rest':[-2,0,1],'anchor':0}], 'curves':[{'kind':0,'width':.001,'points':[[-2,0,1],[-2,1,2]]}], 'edges':[], 'events':[]}
    styles=set()
    for rep in ('carrier','curves','combined'):
        args=r.parse_args(['--input',td,'--output',str(p/'out.png'),'--representation',rep])
        report=r.build_scene(args,frame);styles.add(report['style_sha256'])
        carriers=[o for o in bpy.data.objects if o.get('role')=='actual_exported_carrier']
        strands=[o for o in bpy.data.objects if o.get('role')=='actual_exported_strands']
        assert len(carriers)==(0 if rep=='curves' else 1), (rep,carriers)
        assert len(strands)==(0 if rep=='carrier' else 3), (rep,strands)
        assert report['representation']==rep
        if carriers:
            mat=carriers[0].data.materials[0]; links=list(mat.node_tree.nodes['Principled BSDF'].inputs['Alpha'].links)
            assert len(links)==1 and links[0].from_socket.name=='Color'
    assert len(styles)==1, 'Representation changed camera/light/exposure/material settings'
    empty=p/'missing-carrier';empty.mkdir()
    args=r.parse_args(['--input',str(empty),'--output',str(p/'empty.png')])
    try:
        r.build_scene(args,frame)
    except ValueError as error:
        assert 'requires' in str(error)
    else:
        raise AssertionError('Carrier-only input without mesh/atlas must fail visibly')
    print('REPRESENTATION_CONTRACT_OK')
'''.replace('SCRIPT_DIR', repr(str(HERE)))
        result = subprocess.run(['blender', '-b', '--python-expr', code], capture_output=True, text=True)
        self.assertEqual(result.returncode, 0, result.stdout + result.stderr)
        self.assertIn('REPRESENTATION_CONTRACT_OK', result.stdout, result.stdout + result.stderr)


if __name__ == '__main__':
    unittest.main()
