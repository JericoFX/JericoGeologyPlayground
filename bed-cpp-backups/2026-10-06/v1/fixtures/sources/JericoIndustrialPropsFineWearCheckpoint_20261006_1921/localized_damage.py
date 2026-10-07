"""Original, stdlib-only local damage selection and material edge masks.

All points and regions use the selected part's node-local metres. Front means
-Y and back means +Y; callers provide an explicit signed face label. The CPU
edge-distance mask is an art-direction aid, not a curvature/AO simulation.
"""
from __future__ import annotations

import copy
import math


def _number(value, name, minimum=None, maximum=None):
    if isinstance(value, bool) or not isinstance(value, (int, float)):
        raise ValueError(name + ' must be finite and numeric')
    try:
        value = float(value)
    except OverflowError:
        raise ValueError(name + ' is outside the finite numeric range') from None
    if not math.isfinite(value):
        raise ValueError(name + ' must be finite and numeric')
    if (minimum is not None and value < minimum) or (maximum is not None and value > maximum):
        raise ValueError(name + ' outside supported range')
    return float(value)


def _vector(value, name, length=3):
    if not isinstance(value, (list, tuple)) or len(value) != length:
        raise ValueError(name + ' must contain ' + str(length) + ' coordinates')
    return [_number(v, name) for v in value]


def _region(region):
    if not isinstance(region, dict):
        raise ValueError('region must be a box or sphere object')
    if region.get('shape') == 'box':
        low, high = _vector(region.get('min'), 'region.min'), _vector(region.get('max'), 'region.max')
        if any(a >= b for a, b in zip(low, high)):
            raise ValueError('region box must have positive extent on each axis')
        return {'shape': 'box', 'min': low, 'max': high}
    if region.get('shape') == 'sphere':
        center = _vector(region.get('center'), 'region.center')
        radius = _number(region.get('radius'), 'region.radius', 0)
        if radius <= 0:
            raise ValueError('region.radius must be positive')
        return {'shape': 'sphere', 'center': center, 'radius': radius}
    raise ValueError('region.shape must be box or sphere')


def _selection(selection):
    if not isinstance(selection, dict) or not isinstance(selection.get('part_id'), str) or not selection['part_id']:
        raise ValueError('selection.part_id must name exactly one part')
    if selection.get('side') not in ('front', 'back'):
        raise ValueError('selection.side must be front or back')
    if not isinstance(selection.get('override_protection', False), bool):
        raise ValueError('override_protection must be an explicit boolean')
    result = dict(selection)
    result['region'] = _region(selection.get('region'))
    result['feather_m'] = _number(selection.get('feather_m', 0), 'feather_m', 0)
    protected = selection.get('protected_regions', [])
    if not isinstance(protected, list) or len(protected) > 4096:
        raise ValueError('protected_regions must be a list with at most 4096 regions')
    result['protected_regions'] = [_region(r) for r in protected]
    return result


def smootherstep(value):
    """Clamped C2-continuous interpolation, used for region and edge falloff."""
    t = max(0., min(1., _number(value, 'smootherstep input')))
    return t*t*t*(t*(t*6-15)+10)


def _interior_distance(point, region):
    if region['shape'] == 'sphere':
        return region['radius'] - math.dist(point, region['center'])
    return min([p-low for p, low in zip(point, region['min'])] +
               [high-p for p, high in zip(point, region['max'])])


def _selection_weight(part_id, side, point, selected):
    if part_id != selected['part_id'] or side != selected['side']:
        return 0.
    distance = _interior_distance(point, selected['region'])
    if distance < 0:
        return 0.
    if not selected.get('override_protection', False):
        if any(_interior_distance(point, region) >= 0 for region in selected['protected_regions']):
            return 0.
    feather = selected['feather_m']
    return smootherstep(distance / feather) if feather else 1.


def selection_weight(part_id, side, point, selection):
    """Exact part/face selector multiplied by a bounded 3-D physical region mask.

    Side is a signed label, never abs(normal): a front selection cannot affect
    the back. Optional protected_regions default to zero unless an explicit
    boolean override_protection=True is supplied.
    """
    if side not in ('front', 'back'):
        raise ValueError('query side must be front or back')
    return _selection_weight(part_id, side, _vector(point, 'point'), _selection(selection))


