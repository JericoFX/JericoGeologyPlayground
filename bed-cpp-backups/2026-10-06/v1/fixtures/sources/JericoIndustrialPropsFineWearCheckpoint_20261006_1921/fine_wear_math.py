"""Conservative, opt-in acceleration of exact front contour distance masks."""


def contour_segments(config):
    """Merge exact collinear degree-2 segments and cull only impossible influence.

    This is a shader evaluation list, never a geometry/recipe mutation. Culling
    uses an XZ bounding box expanded by width: every omitted segment is beyond
    the mask's compact support everywhere within its selected region.
    """
    segments = [[tuple(a), tuple(b)] for a, b in config['boundary_segments_xz']]
    while True:
        adjacency = {}
        for index, (a, b) in enumerate(segments):
            adjacency.setdefault(a, []).append(index)
            adjacency.setdefault(b, []).append(index)
        merged = False
        for point, indices in adjacency.items():
            if len(indices) != 2 or indices[0] == indices[1]:
                continue
            first, second = indices
            a = next((v for v in segments[first] if v != point), None)
            b = next((v for v in segments[second] if v != point), None)
            if a is None or b is None:
                continue
            u = (a[0]-point[0], a[1]-point[1])
            v = (b[0]-point[0], b[1]-point[1])
            # Exact equality intentionally avoids an approximate geometric merge.
            if u[0]*v[1]-u[1]*v[0] == 0 and u[0]*v[0]+u[1]*v[1] < 0:
                segments = [segment for i, segment in enumerate(segments) if i not in indices]
                segments.append([a, b])
                merged = True
                break
        if not merged:
            break
    region = config['region']
    if region['shape'] == 'box':
        low = (region['min'][0], region['min'][2])
        high = (region['max'][0], region['max'][2])
    else:
        low = (region['center'][0]-region['radius'], region['center'][2]-region['radius'])
        high = (region['center'][0]+region['radius'], region['center'][2]+region['radius'])
    width = config.get('width_m', .015)
    return [[list(a), list(b)] for a, b in segments
            if all(max(a[axis], b[axis]) >= low[axis]-width and
                   min(a[axis], b[axis]) <= high[axis]+width for axis in (0, 1))]
