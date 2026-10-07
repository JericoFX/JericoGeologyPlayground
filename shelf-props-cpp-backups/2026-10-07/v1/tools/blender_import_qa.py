"""Read-only Blender importer audit; never modifies/saves the imported assets."""
import argparse
from collections import Counter
import hashlib
import json
import math
from pathlib import Path
import struct
import sys

import bpy
from mathutils import Vector
from mathutils.kdtree import KDTree


def f32(x):
    return struct.unpack('<f', struct.pack('<f', x))[0]


ap = argparse.ArgumentParser()
ap.add_argument('--out', required=True)
ap.add_argument('--report', required=True)
ap.add_argument('--files', default='shelf-props.glb,shelf-props.obj,shelf-with-props.glb')
args = ap.parse_args(sys.argv[sys.argv.index('--') + 1:])
out = Path(args.out).resolve()
report = {'read_only': True, 'blender_version': bpy.app.version_string, 'files': [], 'issues': []}

for filename, scene_file in [('shelf-props.glb', 'scene.json'), ('shelf-props.obj', 'scene.json'), ('shelf-with-props.glb', 'shelf-with-props.scene.json')]:
    if filename not in args.files.split(','):
        continue
    bpy.ops.wm.read_factory_settings(use_empty=True)
    asset_hash = hashlib.sha256((out / filename).read_bytes()).hexdigest()
    source_hash = hashlib.sha256((out / scene_file).read_bytes()).hexdigest()
    source = json.loads((out / scene_file).read_text())
    parts = {p['id']: p for p in source['parts']}
    if filename.endswith('.glb'):
        bpy.ops.import_scene.gltf(filepath=str(out / filename))
    else:
        bpy.ops.wm.obj_import(filepath=str(out / filename), forward_axis='Y', up_axis='Z')
    meshes = [o for o in bpy.context.scene.objects if o.type == 'MESH']
    stat = {'file': filename, 'asset_sha256': asset_hash, 'source_sha256': source_hash, 'meshes': len(meshes), 'expected_meshes': len(parts), 'triangles': 0, 'modifiers': 0, 'max_position_error_m': 0, 'materials': len(bpy.data.materials), 'texture_sizes': {}, 'mesh_checks': []}
    issues = []
    if len(meshes) != len(parts):
        issues.append('mesh count differs')
    if any(o.type == 'LIGHT' for o in bpy.context.scene.objects):
        issues.append('asset imported unexpected light')
    for obj in meshes:
        pid = obj.name
        if pid not in parts:
            issues.append('unexpected object ' + pid)
            continue
        p = parts[pid]; mesh = obj.data; stat['modifiers'] += len(obj.modifiers)
        mesh.calc_loop_triangles()
        expected_triangles = sum(len(f) - 2 for f in p['faces'])
        if len(mesh.loop_triangles) != expected_triangles:
            issues.append(pid + ': triangle count differs')
        stat['triangles'] += len(mesh.loop_triangles)
        tree = KDTree(len(p['vertices']))
        for i, v in enumerate(p['vertices']):
            tree.insert(tuple(f32(c) for c in v), i)
        tree.balance()
        max_error = 0; hit = set()
        for v in mesh.vertices:
            world = obj.matrix_world @ v.co
            _, index, distance = tree.find(world)
            max_error = max(max_error, distance); hit.add(index)
        if max_error > 5e-7:
            issues.append(pid + ': imported positions differ by ' + str(max_error))
        stat['max_position_error_m'] = max(stat['max_position_error_m'], max_error)
        if not mesh.uv_layers:
            issues.append(pid + ': UV layer missing')
        for uv_layer in mesh.uv_layers:
            if not all(math.isfinite(c) for uv in uv_layer.data for c in uv.uv):
                issues.append(pid + ': nonfinite UV')
        if not all(math.isfinite(c) for n in mesh.corner_normals for c in n.vector):
            issues.append(pid + ': nonfinite normal')
        if not mesh.materials or any(x is None for x in mesh.materials):
            issues.append(pid + ': missing material')
        for mat in mesh.materials:
            image_nodes = [n for n in mat.node_tree.nodes if n.type == 'TEX_IMAGE' and n.image]
            if not image_nodes:
                issues.append(pid + ': missing imported image textures')
            expected_size = p['textures']['resolution']
            for n in image_nodes:
                size = list(n.image.size)
                stat['texture_sizes'][n.image.name] = size
                if size != [expected_size, expected_size]:
                    issues.append(pid + ': incorrect texture size ' + str(size))
        stat['mesh_checks'].append({'id': pid, 'triangles': len(mesh.loop_triangles), 'source_vertices': len(p['vertices']), 'imported_vertices': len(mesh.vertices), 'max_position_error_m': max_error, 'uv_layers': len(mesh.uv_layers), 'custom_normals': mesh.has_custom_normals, 'materials': [m.name for m in mesh.materials]})
    if stat['modifiers']:
        issues.append('unexpected asset modifiers')
    if hashlib.sha256((out / filename).read_bytes()).hexdigest() != asset_hash:
        issues.append('asset changed during audit')
    stat['status'] = 'PASS' if not issues else 'FAIL'
    stat['issues'] = issues
    report['issues'].extend([filename + ': ' + issue for issue in issues])
    report['files'].append(stat)
    print('IMPORT_AUDIT', filename, stat['status'], len(meshes), stat['triangles'], stat['max_position_error_m'], 'modifiers', stat['modifiers'])

report['status'] = 'PASS' if not report['issues'] else 'FAIL'
Path(args.report).write_text(json.dumps(report, indent=2) + '\n')
print('IMPORT_AUDIT_FINAL', report['status'])
if report['issues']:
    print(json.dumps(report['issues'], indent=2))
    raise RuntimeError('Import audit failed')