def _bounds(part):
    if 'panel_bounds' in part:
        bounds = _vector(part['panel_bounds'], 'panel_bounds', 6)
    else:
        vertices = [_vector(v, 'vertex') for v in part.get('vertices', [])]
        if not vertices:
            raise ValueError('selected part has no panel vertices')
        bounds = [value for axis in range(3) for value in
                  (min(v[axis] for v in vertices), max(v[axis] for v in vertices))]
    if any(bounds[2*i] >= bounds[2*i+1] for i in range(3)):
        raise ValueError('selected part has degenerate panel bounds')
    return bounds


def _segment_distance(point, a, b):
    dx, dz = b[0]-a[0], b[1]-a[1]
    length2 = dx*dx+dz*dz
    t = max(0., min(1., ((point[0]-a[0])*dx+(point[1]-a[1])*dz)/length2)) if length2 else 0.
    return math.hypot(point[0]-a[0]-t*dx, point[1]-a[1]-t*dz)


def _surface_segments(part, side):
    """Mesh cap boundary, removing shared triangulation/arrangement edges."""
    bounds = _bounds(part)
    y = bounds[2] if side == 'front' else bounds[3]
    edges = {}
    vertices = part['vertices']
    for face in part['faces']:
        if all(abs(vertices[i][1]-y) <= 1e-10 for i in face):
            for a, b in zip(face, face[1:]+face[:1]):
                key = tuple(sorted((a, b)))
                edges[key] = edges.get(key, 0)+1
    return [[[vertices[a][0], vertices[a][2]], [vertices[b][0], vertices[b][2]]]
            for (a, b), count in edges.items() if count == 1]


def edge_distance(part, point, side=None):
    """XZ distance to actual face outline, including holes but no diagonals.

    If side is omitted, a point on +Y selects back; other points select front.
    Material masks always pass their signed side explicitly.
    """
    x, y, z = _vector(point, 'point')
    bounds = _bounds(part)
    if side is None:
        side = 'back' if abs(y-bounds[3]) <= 1e-10 else 'front'
    if side not in ('front', 'back'):
        raise ValueError('query side must be front or back')
    segments = _surface_segments(part, side)
    return min((_segment_distance([x, z], a, b) for a, b in segments), default=math.inf)


def _edge_weight(part, side, point, selected, segments):
    bounds = _bounds(part)
    plane = bounds[2] if side == 'front' else bounds[3]
    if abs(point[1]-plane) > 1e-10:
        return 0.
    if any(point[axis] < bounds[2*axis]-1e-10 or point[axis] > bounds[2*axis+1]+1e-10 for axis in range(3)):
        return 0.
    weight = _selection_weight(part['id'], side, point, selected)
    if not weight or not segments:
        return 0.
    if part.get('cuts'):
        from damage_math import point_in_panel
        if not point_in_panel(part, point):
            return 0.
    distance = min(_segment_distance([point[0], point[2]], a, b) for a, b in segments)
    return selected['amount']*weight*(1-smootherstep(distance/selected['width_m']))


def edge_wear_weight(part, side, point, selection):
    """CPU outline falloff times exact part/face/region and support masks."""
    selected = _selection(selection)
    selected['width_m'] = _number(selected.get('width_m', .015), 'width_m', 0)
    if selected['width_m'] <= 0:
        raise ValueError('width_m must be positive')
    selected['amount'] = _number(selected.get('amount', 1), 'amount', 0, 1)
    if side not in ('front', 'back'):
        raise ValueError('query side must be front or back')
    return _edge_weight(part, side, _vector(point, 'point'), selected, _surface_segments(part, side))


EDGE_WEAR_MATERIALS = ('painted_steel', 'dark_steel', 'iron', 'rust')
MAX_OPERATIONS = 64
MAX_CUTS_PER_PART = 12
MAX_SAMPLES_PER_AXIS = 64


def _integer(value, name, minimum, maximum):
    if isinstance(value, bool) or not isinstance(value, int) or not minimum <= value <= maximum:
        raise ValueError(name + ' outside supported integer budget')
    return value


