"""Original, dependency-free subtraction of extruded 2-D panel cuts.

Coordinates are node-local metres, with front = -Y. This is an analytic
rectangular-panel modeller, not a general mesh Boolean or an addon wrapper.
The XZ plane is partitioned into convex cells along cutter edges; occupied
Y intervals are extruded and their internal faces cancelled. A common edge
partition makes the emitted surface watertight even at notches and overlaps.
All input vertices remain at the same indices and positions. Rebuilding can
retriangulate exterior surfaces, including the unchanged opposite plane of a
shallow cavity. Cuts never move, bend or inflate the panel.
"""
from __future__ import annotations

import collections
import copy
import math
import random

from core import _cross, _dot, _sub, _uvs

MAX_PROFILE_VERTICES = 64
MAX_CUTS = 12
MAX_CELLS = 4096
MAX_MESH_VERTICES = 60000
MAX_MESH_FACES = 100000
_EPS = 1e-10
_AREA_EPS = 1e-12


def _finite_vector(value, size, label):
    if not isinstance(value, (list, tuple)) or len(value) != size:
        raise ValueError(label+' must have '+str(size)+' coordinates')
    if any(isinstance(x, bool) or not isinstance(x, (int, float)) or not math.isfinite(x) for x in value):
        raise ValueError(label+' must contain finite numeric coordinates')
    return [float(x) for x in value]


def _cross2(a, b, c):
    return (b[0]-a[0])*(c[1]-a[1])-(b[1]-a[1])*(c[0]-a[0])


def _area(polygon):
    # Translate first to avoid catastrophic cancellation away from the origin.
    a = polygon[0]
    return sum(_cross2(a, polygon[i], polygon[i+1]) for i in range(1, len(polygon)-1))/2


def _on_segment(p, a, b, epsilon=_EPS):
    length = math.dist(a, b)
    return (abs(_cross2(a, b, p)) <= epsilon*max(length, epsilon)
            and min(a[0], b[0])-epsilon <= p[0] <= max(a[0], b[0])+epsilon
            and min(a[1], b[1])-epsilon <= p[1] <= max(a[1], b[1])+epsilon)


def _intersects(a, b, c, d):
    ab_c, ab_d = _cross2(a, b, c), _cross2(a, b, d)
    cd_a, cd_b = _cross2(c, d, a), _cross2(c, d, b)
    if ab_c*ab_d < 0 and cd_a*cd_b < 0:
        return True
    return any((_on_segment(c, a, b), _on_segment(d, a, b), _on_segment(a, c, d), _on_segment(b, c, d)))


def _profile(value):
    if not isinstance(value, (list, tuple)) or not 3 <= len(value) <= MAX_PROFILE_VERTICES:
        raise ValueError('Cutter must contain 3..'+str(MAX_PROFILE_VERTICES)+' vertices')
    polygon = [_finite_vector(point, 2, 'Cutter point') for point in value]
    for i, a in enumerate(polygon):
        for b in polygon[i+1:]:
            if math.dist(a, b) <= _EPS:
                raise ValueError('Cutter has repeated or coincident vertices')
    n = len(polygon)
    for i in range(n):
        a, b = polygon[i], polygon[(i+1) % n]
        for j in range(i+1, n):
            if j == i+1 or (i == 0 and j == n-1):
                continue
            if _intersects(a, b, polygon[j], polygon[(j+1) % n]):
                raise ValueError('Cutter must be a simple, non-self-intersecting polygon')
        prev = polygon[i-1]
        if abs(_cross2(prev, a, b)) <= _EPS*math.dist(prev, b):
            # A straight continuation is harmless; reversing on an edge is not.
            if _dot(_sub([a[0], 0, a[1]], [prev[0], 0, prev[1]]), _sub([b[0], 0, b[1]], [a[0], 0, a[1]])) < 0:
                raise ValueError('Cutter doubles back along an edge')
    area = _area(polygon)
    if abs(area) <= _AREA_EPS:
        raise ValueError('Cutter has zero or unsupported tiny area')
    if area < 0:
        polygon.reverse()
    return polygon


