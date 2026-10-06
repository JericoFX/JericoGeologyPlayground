#!/usr/bin/env python3
"""Render matched v11 support, v13 limestone and experimental v14 alveoli.

Run only once generation and diagnostics are explicitly released:
  blender -b -t 8 -P tools/render_alveolar_studies.py -- --released
No texture, displacement, smoothing, remeshing or object rescaling is applied.
The lateral-light row retains the detail camera and changes actual area lights.
"""
import argparse
import json
import math
from pathlib import Path
import sys
import time

sys.dont_write_bytecode = True
ROOT = Path(__file__).resolve().parents[1]
sys.path.insert(0, str(ROOT / 'tools'))
import render_surface_studies as base

SEEDS = (48, 117, 305)
VARIANTS = ('support', 'limestone', 'alveolar')
ROWS = (('main', 'neutral', 'studio'), ('detail', 'neutral', 'studio'),
        ('detail', 'wireframe', 'studio'), ('detail', 'neutral', 'lateral'))
EXPECTED = {'support': ('heightfield-cliff-v11', 'massive_oblique'),
            'limestone': ('surface-families-v13', 'limestone_dissolution'),
            'alveolar': ('surface-families-v14', 'alveolar_blocks')}
digest, save_json, stable_hash = base.digest, base.save_json, base.stable_hash


def validate_group(entries, seed):
    if set(entries) != set(VARIANTS):
        raise ValueError('Comparison requires support, limestone and alveolar variants')
    recipes = [entries[v]['recipe'] for v in VARIANTS]
    base.validate_matched_recipes(recipes)
    for variant, entry in entries.items():
        recipe = entry['recipe']
        version, profile = EXPECTED[variant]
        if recipe.get('generatorVersion') != version or recipe.get('profile') != profile:
            raise ValueError('Wrong generator/profile for comparison variant: ' + variant)
        if recipe.get('seed') != seed:
            raise ValueError('Wrong recipe seed')
        if recipe.get('grid') != recipes[0].get('grid'):
            raise ValueError('Unmatched source resolution')
    if any(recipes[0].get(k) != v for k, v in {'size': 2., 'relief': .55, 'backDepth': .3}.items()) or recipes[0]['grid'] != {'cells': 256, 'width': 1025, 'height': 1025}:
        raise ValueError('Unexpected study dimensions; comparison labels require 2m/.55m/.3m and 256 cells/1025 source')


def patch_center(diagnostics):
    patches = diagnostics.get('events', {}).get('patches', [])
    if not patches:
        raise ValueError('Detail camera requires at least one accepted patch event')
    event = patches[0]
    values = [event.get(k) for k in ('x', 'y', 'radius', 'aspect', 'angle')]
    if any(not isinstance(v, (int, float)) or isinstance(v, bool) or not math.isfinite(v) for v in values):
        raise ValueError('Invalid patch event coordinates')
    x, y, radius, aspect, angle = values
    if radius <= 0 or aspect <= 0:
        raise ValueError('Invalid patch radius/aspect')
    return {'patch_index': 0, 'center_meters': [x, y], 'patch_radius_meters': radius,
            'patch_aspect': aspect, 'patch_angle_radians': angle,
            'selection': 'First accepted patch in deterministic event order; no visual cherry-picking'}


def sample_front_z(path, xy, size, cells):
    """Read the four original grid vertices surrounding XY, without copying OBJ."""
    gx, gy = [(v / size + .5) * cells for v in xy]
    if not (0 <= gx <= cells and 0 <= gy <= cells):
        raise ValueError('Patch center falls outside source grid')
    ix, iy = min(int(gx), cells - 1), min(int(gy), cells - 1)
    tx, ty = gx - ix, gy - iy
    needed = {(iy + dy) * (cells + 1) + ix + dx: (dx, dy)
              for dy in range(2) for dx in range(2)}
    vertices, index = {}, 0
    with Path(path).open() as source:
        for line in source:
            if not line.startswith('v '):
                continue
            if index in needed:
                x, y, z = map(float, line.split()[1:4])
                dx, dy = needed[index]
                expected = [size * ((ix + dx) / cells - .5), size * ((iy + dy) / cells - .5)]
                if abs(x - expected[0]) > 1e-6 or abs(y - expected[1]) > 1e-6 or not math.isfinite(z):
                    raise ValueError('OBJ front ordering does not match regular source grid')
                vertices[(dx, dy)] = z
            index += 1
            if len(vertices) == 4:
                break
    if len(vertices) != 4:
        raise ValueError('Missing source front vertices around detail target')
    return ((1 - ty) * ((1 - tx) * vertices[(0, 0)] + tx * vertices[(1, 0)])
            + ty * ((1 - tx) * vertices[(0, 1)] + tx * vertices[(1, 1)]))