def _operation(operation, part):
    selected = _selection(operation)
    if selected.get('type') not in ('cut', 'crack', 'edge_wear'):
        raise ValueError('operation.type must be cut, crack, or edge_wear')
    seed = selected.get('seed', 29)
    if isinstance(seed, bool) or not isinstance(seed, int):
        raise ValueError('seed must be an integer')
    selected['seed'] = seed
    if selected['type'] == 'cut':
        thickness = _bounds(part)[3]-_bounds(part)[2]
        depth = selected.get('depth', thickness*.25)
        if depth is not None:
            depth = _number(depth, 'depth', 0)
            if not 1e-10 < depth < thickness-1e-10:
                raise ValueError('numeric depth must be positive and less than thickness; use explicit None for through')
        selected['depth'] = depth
        selected['vertices'] = _integer(selected.get('vertices', 13), 'vertices', 3, 64)
        selected['jaggedness'] = _number(selected.get('jaggedness', .2), 'jaggedness', 0, .85)
    elif selected['type'] == 'crack':
        thickness = _bounds(part)[3]-_bounds(part)[2]
        selected['length_m'] = _number(selected.get('length_m'), 'crack length_m', 0)
        selected['width_m'] = _number(selected.get('width_m'), 'crack width_m', .001)
        if selected['length_m'] < 4*selected['width_m']:
            raise ValueError('crack length_m must be at least four times width_m')
        selected['depth_m'] = _number(selected.get('depth_m', thickness*.25), 'crack depth_m', 0)
        if not 1e-10 < selected['depth_m'] < thickness-1e-10:
            raise ValueError('crack depth_m must be positive and strictly less than panel thickness; through cracks are unsupported')
        selected['branches'] = _integer(selected.get('branches', 2), 'crack branches', 0, 2)
        selected['segments'] = _integer(selected.get('segments', 6), 'crack segments', 2, 12)
        selected['direction_xz'] = _vector(selected.get('direction_xz', [0, 1]), 'crack direction_xz', 2)
        direction_length = math.hypot(*selected['direction_xz'])
        if direction_length <= 0 or not math.isfinite(direction_length):
            raise ValueError('crack direction_xz must have a finite nonzero length')
        low, high = _region_bounds(selected['region'])
        default_center = [(low[0]+high[0])/2, (low[2]+high[2])/2]
        selected['center_xz'] = _vector(selected.get('center_xz', default_center), 'crack center_xz', 2)
    else:
        if part.get('material') not in EDGE_WEAR_MATERIALS:
            raise ValueError('edge_wear requires a supported material base: '+', '.join(EDGE_WEAR_MATERIALS))
        selected['width_m'] = _number(selected.get('width_m', .015), 'width_m', 0)
        if selected['width_m'] <= 0:
            raise ValueError('width_m must be positive')
        if 'fine_breakup' in selected:
            if not isinstance(selected['fine_breakup'], bool):
                raise ValueError('fine_breakup must be an explicit boolean')
            if selected['fine_breakup'] and selected['side'] != 'front':
                raise ValueError('fine_breakup supports front only')
        selected['amount'] = _number(selected.get('amount', 1), 'amount', 0, 1)
        selected['samples_per_axis'] = _integer(selected.get('samples_per_axis', 17), 'samples_per_axis', 2, MAX_SAMPLES_PER_AXIS)
    return selected


def _region_bounds(region):
    if region['shape'] == 'box':
        return region['min'], region['max']
    return ([v-region['radius'] for v in region['center']],
            [v+region['radius'] for v in region['center']])


