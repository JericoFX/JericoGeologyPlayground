"""Original fixed, single-pane mine windows with three interchangeable states.

The opening is one continuous rectangle: no casement leaves, internal mullions,
handles, or invented hinges. Clear width/height measure between retaining beads.
The beads form a genuine two-sided glazing pocket outside that clear opening;
glass has a 1 mm edge clearance from the structural timber, and 11 mm bearing
behind the beads. Broken glazing is a connected closed shell with a real hole.
"""
from __future__ import annotations

import math
import random

from core import Builder, _frame, _uvs, clearance_sweep, normalize_recipe

PRESET = 'single_pane_fixed'
GLAZING_STATES = ('intact', 'broken', 'empty')
REBATE = .012
EDGE_CLEARANCE = .001
GLASS_THICKNESS = .004
FRAME_MEMBER = .135


def _normalize(recipe):
    if not isinstance(recipe, dict):
        raise TypeError('Recipe must be an object')
    local = dict(recipe)
    glazing = local.pop('glazing', 'intact')
    preset = local.pop('preset', PRESET)
    if glazing not in GLAZING_STATES:
        raise ValueError('glazing must be one of '+str(GLAZING_STATES))
    if preset != PRESET:
        raise ValueError('Unknown fixed-window preset: '+str(preset))
    defaults = dict(kind='window', width=1.12, height=1.28, thickness=.06,
                    damage='aged', seed=17)
    defaults.update(local)
    normalized = normalize_recipe(defaults)
    if normalized['kind'] != 'window':
        raise ValueError('The single-pane fixed preset requires kind=window')
    if normalized['angle'] != 0:
        raise ValueError('A fixed frame cannot have a moving leaf angle')
    return normalized, glazing


def _fixed_frame(builder):
    """Reuse the existing aged frame, with a wider external timber section."""
    w, h, t = [builder.r[k] for k in ('width', 'height', 'thickness')]
    recipe = builder.r
    # The timber opening accommodates the pane and edge clearance. Front/back
    # stops overlap it to leave precisely the requested visible clear opening.
    builder.r = dict(recipe, width=w+2*REBATE, height=h+2*REBATE)
    _frame(builder)
    builder.r = recipe
    timber_half = w/2+REBATE
    timber_top = h+2*REBATE
    def widen(x):
        return math.copysign(timber_half+(abs(x)-timber_half)*1.5, x) if abs(x)>timber_half else x
    def height(z):
        if z > timber_top:
            z = timber_top+(z-timber_top)*1.5
        if z < 0:
            z *= 1.5
        return z-REBATE
    for part in builder.scene['parts']:
        for vertex in part['vertices']:
            if part['role']=='Frame':
                vertex[0] = widen(vertex[0])
                vertex[2] = height(vertex[2])
            else:
                # Move existing frame bolt/grime fittings with the widened wood.
                vertex[0] = widen(vertex[0])
                vertex[2] -= REBATE
        if part['material']=='wood':
            part['material'] = 'wood_rough'
        part['uvs'] = _uvs(part['vertices'], part['faces'])
    for envelope in builder.scene['metadata']['frame_envelopes']:
        envelope['rect'] = [[widen(x),y] for x,y in envelope['rect']]
        envelope['z'] = [height(z) for z in envelope['z']]
    opening = builder.scene['sockets'][0]
    opening['coverage'] = {'opening':[w,h,t], 'capabilities':['fixed_glazing'],
                           'max_pane_count':1,
                           'support_part_ids':['frame.left','frame.right','frame.head','frame.sill']}

    # Separate front and back stops clamp the glass without occupying its solid.
    # Only these perimeter members reach the clear-opening boundary.
    bead = REBATE+.018
    profiles = [('left',-w/2-bead,-w/2,0,h),
                ('right',w/2,w/2+bead,0,h),
                ('bottom',-w/2-bead,w/2+bead,-bead,0),
                ('top',-w/2-bead,w/2+bead,h,h+bead)]
    for face, y0, y1 in [('front',-GLASS_THICKNESS/2-.012,-GLASS_THICKNESS/2),
                          ('back',GLASS_THICKNESS/2,GLASS_THICKNESS/2+.012)]:
        for side,x0,x1,z0,z1 in profiles:
            builder.box('frame.bead.'+face+'.'+side,'frame',x0,x1,y0,y1,z0,z1,
                        'wood_rough',.0005,'GlazingBead')
    # Compact corner straps reinforce joinery only; nothing crosses the pane.
    front_y = -t/2-.010
    for side,sign in [('left',-1),('right',1)]:
        cx = sign*(w/2+REBATE+FRAME_MEMBER*.52)
        for corner,z in [('bottom',.09),('top',h-.09)]:
            id = 'frame.reinforcement.'+side+'.'+corner
            builder.box(id,'frame',cx-.034,cx+.034,front_y-.006,front_y,
                        z-.065,z+.065,'iron',.0015,'CornerReinforcement')
            for i,dz in enumerate((-.043,.043)):
                builder.rivet(id+'.bolt.'+str(i),'frame',cx,front_y-.006,z+dz,
                              True,.010)
    return ['frame.bead.'+face+'.'+side for face in ('front','back')
            for side in ('left','right','bottom','top')]


