"""Original stdlib-only finite-width profiles for procedural modeled tears.

These are authored cavities, not fracture mechanics, stress propagation, or
an engine damage solver. Coordinates, length, width and depth are metres in
the panel's local XZ plane. Each result is an independent cutter for
``damage_math.carve_panel``; apply them in the returned order.
"""
from __future__ import annotations

import math
import random

MIN_WIDTH = .001
MAX_SEGMENTS = 12
MAX_PROFILES = 4
MAX_PROFILE_VERTICES = 64
MITER_LIMIT = 1.5


def _number(value, label):
    if isinstance(value, bool) or not isinstance(value, (int, float)) or not math.isfinite(value):
        raise ValueError(label + ' must be finite and numeric')
    return float(value)


def _vector(value, label):
    if not isinstance(value, (list, tuple)) or len(value) != 2:
        raise ValueError(label + ' must have two coordinates')
    return [_number(v, label) for v in value]


def _integer(value, low, high, label):
    if isinstance(value, bool) or not isinstance(value, int) or not low <= value <= high:
        raise ValueError(label + ' must be an integer in ' + str(low) + '..' + str(high))
    return value


def _cross(a, b, c):
    return (b[0]-a[0])*(c[1]-a[1])-(b[1]-a[1])*(c[0]-a[0])


def _area(points):
    return sum(_cross(points[0], points[i], points[i+1]) for i in range(1, len(points)-1))/2


def _on_segment(p, a, b):
    return (abs(_cross(a, b, p)) <= 1e-11*math.dist(a, b)
            and min(a[0], b[0])-1e-11 <= p[0] <= max(a[0], b[0])+1e-11
            and min(a[1], b[1])-1e-11 <= p[1] <= max(a[1], b[1])+1e-11)


def _outline(left, right):
    points = [list(p) for p in left] + [list(p) for p in reversed(right)]
    if _area(points) < 0:
        points.reverse()
    if len(points) > MAX_PROFILE_VERTICES or _area(points) <= 1e-12:
        raise ValueError('Crack outline exceeds the geometric budget')
    for i, a in enumerate(points):
        b = points[(i+1) % len(points)]
        if any(math.dist(a, c) <= 1e-10 for c in points[i+1:]):
            raise ValueError('Crack outline contains coincident points')
        for j in range(i+1, len(points)):
            if j == i+1 or (i == 0 and j == len(points)-1):
                continue
            c, d = points[j], points[(j+1) % len(points)]
            if ((_cross(a, b, c)*_cross(a, b, d) < 0 and _cross(c, d, a)*_cross(c, d, b) < 0)
                    or any((_on_segment(c, a, b), _on_segment(d, a, b),
                            _on_segment(a, c, d), _on_segment(b, c, d)))):
                raise ValueError('Crack outline would self-intersect')
    return points


def _sweep(points, widths):
    """Normal-offset a gently turning path with explicitly limited miters."""
    normals = []
    for a, b in zip(points, points[1:]):
        distance = math.dist(a, b)
        normals.append([-(b[1]-a[1])/distance, (b[0]-a[0])/distance])
    left, right = [], []
    for i, (point, width) in enumerate(zip(points, widths)):
        before, after = normals[max(0, i-1)], normals[min(i, len(normals)-1)]
        miter = [before[k]+after[k] for k in range(2)]
        norm = math.hypot(*miter)
        miter = [x/norm for x in miter]
        cosine = sum(miter[k]*after[k] for k in range(2))
        distance = min(width*.5/max(cosine, 1e-12), MITER_LIMIT*width*.5)
        left.append([point[k]+miter[k]*distance for k in range(2)])
        right.append([point[k]-miter[k]*distance for k in range(2)])
    return left, right