def _cutter(part, selected):
    """Return an inscribed XZ prism, clipped to the region's physical Y extent.

    A spherical selection is conservatively inscribed at the widest Y offset
    of the prism. It is intentionally not a spherical CSG Boolean.
    """
    from damage_math import irregular_polygon
    x0, x1, y0, y1, z0, z1 = _bounds(part)
    low, high = _region_bounds(selected['region'])
    face_y = y0 if selected['side'] == 'front' else y1
    if not low[1] <= face_y <= high[1]:
        return None
    depth = selected['depth']
    requested = y1-y0 if depth is None else depth
    applied_depth = min(requested, high[1]-y0 if selected['side'] == 'front' else y1-low[1])
    if applied_depth <= 1e-10:
        return None
    ymin, ymax = (y0, y0+applied_depth) if selected['side'] == 'front' else (y1-applied_depth, y1)
    if selected['region']['shape'] == 'sphere':
        center = selected['region']['center']
        radius = selected['region']['radius']
        dy = max(abs(ymin-center[1]), abs(ymax-center[1]))
        radius2 = radius*radius-dy*dy
        if radius2 <= 1e-20:
            return None
        radius_xz = math.sqrt(radius2)
        center_xz, radii_xz = [center[0], center[2]], [radius_xz, radius_xz]
    else:
        center_xz = [(low[0]+high[0])/2, (low[2]+high[2])/2]
        radii_xz = [(high[0]-low[0])/2, (high[2]-low[2])/2]
    if center_xz[0]+radii_xz[0] <= x0 or center_xz[0]-radii_xz[0] >= x1 or center_xz[1]+radii_xz[1] <= z0 or center_xz[1]-radii_xz[1] >= z1:
        return None
    polygon = irregular_polygon(center_xz, radii_xz, selected['seed'], vertices=selected['vertices'], jaggedness=selected['jaggedness'])
    through = depth is None and applied_depth >= y1-y0
    return {'polygon_xz': polygon, 'depth': None if through else applied_depth,
            'min': [min(p[0] for p in polygon), ymin, min(p[1] for p in polygon)],
            'max': [max(p[0] for p in polygon), ymax, max(p[1] for p in polygon)]}



def _crack_cutters(part, selected):
    """Require all complete finite-width profiles to fit a convex 3-D region.

    Checking both extruded Y endpoints at every profile vertex suffices for
    the whole prism because box and sphere selections are convex. Profiles
    are never silently shrunk, shifted, or depth-clipped to fit the region.
    """
    from crack_math import crack_profiles
    profiles = crack_profiles(selected['center_xz'], selected['length_m'],
                              selected['width_m'], selected['depth_m'], selected['seed'],
                              direction_xz=selected['direction_xz'],
                              branches=selected['branches'], segments=selected['segments'])
    if len(profiles) != 2+selected['branches'] or len(profiles) > 4:
        raise ValueError('crack profile output exceeds the expected geometric budget')
    bounds = _bounds(part)
    cutters = []
    for profile in profiles:
        depth = profile['depth']
        low_y, high_y = ((bounds[2], bounds[2]+depth) if selected['side'] == 'front'
                         else (bounds[3]-depth, bounds[3]))
        for x, z in profile['polygon_xz']:
            for y in (low_y, high_y):
                if _interior_distance([x, y, z], selected['region']) < -1e-10:
                    raise ValueError('crack profile '+profile['role']+' exceeds the selected region; enlarge the region or reduce length_m, width_m, or depth_m')
        cutters.append({**copy.deepcopy(profile),
                        'min': [min(p[0] for p in profile['polygon_xz']), low_y, min(p[1] for p in profile['polygon_xz'])],
                        'max': [max(p[0] for p in profile['polygon_xz']), high_y, max(p[1] for p in profile['polygon_xz'])]})
    return cutters


def _point_between_nodes(scene, point, source, target):
    if source == target:
        return list(point)
    from core import world_point
    nodes = {node['id']: node for node in scene['nodes']}
    if source not in nodes or target not in nodes:
        raise ValueError('support references an unknown node')
    world = world_point(scene, source, point)
    chain, seen, node_id = [], set(), target
    while node_id is not None:
        if node_id in seen or node_id not in nodes:
            raise ValueError('support node hierarchy is cyclic or incomplete')
        seen.add(node_id)
        node = nodes[node_id]
        chain.append(node)
        node_id = node['parent']
    for node in reversed(chain):
        p = [world[i]-node['translation'][i] for i in range(3)]
        cosine, sine = math.cos(node['rotation_z']), math.sin(node['rotation_z'])
        world = [p[0]*cosine+p[1]*sine, -p[0]*sine+p[1]*cosine, p[2]]
    return world