def settings_for(args, recipe, view, mode, lighting, blender_version, anchor):
    if (view, mode, lighting) not in ROWS:
        raise ValueError('Unknown alveolar comparison row')
    settings = base.settings_for(args, recipe, view, mode, blender_version)
    if view == 'detail':
        size = recipe['size']
        target = anchor['target']
        aspect = anchor.get('patch_aspect', 1.)
        scale = max(size * .32, 2.8 * anchor['patch_radius_meters'] * max(aspect, 1. / aspect))
        direction = (.34, -.42, 3.6)
        settings['camera'] = {'location': [target[i] + size * direction[i] for i in range(3)],
                              'target': list(target), 'ortho_scale': scale,
                              'clip_start': size * .001, 'clip_end': size * 100.}
    if lighting == 'lateral':
        size = recipe['size']
        # Real lower-elevation area illumination, not a camera-only alternate.
        settings['lights'] = [
            {'name': 'LateralKey', 'location': [-2.7 * size, -.7 * size, .65 * size],
             'power_watts': 440. * size * size, 'disk_diameter_meters': .65 * size},
            {'name': 'LateralFill', 'location': [2.4 * size, .8 * size, 2.4 * size],
             'power_watts': 35. * size * size, 'disk_diameter_meters': 1.9 * size}]
        settings['world'] = {'color': [.22, .22, .22], 'strength': .22}
    if mode == 'wireframe':
        settings['wireframe']['color'] = [.015, .015, .015]
    settings['lighting_preset'] = lighting
    settings['detail_anchor'] = anchor if view == 'detail' else None
    settings['note'] = ('Direct OBJ; imported normals and transforms preserved. No textures, displacement, '
                        'mesh modifiers, smoothing or object rescaling. All variants use identical settings '
                        'within a seed/row. Detail centers the first accepted patch event, using the original '
                        'support height. Lateral row changes actual area lights while retaining detail camera.')
    return settings


def stem_for(seed, variant, view, mode, lighting):
    return f'alveolar-{seed}-{variant}-{view}-{mode}-{lighting}'


def dependencies():
    return {'render_alveolar_studies.py': digest(Path(__file__)),
            'render_surface_studies.py': digest(Path(base.__file__))}


def render_one(args, entry, seed, variant, row, anchor, auxiliary):
    import bpy
    from mathutils import Vector
    view, mode, lighting = row
    stem = stem_for(seed, variant, *row)
    output = args.output_dir / (stem + '.png')
    record_path = args.output_dir / 'provenance' / (stem + '.json')
    settings = settings_for(args, entry['recipe'], view, mode, lighting, bpy.app.version_string, anchor)
    settings_hash = stable_hash(settings)
    source_path = entry['directory'] / 'heightfield.obj'
    dependency_hashes = dependencies()
    if output.is_file() and record_path.is_file() and not args.force:
        old = json.loads(record_path.read_text())
        if (old.get('source_hashes') == entry['source_hashes'] and old.get('auxiliary_sources') == auxiliary
                and old.get('settings_sha256') == settings_hash and old.get('renderer_dependencies') == dependency_hashes
                and old.get('png_sha256') == digest(output)
                and old.get('imported_geometry_normals_transforms_unchanged') is True):
            print('RENDER_SKIP ' + stem, flush=True)
            return record_path
    start = time.monotonic()
    bpy.data.orphans_purge(do_recursive=True)
    scene, clay = base.setup_scene(settings)
    bpy.ops.object.select_all(action='DESELECT')
    bpy.ops.wm.obj_import(filepath=str(source_path), forward_axis='Y', up_axis='Z')
    meshes = [obj for obj in bpy.context.selected_objects if obj.type == 'MESH']
    if not meshes:
        raise ValueError('OBJ contains no mesh')
    fingerprints = {obj.name: base.mesh_fingerprint(obj) for obj in meshes}
    stats = []
    for obj in meshes:
        if obj.modifiers or not obj.data.has_custom_normals:
            raise ValueError('Direct OBJ must have custom normals and no modifiers')
        obj.data.materials.clear()
        obj.data.materials.append(clay)
        stats.append({'name': obj.name, 'vertices': len(obj.data.vertices), 'triangles': len(obj.data.polygons),
                      'has_custom_normals': obj.data.has_custom_normals,
                      'smooth_polygons': sum(p.use_smooth for p in obj.data.polygons),
                      'world_bounds': [list(obj.matrix_world @ Vector(v)) for v in obj.bound_box],
                      'matrix_world': [list(row) for row in obj.matrix_world],
                      'imported_mesh_sha256': fingerprints[obj.name]})
    scene.render.filepath = str(output)
    bpy.ops.render.render(write_still=True)
    if fingerprints != {obj.name: base.mesh_fingerprint(obj) for obj in meshes}:
        raise RuntimeError('Imported geometry/normals/transforms changed while rendering')
    for filename, sha in entry['source_hashes'].items():
        if digest(entry['directory'] / filename) != sha:
            raise RuntimeError('Source changed during render: ' + filename)
    for source in auxiliary:
        if digest(ROOT / source['path']) != source['sha256']:
            raise RuntimeError('Detail anchor source changed while rendering')
    save_json(record_path, {'profile': entry['recipe']['profile'], 'seed': seed, 'variant': variant,
                           'view': view, 'mode': mode, 'lighting': lighting,
                           'obj': str(source_path.relative_to(ROOT)), 'recipe': entry['recipe'],
                           'source_hashes': entry['source_hashes'], 'auxiliary_sources': auxiliary,
                           'geometry_accepted': True, 'experimental_not_promoted': True,
                           'image': output.name, 'png_sha256': digest(output),
                           'settings': settings, 'settings_sha256': settings_hash,
                           'renderer_dependencies': dependency_hashes, 'mesh_stats': stats,
                           'imported_geometry_normals_transforms_unchanged': True,
                           'elapsed_seconds': round(time.monotonic() - start, 3)})
    print('RENDER_DONE ' + stem, flush=True)
    return record_path