def _contains(polygon, point, boundary=True):
    inside = False
    x, z = point
    for a, b in zip(polygon, polygon[1:]+polygon[:1]):
        if _on_segment(point, a, b):
            return boundary
        if (a[1] > z) != (b[1] > z):
            crossing = a[0]+(z-a[1])*(b[0]-a[0])/(b[1]-a[1])
            if x < crossing:
                inside = not inside
    return inside


def _bounds(part, check_mesh=False):
    if not isinstance(part, dict) or not part.get('vertices') or not part.get('faces'):
        raise ValueError('Panel must be a nonempty mesh part')
    vertices = part['vertices']
    if len(vertices) > MAX_MESH_VERTICES or len(part['faces']) > MAX_MESH_FACES:
        raise ValueError('Input mesh exceeds panel budget')
    vertices = [_finite_vector(v, 3, 'Panel vertex') for v in vertices]
    raw = part.get('panel_bounds')
    if raw is not None:
        bounds = _finite_vector(raw, 6, 'panel_bounds')
    else:
        bounds = [min(v[axis] for v in vertices) if endpoint == 0 else max(v[axis] for v in vertices) for axis in range(3) for endpoint in range(2)]
    x0, x1, y0, y1, z0, z1 = bounds
    if min(x1-x0, y1-y0, z1-z0) <= _EPS or max(abs(v) for v in bounds) > 1e5:
        raise ValueError('Panel bounds are degenerate or outside supported metric range')
    if any(not (x0-_EPS <= x <= x1+_EPS and y0-_EPS <= y <= y1+_EPS and z0-_EPS <= z <= z1+_EPS) for x, y, z in vertices):
        raise ValueError('Panel vertices lie outside panel_bounds')
    if check_mesh:
        edges = collections.Counter()
        volume = 0.
        original = not part.get('cuts')
        for face in part['faces']:
            if len(face) < 3 or any(isinstance(i, bool) or not isinstance(i, int) or not 0 <= i < len(vertices) for i in face):
                raise ValueError('Invalid panel face')
            vs = [vertices[i] for i in face]
            if original and not any(all(abs(v[axis]-end) <= _EPS for v in vs) for axis, pair in enumerate([(x0, x1), (y0, y1), (z0, z1)]) for end in pair):
                raise ValueError('Only axis-aligned rectangular panel prisms are supported')
            for i, j in zip(face, face[1:]+face[:1]):
                edges[i, j] += 1
            for k in range(1, len(vs)-1):
                normal = _cross(_sub(vs[k], vs[0]), _sub(vs[k+1], vs[0]))
                if _dot(normal, normal) <= 1e-20:
                    raise ValueError('Panel contains degenerate triangles')
                volume += _dot(vs[0], _cross(vs[k], vs[k+1]))/6
        if any(count != 1 or edges[j, i] != 1 for (i, j), count in edges.items()) or volume <= _AREA_EPS:
            raise ValueError('Panel must be closed, consistently oriented and positive-volume')
        if original and not math.isclose(volume, (x1-x0)*(y1-y0)*(z1-z0), rel_tol=1e-8, abs_tol=1e-12):
            raise ValueError('Mesh is not a complete rectangular panel prism')
    return bounds


def _cut_record(polygon, side, depth, bounds):
    if side not in ('front', 'back'):
        raise ValueError("side must be 'front' or 'back'")
    y0, y1 = bounds[2:4]
    if depth is None:
        low, high = y0, y1
    else:
        if isinstance(depth, bool) or not isinstance(depth, (int, float)) or not math.isfinite(depth) or not _EPS < depth < y1-y0-_EPS:
            raise ValueError('Numeric depth must be positive and strictly less than thickness; use None for through')
        depth = float(depth)
        low, high = (y0, y0+depth) if side == 'front' else (y1-depth, y1)
    return dict(polygon_xz=_profile(polygon), side=side, depth=depth, through=depth is None, y_range=[low, high])


