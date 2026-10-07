#!/usr/bin/env python3
"""Read-only independent shelf props QA. Python never generates model/material data.

Checks actual float32 triangle topology, open mouths, precise triangle distances,
assembly endpoint containment and unchanged/supported bookshelf integration.
Reports go only to the explicitly supplied verification path.
"""
import argparse
from collections import Counter
import hashlib
import json
from pathlib import Path
import sys

import numpy as np
from validate_native_glb import Glb


def triangles(part, quantize=True):
    v = np.asarray(part['vertices'], dtype=np.float32 if quantize else np.float64).astype(np.float64)
    ix = np.asarray([(f[0], f[k], f[k + 1]) for f in part['faces'] for k in range(1, len(f) - 1)])
    return v[ix]


def dot(a, b):
    return np.sum(a * b, axis=-1)


def topology(t):
    # Weld only identical exported coordinates, not a tolerance that could hide cracks.
    v, ix = np.unique(t.reshape(-1, 3), axis=0, return_inverse=True)
    ix = ix.reshape(-1, 3)
    edges = Counter((int(a), int(b)) for f in ix for a, b in zip(f, np.roll(f, -1)))
    undirected = Counter()
    for (a, b), count in edges.items():
        undirected[tuple(sorted((a, b)))] += count
    area2 = np.linalg.norm(np.cross(t[:, 1] - t[:, 0], t[:, 2] - t[:, 0]), axis=1)
    volume = float(np.sum(dot(t[:, 0], np.cross(t[:, 1], t[:, 2]))) / 6)
    return dict(vertices=len(v), triangles=len(t), edges=len(undirected),
                nonmanifold_edges=sum(n != 2 for n in undirected.values()),
                nonopposed_edges=sum(edges[(a, b)] != edges[(b, a)] for a, b in undirected),
                degenerate_triangles=int(np.sum(area2 == 0)), min_triangle_area_m2=float(area2.min() / 2),
                signed_volume_m3=volume, euler_characteristic=len(v) - len(undirected) + len(t))


def ray_vertical(t, x, y):
    a = t[:, 0]; e1 = t[:, 1] - a; e2 = t[:, 2] - a
    det = e1[:, 0] * e2[:, 1] - e1[:, 1] * e2[:, 0]
    valid = np.abs(det) > 1e-16
    u = np.divide((x - a[:, 0]) * e2[:, 1] - (y - a[:, 1]) * e2[:, 0], det,
                  out=np.zeros_like(det), where=valid)
    v = np.divide(e1[:, 0] * (y - a[:, 1]) - e1[:, 1] * (x - a[:, 0]), det,
                  out=np.zeros_like(det), where=valid)
    good = valid & (u >= -1e-7) & (v >= -1e-7) & (u + v <= 1 + 1e-7)
    z = a[:, 2] + u * e1[:, 2] + v * e2[:, 2]
    return sorted(set(round(float(k), 10) for k in z[good]), reverse=True)


def inside(point, t):
    # Closed oriented triangle solid-angle winding test, independent of source labels.
    v = t - np.asarray(point)
    lengths = np.linalg.norm(v, axis=2)
    num = dot(v[:, 0], np.cross(v[:, 1], v[:, 2]))
    den = np.prod(lengths, axis=1) + dot(v[:, 0], v[:, 1]) * lengths[:, 2] + dot(v[:, 1], v[:, 2]) * lengths[:, 0] + dot(v[:, 2], v[:, 0]) * lengths[:, 1]
    winding = float(np.sum(2 * np.arctan2(num, den)) / (4 * np.pi))
    return abs(winding) > .5


def point_segment_distance(p, a, b):
    ab = b - a
    fraction = np.clip(dot(p - a, ab) / np.maximum(dot(ab, ab), 1e-30), 0, 1)
    return np.linalg.norm(p - (a + fraction[..., None] * ab), axis=-1)