def _sample_protection(scene, part, point, node, radius):
    transformed = [_point_between_nodes(scene, [point[0]+dx, point[1]+dy, point[2]+dz], node, part['node'])
                   for dx in (-radius, radius) for dy in (-radius, radius) for dz in (-radius, radius)]
    low = [min(p[axis] for p in transformed) for axis in range(3)]
    high = [max(p[axis] for p in transformed) for axis in range(3)]
    center = _point_between_nodes(scene, point, node, part['node'])
    bounds = _bounds(part)
    # Extend the protected mount column to its nearest exterior face, so a
    # paint mask cannot wear through the visibly functional mounting patch.
    if abs(center[1]-bounds[2]) <= abs(center[1]-bounds[3]):
        low[1] = min(low[1], bounds[2])
    else:
        high[1] = max(high[1], bounds[3])
    return {'shape': 'box', 'min': low, 'max': high}


def _protections(scene, part):
    """Conservative functional keep-outs; no destruction of unselected parts."""
    result = []
    for socket in scene.get('sockets', []):
        if not socket.get('enabled', True):
            continue
        has_samples = False
        named_by_end = False
        for end in socket.get('support_ends', {}).values():
            if part['id'] not in end.get('part_ids', []):
                continue
            named_by_end = True
            radius = _number(end.get('required_depth_m', .005), 'required_depth_m', 0)
            if radius <= 0:
                raise ValueError('required_depth_m must be positive')
            for point in end.get('samples', []):
                has_samples = True
                result.append((socket['id'], _sample_protection(scene, part, _vector(point, 'support sample'), end.get('node', socket.get('node')), radius)))
        if not has_samples and (named_by_end or part['id'] in socket.get('coverage', {}).get('support_part_ids', [])):
            # Coverage without authored samples cannot safely localize a load
            # path. Protect the whole explicitly named support, conservatively.
            bounds = _bounds(part)
            result.append((socket['id'], {'shape': 'box', 'min': bounds[::2], 'max': bounds[1::2]}))
    if len(result) > 4096:
        raise ValueError('functional protection sample budget exceeded')
    return result


def _intersects(first, second):
    low, high = _region_bounds(second)
    return all(first['max'][axis] >= low[axis] and first['min'][axis] <= high[axis] for axis in range(3))



def _clip_polygon_xz(polygon, low, high):
    """Original Sutherland-Hodgman clip against an axis-aligned support box."""
    output = [list(point) for point in polygon]
    for axis, bound, keep_greater in ((0, low[0], True), (0, high[0], False),
                                      (1, low[2], True), (1, high[2], False)):
        source, output = output, []
        if not source:
            break
        previous = source[-1]
        previous_inside = previous[axis] >= bound if keep_greater else previous[axis] <= bound
        for point in source:
            inside = point[axis] >= bound if keep_greater else point[axis] <= bound
            if inside != previous_inside:
                t = (bound-previous[axis])/(point[axis]-previous[axis])
                output.append([previous[i]+t*(point[i]-previous[i]) for i in range(2)])
            if inside:
                output.append(point)
            previous, previous_inside = point, inside
    return output


def _support_volume_present(part, region):
    """Require the whole support box to remain in the original analytic solid."""
    bounds = _bounds(part)
    low, high = region['min'], region['max']
    if any(low[axis] < bounds[2*axis]-1e-10 or high[axis] > bounds[2*axis+1]+1e-10 for axis in range(3)):
        return False
    for cut in part.get('cuts', []):
        y_overlap = min(high[1], cut['y_range'][1])-max(low[1], cut['y_range'][0])
        if y_overlap <= 1e-10:
            continue
        polygon = _clip_polygon_xz(cut['polygon_xz'], low, high)
        if len(polygon) < 3:
            continue
        area2 = abs(sum(a[0]*b[1]-b[0]*a[1] for a, b in zip(polygon, polygon[1:]+polygon[:1])))
        if area2 > 2e-12:
            return False
    return True