def _polygon_area(points):
    return abs(sum(a[0]*b[1]-b[0]*a[1] for a,b in zip(points,points[1:]+points[:1])))/2


def _broken_pane(builder, bounds):
    """Triangulate radial annular cells, then close outer and fracture walls.

    Each fracture point and its outer partner occupy the same ordered ray from
    an interior center. Including all rectangle corners gives an exact outer
    rectangle. Inner radii are strictly inside it, so cells cannot intersect.
    One connected shell represents frame-retained shards, never loose triangles.
    """
    x0,x1,z0,z1 = bounds
    rng = random.Random(builder.r['seed']+40193)
    width,height = x1-x0,z1-z0
    cx = rng.choice((-1,1))*rng.uniform(.075,.12)*width
    cz = (z0+z1)/2+rng.choice((-1,1))*rng.uniform(.035,.09)*height
    # Unequal broad fracture sectors, with only a few long retained shards.
    # A dense alternating radial pattern reads as a regular comic-book star.
    angles = [(i+rng.uniform(-.35,.35))*math.tau/10 % math.tau for i in range(10)]
    corners = [(x0,z0),(x1,z0),(x1,z1),(x0,z1)]
    for x,z in corners:
        angle = math.atan2(z-cz,x-cx) % math.tau
        angles = [a for a in angles if abs((a-angle+math.pi)%math.tau-math.pi)>.14]
        angles.append(angle)
    angles.sort()
    candidates = list(range(len(angles)))
    rng.shuffle(candidates)
    long_shards = []
    for index in candidates:
        if all(abs((angles[index]-angles[j]+math.pi)%math.tau-math.pi)>.90
               for j in long_shards):
            long_shards.append(index)
        if len(long_shards)==3:
            break
    outer,inner = [],[]
    for index,angle in enumerate(angles):
        dx,dz = math.cos(angle),math.sin(angle)
        tx = ((x1 if dx>0 else x0)-cx)/dx if abs(dx)>1e-14 else math.inf
        tz = ((z1 if dz>0 else z0)-cz)/dz if abs(dz)>1e-14 else math.inf
        length = min(tx,tz)
        outer.append([cx+dx*length,cz+dz*length])
        fraction = rng.uniform(.32,.46) if index in long_shards else rng.uniform(.76,.94)
        inner.append([cx+dx*length*fraction,cz+dz*length*fraction])
    count = len(outer)
    perimeter = outer+inner
    vertices = [[x,y,z] for y in (-GLASS_THICKNESS/2,GLASS_THICKNESS/2)
                for x,z in perimeter]
    faces = []
    for i in range(count):
        j=(i+1)%count
        a,b,c,d=i,j,count+j,count+i
        cap = [[a,b,c],[a,c,d]]
        faces.extend(cap)
        faces.extend([[v+2*count for v in reversed(triangle)] for triangle in cap])
        faces.append([i,2*count+i,2*count+j,j])
        faces.append([count+j,3*count+j,3*count+i,count+i])
    builder.mesh('window.glass','frame',vertices,faces,'glass',.00015,'GlassPane')
    area = width*height
    return {'retained_glass_fraction':1-_polygon_area(inner)/area,
            'glass_hole':{'center_xz':[cx,cz], 'polygon_xz':inner,
                          'bounds_xz':[[min(v[0] for v in inner),min(v[1] for v in inner)],
                                       [max(v[0] for v in inner),max(v[1] for v in inner)]],
                          'area_m2':_polygon_area(inner),
                          'construction':'Closed connected annular glass shell with inner fracture walls; physical through-hole'}}