def _path(origin, direction, length, width, segments, rng, centered):
    # Large widths need longer segments so offset edges cannot fold over.
    count = min(segments, max(2, int(length/(2*width))))
    step = length/count
    normal = [-direction[1], direction[0]]
    amplitude = min(.055*length, .30*step)
    points, widths = [], []
    for i in range(count+1):
        t = i/count
        offset = 0. if i in (0, count) else rng.uniform(-amplitude, amplitude)
        along = length*(t-.5 if centered else t)
        points.append([origin[k]+direction[k]*along+normal[k]*offset for k in range(2)])
        taper = .26+.74*math.sin(math.pi*t)
        variation = 1. if i == count//2 else rng.uniform(.84, 1.)
        widths.append(max(MIN_WIDTH, width*taper*variation))
    return points, widths


def _record(left, right, points, depth, role):
    return dict(polygon_xz=_outline(left, right), depth=depth, role=role,
                centerline_xz=[list(p) for p in points])


def crack_profiles(center_xz, length, width, depth, seed,
                   direction_xz=(0, 1), branches=2, segments=6):
    """Return at most four simple CCW cutters for one connected modeled tear.

    ``width`` is the maximum nominal swept width (at least 0.001 m); taper
    clamps to that minimum at tips and branches, avoiding zero-width slivers.
    ``length`` is the main centerline's projected length and must be at least
    four widths. Direction is normalized. Segments accepts 2..12; short wide
    tears reduce their segment count to keep miters separated. Branches is
    0..2. The local random generator never touches global random state.

    The main cavity is 65% of depth, an overlapping central section reaches
    full depth, and connected branches use 45%. This stepped authored depth
    variation is deliberate. All depths must fit the target panel thickness;
    the caller chooses the panel side and handles edge clipping/cut budgets.
    """
    center = _vector(center_xz, 'center_xz')
    direction = _vector(direction_xz, 'direction_xz')
    length, width, depth = (_number(value, name) for value, name in
                            ((length, 'length'), (width, 'width'), (depth, 'depth')))
    if width < MIN_WIDTH:
        raise ValueError('width must be at least 0.001 m')
    if length < 4*width:
        raise ValueError('length must be at least four times width')
    if depth <= 0 or depth*.45 == 0:
        raise ValueError('depth must be positive and representable for every section')
    if isinstance(seed, bool) or not isinstance(seed, int):
        raise ValueError('seed must be an integer')
    branches = _integer(branches, 0, 2, 'branches')
    segments = _integer(segments, 2, MAX_SEGMENTS, 'segments')
    norm = math.hypot(*direction)
    if norm == 0 or not math.isfinite(norm):
        raise ValueError('direction_xz must have finite nonzero length')
    direction = [v/norm for v in direction]
    if max(abs(v) for v in center)+2*length > 1e5:
        raise ValueError('Crack exceeds supported local metric coordinate range')
    rng = random.Random(seed)
    main, widths = _path(center, direction, length, width, segments, rng, True)
    left, right = _sweep(main, widths)
    result = [_record(left, right, main, depth*.65, 'main')]
    count = len(main)-1
    start = max(0, count//2-1)
    end = min(count, count//2+1)
    if end-start == count:
        end -= 1
    result.append(_record(left[start:end+1], right[start:end+1], main[start:end+1], depth, 'deep_section'))
    normal = [-direction[1], direction[0]]
    for branch in range(branches):
        root_index = max(1, min(count-1, round(count*(.34 if branch == 0 else .68))))
        root = main[root_index]
        sign = 1 if branch == 0 else -1
        angle = rng.uniform(.65, .95)
        branch_direction = [direction[k]*math.cos(angle)+sign*normal[k]*math.sin(angle) for k in range(2)]
        branch_width = max(MIN_WIDTH, width*.48)
        branch_length = max(4*branch_width, length*rng.uniform(.24, .34))
        branch_points, branch_widths = _path(root, branch_direction, branch_length,
                                           branch_width, 2, rng, False)
        branch_left, branch_right = _sweep(branch_points, branch_widths)
        result.append(_record(branch_left, branch_right, branch_points, depth*.45, 'branch'))
    return result