def point_triangle_distance(p, t):
    a, b, c = t[:, 0], t[:, 1], t[:, 2]
    e1, e2 = b - a, c - a
    normal = np.cross(e1, e2)
    normal /= np.linalg.norm(normal, axis=1)[:, None]
    distance = dot(p - a, normal)
    q = p - distance[:, None] * normal
    v = q - a
    d00, d01, d11 = dot(e1, e1), dot(e1, e2), dot(e2, e2)
    d20, d21 = dot(v, e1), dot(v, e2)
    denominator = d00 * d11 - d01 * d01
    u = (d11 * d20 - d01 * d21) / denominator
    w = (d00 * d21 - d01 * d20) / denominator
    within = (u >= 0) & (w >= 0) & (u + w <= 1)
    edge = np.minimum.reduce([point_segment_distance(p, a, b), point_segment_distance(p, b, c), point_segment_distance(p, c, a)])
    return np.where(within, np.abs(distance), edge)


def segment_distance(p1, q1, p2, q2):
    # Exact constrained segment/segment minimum: four boundary minima plus an
    # interior stationary point when both line parameters lie on the segments.
    u, v, w = q1 - p1, q2 - p2, p1 - p2
    a, b, c, d, e = dot(u, u), dot(u, v), dot(v, v), dot(u, w), dot(v, w)
    denom = a * c - b * b
    valid = denom > 1e-24
    s = np.divide(b * e - c * d, denom, out=np.zeros_like(denom), where=valid)
    t = np.divide(a * e - b * d, denom, out=np.zeros_like(denom), where=valid)
    within = valid & (s >= 0) & (s <= 1) & (t >= 0) & (t <= 1)
    inner = np.where(within, np.linalg.norm(w + s[:, None] * u - t[:, None] * v, axis=1), np.inf)
    return np.minimum.reduce([inner, point_segment_distance(p1, p2, q2), point_segment_distance(q1, p2, q2), point_segment_distance(p2, p1, q1), point_segment_distance(q2, p1, q1)])


def segment_crosses_triangle(p, q, t):
    direction = q - p
    e1, e2 = t[:, 1] - t[:, 0], t[:, 2] - t[:, 0]
    h = np.cross(direction, e2); det = dot(e1, h)
    valid = np.abs(det) > 1e-18
    inv = np.divide(1, det, out=np.zeros_like(det), where=valid)
    s = p - t[:, 0]; u = inv * dot(s, h)
    cross = np.cross(s, e1); v = inv * dot(direction, cross); r = inv * dot(e2, cross)
    return valid & (u >= 0) & (v >= 0) & (u + v <= 1) & (r >= 0) & (r <= 1)


def triangle_pairs_distance(a, b):
    distance = np.full(len(a), np.inf)
    intersections = np.zeros(len(a), dtype=bool)
    for i in range(3):
        distance = np.minimum(distance, point_triangle_distance(a[:, i], b))
        distance = np.minimum(distance, point_triangle_distance(b[:, i], a))
        intersections |= segment_crosses_triangle(a[:, i], a[:, (i + 1) % 3], b)
        intersections |= segment_crosses_triangle(b[:, i], b[:, (i + 1) % 3], a)
        for j in range(3):
            distance = np.minimum(distance, segment_distance(a[:, i], a[:, (i + 1) % 3], b[:, j], b[:, (j + 1) % 3]))
    return np.where(intersections, 0, distance)


def mesh_clearance(a, b, search=.02):
    # AABB pruning cannot discard any pair closer than search. Calculate exact
    # triangle feature distances for every surviving pair, in bounded batches.
    blo, bhi = b.min(axis=1), b.max(axis=1)
    distance = search; pairs = 0
    for i in range(0, len(a), 32):
        aa = a[i:i + 32]
        lo, hi = aa.min(axis=1), aa.max(axis=1)
        candidate = np.all((lo[:, None, :] - search <= bhi) & (hi[:, None, :] + search >= blo), axis=2)
        x, y = np.nonzero(candidate)
        if not len(x):
            continue
        pairs += len(x)
        distance = min(distance, float(triangle_pairs_distance(aa[x], b[y]).min()))
    return dict(min_triangle_distance_m=distance, candidate_triangle_pairs=pairs,
                search_radius_m=search, exact_minimum_found=distance < search)