def _validate_supports(scene):
    """Recheck every authored sample, including ends in other local nodes.

    Each authored sample must retain material, and its protected support
    column must fit inside the source panel and avoid every cut volume. Node
    rotations use a conservative transformed AABB. These authored volumes do
    not certify whole-part connectivity, load, or strength. Coverage-only
    references are presence-checked and explicitly unverified.
    """
    from damage_math import point_in_panel
    parts = {part['id']: part for part in scene['parts']}
    invalidated, validation = [], []
    for socket in scene.get('sockets', []):
        reasons, count = [], 0
        for end_name, end in socket.get('support_ends', {}).items():
            part_ids = end.get('part_ids', [])
            samples = end.get('samples', [])
            radius = _number(end.get('required_depth_m', .005), 'required_depth_m', 0)
            if radius <= 0:
                raise ValueError('required_depth_m must be positive')
            if not part_ids or not samples:
                reasons.append(end_name+': missing authored support parts or samples')
            for index, point in enumerate(samples):
                count += 1
                material_found = False
                volume_found = False
                for part_id in part_ids:
                    part = parts.get(part_id)
                    if part is None:
                        continue
                    local = _point_between_nodes(scene, _vector(point, 'support sample'), end.get('node', socket.get('node')), part['node'])
                    if point_in_panel(part, local):
                        material_found = True
                        keepout = _sample_protection(scene, part, point, end.get('node', socket.get('node')), radius)
                        if _support_volume_present(part, keepout):
                            volume_found = True
                            break
                if not material_found:
                    reasons.append(end_name+': material missing at sample '+str(index))
                elif not volume_found:
                    reasons.append(end_name+': support volume missing around sample '+str(index))
        for part_id in socket.get('coverage', {}).get('support_part_ids', []):
            if part_id not in parts:
                reasons.append('coverage: missing support part '+part_id)
        report = {'method': 'conservative_authored_support_volumes', 'checked_sample_count': count, 'checked_volume_count': count,
                  'valid': not reasons if count or reasons else None, 'reasons': reasons,
                  'limitation': 'Every authored sample and its support column must retain material. Rotated columns use conservative AABBs. This does not certify whole-part strength, connectivity, or unsampled support. Coverage-only references verify presence only.'}
        socket['support_validation'] = report
        if reasons:
            socket['enabled'] = False
            invalidated.append(socket['id'])
        validation.append({'socket_id': socket['id'], **copy.deepcopy(report)})
    grips = []
    for grip in scene.get('grips', []):
        if grip.get('socket_id') in invalidated:
            grip['enabled'] = False
            grip['support_validation'] = {'valid': False, 'reasons': ['support socket '+grip['socket_id']+' lost authored support material or volume']}
            grips.append(grip['id'])
    scene.setdefault('metadata', {})['damage_support_validation'] = validation
    return invalidated, grips


def _edge_config(part, selected):
    bounds = _bounds(part)
    side = selected['side']
    plane = bounds[2] if side == 'front' else bounds[3]
    segments = _surface_segments(part, side)
    config = copy.deepcopy(selected)
    config['method'] = 'original_perimeter_distance_mask'
    config['geometry_effect'] = 'none; shader material/normal route only'
    config['boundary_segments_xz'] = segments
    config['vertex_weights'] = [_edge_weight(part, side, point, selected, segments) for point in part['vertices']]
    low, high = _region_bounds(selected['region'])
    x0, x1 = max(bounds[0], low[0]), min(bounds[1], high[0])
    z0, z1 = max(bounds[4], low[2]), min(bounds[5], high[2])
    n = selected['samples_per_axis']
    samples = []
    if x0 <= x1 and z0 <= z1:
        for i in range(n):
            for j in range(n):
                point = [x0+(x1-x0)*i/(n-1), plane, z0+(z1-z0)*j/(n-1)]
                samples.append({'point': point, 'side': side, 'weight': _edge_weight(part, side, point, selected, segments)})
    config['samples'] = samples
    return config