def _read_cuts(part, bounds):
    raw = part.get('cuts', [])
    if not isinstance(raw, list) or len(raw) > MAX_CUTS:
        raise ValueError('Panel exceeds cut budget')
    result = []
    for cut in raw:
        if not isinstance(cut, dict):
            raise ValueError('Malformed cut metadata')
        result.append(_cut_record(cut.get('polygon_xz'), cut.get('side'), cut.get('depth'), bounds))
    return result


def point_in_panel(part, p):
    """Whether a finite node-local point lies in remaining analytic material.

    Outer boundaries belong to material; cut boundaries belong to the removed
    region. Queries use the same exact polygon extrusions as mesh construction.
    """
    x, y, z = _finite_vector(p, 3, 'Point')
    bounds = _bounds(part, check_mesh=True)
    x0, x1, y0, y1, z0, z1 = bounds
    if not (x0-_EPS <= x <= x1+_EPS and y0-_EPS <= y <= y1+_EPS and z0-_EPS <= z <= z1+_EPS):
        return False
    return not any(cut['y_range'][0]-_EPS <= y <= cut['y_range'][1]+_EPS and _contains(cut['polygon_xz'], [x, z]) for cut in _read_cuts(part, bounds))


def irregular_polygon(center_xz, radii_xz, seed, vertices=13, jaggedness=.25):
    """Deterministic simple radial profile, entirely inside the given ellipse."""
    center = _finite_vector(center_xz, 2, 'Center')
    radii = _finite_vector(radii_xz, 2, 'Radii')
    if min(radii) <= _EPS:
        raise ValueError('Radii must be positive')
    if isinstance(seed, bool) or not isinstance(seed, int):
        raise ValueError('Seed must be an integer')
    if isinstance(vertices, bool) or not isinstance(vertices, int) or not 3 <= vertices <= MAX_PROFILE_VERTICES:
        raise ValueError('vertices outside supported 3..64 range')
    if isinstance(jaggedness, bool) or not isinstance(jaggedness, (float, int)) or not math.isfinite(jaggedness) or not 0 <= jaggedness <= .85:
        raise ValueError('jaggedness must be finite in 0..0.85')
    rng = random.Random(seed)
    result = []
    for i in range(vertices):
        angle = (i+rng.uniform(-.15, .15)*jaggedness)*2*math.pi/vertices
        radius = 1-rng.uniform(0, jaggedness)
        result.append([center[0]+radii[0]*radius*math.cos(angle), center[1]+radii[1]*radius*math.sin(angle)])
    return _profile(result)


def _clean(polygon):
    result = []
    for point in polygon:
        if not result or math.dist(point, result[-1]) > _EPS:
            result.append(point)
    if len(result) > 1 and math.dist(result[0], result[-1]) <= _EPS:
        result.pop()
    return result


def _split(cell, a, b):
    length = math.dist(a, b)
    distances = [_cross2(a, b, point)/length for point in cell]
    if min(distances) >= -_EPS or max(distances) <= _EPS:
        return [cell]
    positive, negative = [], []
    for i, point in enumerate(cell):
        j = (i+1) % len(cell)
        distance, other = distances[i], distances[j]
        if distance >= -_EPS:
            positive.append(point)
        if distance <= _EPS:
            negative.append(point)
        if (distance > _EPS and other < -_EPS) or (distance < -_EPS and other > _EPS):
            t = distance/(distance-other)
            hit = [point[k]+t*(cell[j][k]-point[k]) for k in range(2)]
            positive.append(hit)
            negative.append(hit)
    result = []
    for polygon in (positive, negative):
        polygon = _clean(polygon)
        # A temporary partition sliver is not a mesh triangle: retain its
        # positive area exactly so compatible neighbors can absorb the seam.
        # Final cap and mesh validation still enforce triangle-area limits.
        if len(polygon) >= 3 and _area(polygon) > 0:
            result.append(polygon)
        else:
            raise ValueError('Cutter produces a numerically degenerate cell')
    return result