def clipped_to_z(t, lower, upper):
    """QA-only triangle clipping closes the test-coverage gap at slab edges."""
    result = []
    for triangle in t:
        polygon = list(triangle)
        for z, sign in ((lower, 1), (upper, -1)):
            clipped = []
            if not polygon:
                break
            for a, b in zip(polygon, polygon[1:] + polygon[:1]):
                ia, ib = sign * (a[2] - z) >= 0, sign * (b[2] - z) >= 0
                if ia:
                    clipped.append(a)
                if ia != ib:
                    clipped.append(a + (b - a) * ((z - a[2]) / (b[2] - a[2])))
            polygon = clipped
        for i in range(1, len(polygon) - 1):
            tri = np.array([polygon[0], polygon[i], polygon[i + 1]])
            if np.linalg.norm(np.cross(tri[1] - tri[0], tri[2] - tri[0])) > 1e-20:
                result.append(tri)
    return np.asarray(result)


def obb_separation(a, b):
    ca, cb = np.array(a['center']), np.array(b['center'])
    axes_a, axes_b = np.array(a['axes']), np.array(b['axes'])
    axes = [*axes_a, *axes_b, *[np.cross(x, y) for x in axes_a for y in axes_b]]
    separations = []
    for axis in axes:
        if np.linalg.norm(axis) < 1e-10:
            continue
        axis /= np.linalg.norm(axis)
        ra = np.sum(np.abs(axes_a @ axis) * a['half'])
        rb = np.sum(np.abs(axes_b @ axis) * b['half'])
        separations.append(float(abs(np.dot(cb - ca, axis)) - ra - rb))
    return max(separations)