def apply_operations(scene, operations):
    """Return a deep copy with exact-part, exact-face, physically bounded edits.

    Cut depth defaults to 25% thickness. Only an explicit depth=None requests
    through removal, and physical region Y limits still apply. Default support
    protection rejects a whole cut if its conservative AABB touches a keep-out.
    A crack has 2..4 finite-width cavity profiles, each wholly inside its region,
    with numeric depth_m below panel thickness. The entire crack is protected
    and applied transactionally. No profile is scaled or clipped to fit.
    An explicit boolean override revalidates all authored support volumes after
    each applied cut. No engine constraints or destruction rules are authored.
    """
    if not isinstance(operations, list) or len(operations) > MAX_OPERATIONS:
        raise ValueError('operations must be a list of at most 64 entries')
    original_parts = {part['id']: part for part in scene['parts']}
    if len(original_parts) != len(scene['parts']):
        raise ValueError('scene has duplicate part IDs')
    selected_operations, counts = [], {}
    for operation in operations:
        if not isinstance(operation, dict) or not isinstance(operation.get('part_id'), str) or operation['part_id'] not in original_parts:
            raise ValueError('operation must select an existing exact part_id')
        part = original_parts[operation['part_id']]
        selected = _operation(operation, part)
        selected_operations.append(selected)
        if selected['type'] in ('cut', 'crack'):
            additional_cuts = 2+selected['branches'] if selected['type'] == 'crack' else 1
            counts[part['id']] = counts.get(part['id'], len(part.get('cuts', [])))+additional_cuts
            if counts[part['id']] > MAX_CUTS_PER_PART:
                raise ValueError('operation cut budget exceeds 12 per part')
    result = copy.deepcopy(scene)
    if not selected_operations:
        return result
    reports = result.setdefault('metadata', {}).setdefault('damage_operations', [])
    positions = {part['id']: i for i, part in enumerate(result['parts'])}
    for selected in selected_operations:
        part = result['parts'][positions[selected['part_id']]]
        protections = _protections(result, part)
        selected = dict(selected, protected_regions=selected['protected_regions']+[region for _, region in protections])
        report = {'type': selected['type'], 'part_id': selected['part_id'], 'side': selected['side'],
                  'region': copy.deepcopy(selected['region']), 'seed': selected['seed'],
                  'override_protection': selected.get('override_protection', False)}
        if selected['type'] == 'edge_wear':
            part.setdefault('edge_wear', []).append(_edge_config(part, selected))
            report.update(status='applied', geometry_changed=False)
            reports.append(report)
            continue
        if selected['type'] == 'crack':
            cutters = _crack_cutters(part, selected)
            report.update(profile_count=len(cutters), length_m=selected['length_m'],
                          width_m=selected['width_m'], depth_m=selected['depth_m'],
                          center_xz=copy.deepcopy(selected['center_xz']),
                          direction_xz=copy.deepcopy(selected['direction_xz']),
                          branches=selected['branches'], segments=selected['segments'],
                          profiles=[{key: copy.deepcopy(cutter[key]) for key in ('polygon_xz', 'depth', 'role', 'centerline_xz')} for cutter in cutters])
        else:
            cutter = _cutter(part, selected)
            cutters = [cutter] if cutter is not None else []
        if not cutters:
            report.update(status='outside', reason='Selection does not intersect the requested exterior face with positive depth.')
            reports.append(report)
            continue
        protected_sockets = sorted({socket_id for socket_id, region in protections
                                    if any(_intersects(cutter, region) for cutter in cutters)})
        user_protected = any(_intersects(cutter, region) for region in selected['protected_regions'] for cutter in cutters)
        if not selected.get('override_protection', False) and (protected_sockets or user_protected):
            report.update(status='protected', protected_socket_ids=protected_sockets,
                          reason='Operation conservatively intersects a functional keep-out; explicit boolean override_protection=True is required.')
            reports.append(report)
            continue
        from damage_math import carve_panel
        carved = part
        try:
            for cutter in cutters:
                carved = carve_panel(carved, cutter['polygon_xz'], side=selected['side'], depth=cutter['depth'])
        except ValueError as error:
            if selected['type'] == 'crack':
                raise ValueError('crack could not apply every profile transactionally: '+str(error)) from error
            if str(error) != 'Cutter does not remove new panel material':
                raise
            report.update(status='outside', reason=str(error))
            reports.append(report)
            continue
        if carved.get('edge_wear'):
            carved['edge_wear'] = [_edge_config(carved, config) for config in carved['edge_wear']]
        result['parts'][positions[part['id']]] = carved
        if selected['type'] == 'crack':
            report.update(status='applied', depth=selected['depth_m'], through=False,
                          geometry_changed=True, region_method='complete_profiles_fully_contained',
                          cut_indices=list(range(len(part.get('cuts', [])), len(carved['cuts']))))
        else:
            report.update(status='applied', depth=cutters[0]['depth'], through=cutters[0]['depth'] is None,
                          geometry_changed=True, region_method='inscribed_polygon_prism_clipped_to_y_extent')
        if selected.get('override_protection', False):
            sockets, grips = _validate_supports(result)
            report.update(invalidated_socket_ids=sockets, invalidated_grip_ids=grips)
        reports.append(report)
    return result