def _partition(bounds, cuts):
    x0, x1, _, _, z0, z1 = bounds
    cells = [[[x0, z0], [x1, z0], [x1, z1], [x0, z1]]]
    lines = set()
    for cut in cuts:
        polygon = cut['polygon_xz']
        for a, b in zip(polygon, polygon[1:]+polygon[:1]):
            # Split only cells touched by the finite cutter segment. Extending
            # every edge across the entire panel creates needless intersections
            # and microscopic slivers far away from the actual cut.
            key = tuple(sorted(tuple(round(v, 11) for v in p) for p in (a, b)))
            if key in lines:
                continue
            lines.add(key)
            divided = []
            for cell in cells:
                touched = (_contains(cell, a) or _contains(cell, b)
                           or any(_intersects(a, b, c, d) for c, d in zip(cell, cell[1:]+cell[:1])))
                divided.extend(_split(cell, a, b) if touched else [cell])
                if len(divided) > MAX_CELLS:
                    raise ValueError('Cut arrangement exceeds cell budget')
            cells = divided
    return cells


def _triangles(polygon):
    """Convex ear removal that retains collinear boundary subdivision points."""
    remaining = list(range(len(polygon)))
    result = []
    while len(remaining) > 3:
        found = False
        for k, current in enumerate(remaining):
            previous, following = remaining[k-1], remaining[(k+1) % len(remaining)]
            a, b, c = polygon[previous], polygon[current], polygon[following]
            if _cross2(a, b, c) <= 1e-10:
                continue
            if any(_cross2(a, b, polygon[j]) >= -_AREA_EPS and _cross2(b, c, polygon[j]) >= -_AREA_EPS and _cross2(c, a, polygon[j]) >= -_AREA_EPS for j in remaining if j not in (previous, current, following)):
                continue
            result.append([previous, current, following])
            remaining.pop(k)
            found = True
            break
        if not found:
            raise ValueError('Unsupported tiny or ill-conditioned surface triangulation')
    if _cross2(*(polygon[i] for i in remaining)) <= 1e-10:
        raise ValueError('Unsupported tiny surface triangle')
    result.append(remaining)
    return result


def _cap_mesh(cell):
    """Triangulate without discarding tiny boundary edges or moving points.

    A convex polygon can have no usable diagonal-only triangulation when two
    short boundary segments meet. A strictly interior Steiner point lets each
    short edge reach across the full cell instead of forcing a tiny corner ear.
    """
    try:
        return cell, _triangles(cell)
    except ValueError:
        center = [sum(p[k] for p in cell)/len(cell) for k in range(2)]
        if any(_cross2(a, b, center) <= 1e-10 for a, b in zip(cell, cell[1:]+cell[:1])):
            raise ValueError('Cell needs a larger compatible triangulation domain') from None
        return cell+[center], [[i, (i+1) % len(cell), len(cell)] for i in range(len(cell))]


def _union_convex(first, second):
    """Remove their shared partition edges; preserve every exterior vertex."""
    positions, edges = {}, {}
    for cell in (first, second):
        keys = []
        for point in cell:
            key = tuple(round(v, 11) for v in point)
            positions.setdefault(key, point)
            keys.append(key)
        for a, b in zip(keys, keys[1:]+keys[:1]):
            if (b, a) in edges:
                del edges[b, a]
            else:
                edges[a, b] = True
    if len(edges) == len(first)+len(second):
        return None
    outgoing = {}
    for a, b in edges:
        if a in outgoing:
            return None  # More than one loop or a pinched boundary.
        outgoing[a] = b
    start = next(iter(outgoing))
    current, outline = start, []
    for _ in range(len(edges)):
        if current not in outgoing or current in outline:
            return None
        outline.append(current)
        current = outgoing[current]
    if current != start:
        return None
    merged = [positions[key] for key in outline]
    if any(_cross2(merged[i-1], merged[i], merged[(i+1) % len(merged)]) < -_AREA_EPS for i in range(len(merged))):
        return None
    if not math.isclose(_area(merged), _area(first)+_area(second), rel_tol=1e-9, abs_tol=1e-14):
        return None
    return merged