def arguments():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--released', action='store_true')
    parser.add_argument('--seeds', nargs='+', type=int, choices=SEEDS, default=list(SEEDS))
    parser.add_argument('--rows', nargs='+', choices=['main', 'detail', 'wireframe', 'lateral'],
                        default=['main', 'detail', 'wireframe', 'lateral'])
    parser.add_argument('--size', type=int, default=640)
    parser.add_argument('--samples', type=int, default=32)
    parser.add_argument('--input-dir', type=Path, default=ROOT / 'out/alveolar-v14')
    parser.add_argument('--output-dir', type=Path, default=ROOT / 'renders/alveolar-v14')
    parser.add_argument('--force', action='store_true')
    args = parser.parse_args(sys.argv[sys.argv.index('--') + 1:] if '--' in sys.argv else sys.argv[1:])
    if not args.released:
        parser.error('Pass --released only after explicit confirmation of complete generation')
    if args.size < 64 or args.samples < 1:
        parser.error('Invalid render size or samples')
    return args


def main():
    args = arguments()
    entries, anchors, auxiliary = {}, {}, {}
    for seed in args.seeds:
        entries[seed] = {v: base.preflight_directory(args.input_dir / f'{v}-{seed}') for v in VARIANTS}
        validate_group(entries[seed], seed)
        diag_path = entries[seed]['alveolar']['directory'] / 'diagnostics.json'
        diag = json.loads(diag_path.read_text())
        anchor = patch_center(diag)
        support = entries[seed]['support']
        r = support['recipe']
        z = sample_front_z(support['directory'] / 'heightfield.obj', anchor['center_meters'], r['size'], r['grid']['cells'])
        anchor['target'] = anchor['center_meters'] + [z]
        anchor['center_uv'] = [v / r['size'] + .5 for v in anchor['center_meters']]
        anchors[seed] = anchor
        auxiliary[seed] = [{'path': str(p.relative_to(ROOT)), 'sha256': digest(p)} for p in
                           (diag_path, support['directory'] / 'heightfield.obj')]
    args.output_dir.mkdir(parents=True, exist_ok=True)
    (args.output_dir / 'provenance').mkdir(exist_ok=True)
    row_map = dict(zip(('main', 'detail', 'wireframe', 'lateral'), ROWS))
    records = []
    for seed in args.seeds:
        for row_name in args.rows:
            for variant in VARIANTS:
                record = render_one(args, entries[seed][variant], seed, variant, row_map[row_name], anchors[seed], auxiliary[seed])
                records.append(json.loads(record.read_text()))
    save_json(args.output_dir / 'render_manifest.json', {'files': records, 'renderer_dependencies': dependencies(),
                                                       'experimental_not_promoted': True})


if __name__ == '__main__':
    main()