def build_singlepane(recipe):
    """Build one fixed-window scene in the existing metric JSON contract."""
    normalized,glazing = _normalize(recipe)
    builder = Builder(normalized)
    w,h,t = [normalized[k] for k in ('width','height','thickness')]
    supports = _fixed_frame(builder)
    bearing = REBATE-EDGE_CLEARANCE
    bounds = [-w/2-bearing,w/2+bearing,-bearing,h+bearing]
    x0,x1,z0,z1 = bounds
    builder.socket('window.glass.socket','frame','Glass','pane.rect.v1',[0,0,h/2],
                   {'footprint':[x1-x0,z1-z0], 'opening':[w,h],
                    'thickness':GLASS_THICKNESS, 'max_thickness':GLASS_THICKNESS,
                    'capabilities':['glazing','fixed_glazing'],
                    'support_part_ids':supports, 'support_overlap':bearing,
                    'edge_clearance':EDGE_CLEARANCE, 'glazing_state':glazing},
                   glazing!='empty')
    metadata = builder.scene['metadata']
    metadata.update({'generator':'JericoWindowsSinglePaneLab/singlepane.py',
                     'preset':PRESET, 'glazing_state':glazing,
                     'dimensions':{'clear_opening':[w,h,t],
                                   'structural_opening':[w+2*REBATE,h+2*REBATE],
                                   'frame_member':FRAME_MEMBER,'rebate_depth':REBATE,
                                   'glass_size':[x1-x0,z1-z0,GLASS_THICKNESS],
                                   'glass_edge_clearance':EDGE_CLEARANCE},
                     'leaf_envelopes':[],
                     'retained_glass_fraction':1.0 if glazing=='intact' else 0.0,
                     'grasp_policy':'Not applicable: fixed glazing has no moving leaf or hand-grasp interaction.',
                     'damage_policy':'Glazing state is independent of the stable aged timber frame. Broken glazing retains a supported closed perimeter shell. Empty removes the glass and disables its socket.',
                     'support_policy':'Glass sits in a two-sided stop/rebate pocket with 1 mm clearance from structural timber. Front/back retaining beads overlap all four edges by 11 mm; no floating fragments.',
                     'socket_coverage_convention':'Glass footprint is the full pane cut size, opening is visible clear width/height. support_part_ids identifies both sets of retaining beads. Existing glass and the modeled stop pocket are 4 mm thick. The replacement interface is capped at 4 mm; no adjustable-stop mechanism is modeled.',
                     'art_direction':'Dark aged timber and compact bolted iron perimeter reinforcement. Existing chipped frame outlines only; no new wood-edge damage or bars.'})
    if glazing=='intact':
        builder.box('window.glass','frame',x0,x1,-GLASS_THICKNESS/2,GLASS_THICKNESS/2,
                    z0,z1,'glass',.00015,'GlassPane')
    elif glazing=='broken':
        metadata.update(_broken_pane(builder,bounds))
    else:
        metadata['missing_parts'].append('window.glass')
    metadata['clearance'] = clearance_sweep(builder.scene)
    builder.scene['recipe'] = dict(normalized,preset=PRESET,glazing=glazing)
    return builder.scene