def _coalesce_slivers(cells, cuts):
    """Remove artificial seams only between identically classified material.

    Real cutter boundaries never move or disappear. Convex union is required,
    so its interior mean remains a valid point for the extrusion classifier.
    """
    signatures = []
    levels = sorted(set(y for cut in cuts for y in cut['y_range']))
    middles = [(a+b)/2 for a, b in zip(levels, levels[1:])]
    for cell in cells:
        center = [sum(p[k] for p in cell)/len(cell) for k in range(2)]
        inside = [_contains(cut['polygon_xz'], center) for cut in cuts]
        # Cutter identity is irrelevant where a shallow cut overlaps a deeper
        # cavity. Match actual removed Y intervals, both before and after the
        # newest cut, preserving its exact incremental-volume measurement.
        signature = tuple((any(inside[i] and cut['y_range'][0] < y < cut['y_range'][1]
                               for i, cut in enumerate(cuts[:-1])),
                           any(inside[i] and cut['y_range'][0] < y < cut['y_range'][1]
                               for i, cut in enumerate(cuts))) for y in middles)
        signatures.append(signature)
    cells = list(cells)
    index = 0
    while index < len(cells):
        try:
            _cap_mesh(cells[index])
        except ValueError:
            candidates = []
            for other, candidate in enumerate(cells):
                if other == index or signatures[other] != signatures[index]:
                    continue
                merged = _union_convex(cells[index], candidate)
                if merged is not None:
                    try:
                        _cap_mesh(merged)
                        usable = True
                    except ValueError:
                        usable = False
                    candidates.append((usable, _area(merged), other, merged))
            if not candidates:
                raise ValueError('Cut creates an unsupported genuinely tiny material region') from None
            # Area alone can greedily choose an untriangulable union while a
            # smaller neighbor provides a sound cap without further merging.
            _, _, other, merged = max(candidates, key=lambda item: (item[0], item[1], -item[2]))
            cells[index] = merged
            cells.pop(other)
            signatures.pop(other)
            if other < index:
                index -= 1
            continue
        index += 1
    return cells


def _conform(cells):
    # Numerical intersections from separate clips are welded by their metric key.
    positions = {}
    for cell in cells:
        for point in cell:
            positions.setdefault(tuple(round(v, 11) for v in point), point)
    points = list(positions.values())
    result = []
    for cell in cells:
        boundary = []
        for a, b in zip(cell, cell[1:]+cell[:1]):
            segment = [p for p in points if _on_segment(p, a, b)]
            dx, dz = b[0]-a[0], b[1]-a[1]
            segment.sort(key=lambda p: (p[0]-a[0])*dx+(p[1]-a[1])*dz)
            boundary.extend(segment[:-1])
        result.append(_clean(boundary))
    return result