def main():
    ap = argparse.ArgumentParser(description=__doc__)
    ap.add_argument('--out', required=True, type=Path)
    ap.add_argument('--shelf-source', required=True, type=Path)
    ap.add_argument('--report', required=True, type=Path)
    args = ap.parse_args()
    report = dict(output=str(args.out), checks=[], issues=[], topology={}, exported={}, openings={}, lantern={}, integration={})
    def check(condition, text):
        report['checks'].append(dict(pass_=bool(condition), description=text))
        if not condition:
            report['issues'].append(text)
    scene = json.loads((args.out / 'scene.json').read_text())
    parts = {p['id']: p for p in scene['parts']}
    items = {x['id']: x for x in scene['metadata']['items']}
    for pid, part in parts.items():
        t = triangles(part); stat = topology(t); report['topology'][pid] = stat
        check(np.isfinite(t).all() and stat['nonmanifold_edges'] == stat['nonopposed_edges'] == stat['degenerate_triangles'] == 0 and stat['signed_volume_m3'] > 0, pid + ': positive-volume closed oriented nondegenerate float32 surface')
        for field in ('uvs', 'texture_uvs', 'normals'):
            check(all(np.isfinite(np.asarray(face, dtype=np.float32)).all() for face in part[field]), pid + ': finite float32 ' + field)
    for item_id, item in items.items():
        group = item['grab']['group_part_ids']
        actual = [p['id'] for p in scene['parts'] if p['item_id'] == item_id]
        check(set(group) == set(actual) and len(group) == len(set(group)), item_id + ': exact nonempty grab group')
        v = np.concatenate([np.array(parts[x]['vertices']) for x in group])
        obb = item['obb']; a = np.array(obb['axes']); local = (v - obb['center']) @ a.T
        check(np.allclose(a @ a.T, np.eye(3), atol=1e-10) and np.linalg.det(a) > 0 and np.all(abs(local) <= np.array(obb['half']) + 1e-10), item_id + ': OBB contains every vertex with proper axes')
    for kind, floor in [('mug', .005), ('bottle', .010)]:
        item = items['prop.' + kind]; scale = scene['recipe'][kind + '_scale']
        t = np.concatenate([triangles(parts[p]) for p in item['grab']['group_part_ids']])
        x, y, z = item['pivot']; hits = ray_vertical(t, x, y)
        report['openings'][kind] = dict(vertical_center_hit_heights_m=hits, first_expected_m=z + floor * scale)
        # The lightly dented lathe moves the axial vertex radially by fractions
        # of a mm; its shallow floor cone therefore differs at the exact pivot
        # by sub-micron amounts. One micron still separates it unambiguously
        # from any erroneous cap at the mouth, >80 mm above the floor.
        check(hits and abs(hits[0] - (z + floor * scale)) < 1e-6 * scale, kind + ': mouth center ray first reaches actual interior floor')
    lantern_scale = scene['recipe']['lantern_scale']
    globe = triangles(parts['prop.lantern.globe'])
    targets = {'lower': [triangles(parts['prop.lantern.reservoir']), triangles(parts['prop.lantern.burner_cup'])], 'upper': [triangles(parts['prop.lantern.hood']), triangles(parts['prop.lantern.hood_roll'])]}
    report['lantern']['guard_contacts'] = {}
    report['lantern']['glass_clearance'] = {}
    for pid, part in parts.items():
        if part['role'] == 'GlobeGuard':
            # Source tube end cap centres are the final two vertices; winding
            # confirms they are embedded in real solid metal, not merely nearby.
            ends = np.asarray(part['vertices'][-2:], dtype=np.float32).astype(float)
            result = {}
            for label, point in zip(('lower', 'upper'), ends):
                result[label] = dict(position=point.tolist(), inside_support_union=any(inside(point, t) for t in targets[label]))
                check(result[label]['inside_support_union'], pid + ': ' + label + ' endpoint embedded in actual metal support')
            report['lantern']['guard_contacts'][pid] = result
        if part['role'] in ('GlobeGuard', 'LanternSideRail'):
            t = triangles(part)
            # Both boundaries avoid intentionally embedded metal/glass seats.
            selected = clipped_to_z(t, .064 * lantern_scale, .172 * lantern_scale)
            distance = mesh_clearance(selected, globe, .025 * lantern_scale)
            distance['free_interval_z_m'] = [.064 * lantern_scale, .172 * lantern_scale]
            distance['tested_triangles'] = len(selected)
            report['lantern']['glass_clearance'][pid] = distance
            check(distance['min_triangle_distance_m'] > 1e-6, pid + ': positive glass separation through free cage span')
    check(scene['metadata']['lantern']['unlit'] is True and scene['metadata']['lantern']['emissive'] is False and scene['metadata']['engine_physics'] is False, 'unlit lantern and no engine physics metadata')
    for file in ['shelf-props.glb', 'mug.glb', 'bottle.glb', 'lantern.glb', 'shelf-with-props.glb']:
        glb = Glb(args.out / file); info = dict(prop_meshes=0, vertices=0, triangles=0, invalid_attributes=0, topology_failures=[])
        check(not glb.doc.get('extensions', {}).get('KHR_lights_punctual'), file + ': contains no light extension')
        for mat in glb.doc['materials']:
            check(not any(mat.get('emissiveFactor', [0, 0, 0])) and 'emissiveTexture' not in mat, file + ': nonemissive material ' + mat.get('name', ''))
        for mesh in glb.doc['meshes']:
            if not mesh['name'].startswith('prop.'):
                continue
            info['prop_meshes'] += 1
            for primitive in mesh['primitives']:
                attr = {name: np.array(glb.accessor(ix)) for name, ix in primitive['attributes'].items()}
                for name in ('POSITION', 'NORMAL', 'TANGENT', 'TEXCOORD_0', 'TEXCOORD_1'):
                    check(name in attr and np.isfinite(attr[name]).all(), file + ': ' + mesh['name'] + ' finite exported ' + name)
                indices = np.array(glb.accessor(primitive['indices'])).ravel().reshape(-1, 3)
                t = attr['POSITION'][indices]; info['vertices'] += len(attr['POSITION']); info['triangles'] += len(t)
                stat = topology(t)
                if stat['nonmanifold_edges'] or stat['nonopposed_edges'] or stat['degenerate_triangles'] or stat['signed_volume_m3'] <= 0:
                    info['topology_failures'].append(dict(mesh=mesh['name'], **stat))
        check(not info['topology_failures'], file + ': all exported prop float32 surfaces closed and oriented')
        report['exported'][file] = info
    source = json.loads(args.shelf_source.read_text())
    combined = json.loads((args.out / 'shelf-with-props.scene.json').read_text())
    original = {p['id']: p for p in source['parts']}; integrated = {p['id']: p for p in combined['parts']}
    changed = [pid for pid, p in original.items() if integrated.get(pid) != p]
    report['integration']['original_parts'] = len(original)
    report['integration']['changed_original_parts'] = changed
    check(not changed and len(integrated) == len(original) + len(parts), 'all original shelf/book/crate parts remain structurally identical')
    orig_items = {x['id']: x for x in source['metadata']['items']}
    all_items = {x['id']: x for x in combined['metadata']['items']}
    check(all(all_items[k] == v for k, v in orig_items.items()), 'all original item transforms, OBBs and grab groups unchanged')
    bays = {x['index']: x for x in source['metadata']['compartments']}
    report['integration']['props'] = {}
    for pid in items:
        item = all_items[pid]; bay = bays[item['compartment']]
        t = np.concatenate([triangles(integrated[p]) for p in item['grab']['group_part_ids']])
        bottom = float(t[:, :, 2].min()); support = triangles(integrated[item['support_id']])
        bottom_vertices = np.unique(t.reshape(-1, 3), axis=0)
        bottom_vertices = bottom_vertices[abs(bottom_vertices[:, 2] - bottom) < 1e-7]
        gaps = []
        for p in bottom_vertices:
            hits = ray_vertical(support, p[0], p[1])
            gaps.append(None if not hits else float(p[2] - hits[0]))
        separations = {oid: obb_separation(item['obb'], other['obb']) for oid, other in all_items.items() if oid != pid}
        invalid = {k: v for k, v in separations.items() if v < -1e-7}
        report['integration']['props'][pid] = dict(actual_bottom_m=bottom, metadata_bottom_plane_z=item['bottom_plane_z'], compartment_bottom_m=bay['bottom'], support_samples=len(gaps), min_support_gap_m=min(g for g in gaps if g is not None), max_support_gap_m=max(g for g in gaps if g is not None), unsupported_samples=gaps.count(None), obb_collisions=invalid, minimum_obb_separation_m=min(separations.values()))
        check(not invalid, pid + ': no positive-volume OBB collision with any other shelf item')
        check(all(g is not None and abs(g) < 2e-6 for g in gaps), pid + ': all bottom vertices rest on actual shelf triangles')
        check(abs(item['bottom_plane_z'] - bottom) < 2e-6, pid + ': integrated bottom_plane_z follows translated support plane')
    report['source_sha256'] = hashlib.sha256((args.out / 'scene.json').read_bytes()).hexdigest()
    report['status'] = 'PASS' if not report['issues'] else 'FAIL'
    args.report.write_text(json.dumps(report, indent=2) + '\n')
    print(json.dumps({k: report[k] for k in ('status', 'issues', 'openings', 'lantern', 'integration')}, indent=2))
    return 0 if not report['issues'] else 1


if __name__ == '__main__':
    sys.exit(main())