def carve_panel(part, polygon_xz, side='front', depth=None):
    """Return a new closed panel with a simple XZ-profile extrusion removed.

    ``depth=None`` explicitly means through. Numeric depth is measured inward
    from ``side`` and must be less than the panel thickness. A cutter may cross
    any panel edge; exterior portions are clipped naturally by classification.
    Successive cuts are rebuilt together, so overlaps subtract material once.
    Empty, nonintersecting, completely erasing and ill-conditioned cuts fail
    with ValueError instead of returning an invalid or silently altered mesh.
    """
    bounds = _bounds(part, check_mesh=True)
    cuts = _read_cuts(part, bounds)
    if len(cuts) >= MAX_CUTS:
        raise ValueError('Panel exceeds '+str(MAX_CUTS)+' cut budget')
    new_cut = _cut_record(polygon_xz, side, depth, bounds)
    cuts.append(new_cut)
    cells = _coalesce_slivers(_conform(_partition(bounds, cuts)), cuts)
    ys = sorted(set([bounds[2], bounds[3]]+[y for cut in cuts for y in cut['y_range']]))
    output = copy.deepcopy(part)
    vertices = output['vertices']
    vertex_map = {tuple(round(value, 11) for value in v): i for i, v in reversed(list(enumerate(vertices)))}

    def vertex(point):
        key = tuple(round(value, 11) for value in point)
        if key not in vertex_map:
            if len(vertices) >= MAX_MESH_VERTICES:
                raise ValueError('Generated mesh exceeds vertex budget')
            vertex_map[key] = len(vertices)
            vertices.append(list(point))
        return vertex_map[key]

    surfaces = {}

    def surface(face):
        key = tuple(sorted(face))
        if key in surfaces:
            del surfaces[key]
        else:
            surfaces[key] = face
            if len(surfaces) > MAX_MESH_FACES:
                raise ValueError('Generated mesh exceeds face budget')

    removed_volume = 0.
    remaining_volume = 0.
    for cell in cells:
        center = [sum(point[k] for point in cell)/len(cell) for k in range(2)]
        in_cuts = [cut for cut in cuts if _contains(cut['polygon_xz'], center)]
        cell_area = _area(cell)
        cap_points, triangles = _cap_mesh(cell)
        for low, high in zip(ys, ys[1:]):
            mid = (low+high)/2
            removed = [cut for cut in in_cuts if cut['y_range'][0] < mid < cut['y_range'][1]]
            if removed:
                if new_cut in removed and not any(cut is not new_cut for cut in removed):
                    removed_volume += cell_area*(high-low)
                continue
            remaining_volume += cell_area*(high-low)
            front = [vertex([x, low, z]) for x, z in cap_points]
            back = [vertex([x, high, z]) for x, z in cap_points]
            # CCW XZ has outward normal -Y on the front cap.
            for triangle in triangles:
                surface([front[i] for i in triangle])
                surface([back[i] for i in reversed(triangle)])
            for i in range(len(cell)):
                j = (i+1) % len(cell)
                surface([front[i], back[i], back[j], front[j]])
    if removed_volume <= 1e-12:
        raise ValueError('Cutter does not remove new panel material')
    if remaining_volume <= 1e-12 or not surfaces:
        raise ValueError('Cutter would erase the entire panel')
    faces = list(surfaces.values())
    materials = []
    provenance = []
    cut_material = 'glass_edge' if 'glass' in str(part.get('material', '')).lower() else 'cut_metal'
    exterior_planes = [(axis, endpoint) for axis in range(3) for endpoint in bounds[axis*2:axis*2+2]]
    for face in faces:
        points = [vertices[i] for i in face]
        exterior = any(all(abs(p[axis]-endpoint) <= _EPS for p in points) for axis, endpoint in exterior_planes)
        materials.append(part.get('material', 'metal') if exterior else cut_material)
        if exterior:
            provenance.append(dict(kind='original_surface', source_part_id=part.get('id')))
        else:
            center = [sum(p[k] for p in points)/len(points) for k in range(3)]
            owners = [i for i, cut in enumerate(cuts) if cut['y_range'][0]-_EPS <= center[1] <= cut['y_range'][1]+_EPS and _contains(cut['polygon_xz'], [center[0], center[2]])]
            provenance.append(dict(kind='cut_surface', source_part_id=part.get('id'), cut_indices=owners))
    output.update(vertices=vertices, faces=faces, uvs=_uvs(vertices, faces), face_materials=materials, face_provenance=provenance, panel_bounds=bounds, cuts=cuts)
    output['cut_math'] = dict(method='original_convex_arrangement_extrusion', units='m', cells=len(cells), remaining_volume=remaining_volume, last_removed_volume=removed_volume)
    # Return only checked meshes. Sliver configurations are rejected explicitly.
    _bounds(output, check_mesh=True)
    return output
