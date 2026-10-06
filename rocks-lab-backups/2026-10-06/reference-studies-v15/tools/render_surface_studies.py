#!/usr/bin/env python3
"""Render released mathematical surface OBJs, without modifying their geometry.

Run after the generating process has finished successfully:
 blender -b -t 8 -P tools/render_surface_studies.py -- --released --seeds 48 \
   --variants legacy new --views main --modes neutral

For complete evidence use --seeds 48 117 305 --variants legacy control new
 --views main alternate detail --modes neutral wireframe. A wireframe is a
 material-only view of the imported triangle edges, not a mesh modifier.
"""
import argparse
from array import array
import hashlib
import json
import math
from pathlib import Path
import sys
import time

ROOT = Path(__file__).resolve().parents[1]
PROFILES = ('shale_laminated', 'slate_foliated', 'limestone_dissolution')
SEEDS = (48, 117, 305)
VARIANTS = ('legacy', 'control', 'new')
VIEWS = ('main', 'alternate', 'detail')
MODES = ('neutral', 'wireframe')


def digest(path):
    h = hashlib.sha256()
    with Path(path).open('rb') as handle:
        for block in iter(lambda: handle.read(1024*1024), b''):
            h.update(block)
    return h.hexdigest()


def stable_hash(value):
    return hashlib.sha256(json.dumps(value, sort_keys=True).encode()).hexdigest()


def save_json(path, value):
    tmp = path.with_suffix(path.suffix + '.tmp')
    tmp.write_text(json.dumps(value, indent=2, ensure_ascii=False) + '\n')
    tmp.replace(path)


def input_name(profile, seed, variant):
    if profile not in PROFILES or seed not in SEEDS or variant not in VARIANTS:
        raise ValueError('Unknown surface profile, seed or comparison variant')
    return ('' if variant == 'new' else variant + '-') + f'{profile}-{seed}'


def validate_matched_recipes(recipes):
    """A comparison may vary its generator, never its physical dimensions."""
    if not recipes:
        raise ValueError('Empty comparison')
    first = recipes[0]
    for r in recipes:
        if r.get('units') != 'm':
            raise ValueError('Render requires documented meter units')
        for key in ('size', 'relief', 'backDepth'):
            value = r.get(key)
            if not isinstance(value, (float, int)) or not math.isfinite(value) or value <= 0:
                raise ValueError(f'Invalid physical dimension {key}')
        for key in ('size', 'relief', 'backDepth', 'units', 'seed'):
            if r.get(key) != first.get(key):
                raise ValueError(f'Unmatched comparison recipes: {key}')
        if r.get('grid', {}).get('cells') != first.get('grid', {}).get('cells'):
            raise ValueError('Unmatched comparison mesh resolution')


def preflight_directory(directory):
    paths = {name: directory / name for name in ('heightfield.obj', 'recipe.json', 'metrics.json')}
    for path in paths.values():
        if not path.is_file() or path.stat().st_size == 0:
            raise FileNotFoundError(f'Unreleased or incomplete surface: {path}')
    recipe = json.loads(paths['recipe.json'].read_text())
    metrics = json.loads(paths['metrics.json'].read_text())
    if metrics.get('accepted') is not True:
        raise ValueError(f'Unaccepted geometry: {directory}')
    validate_matched_recipes([recipe])
    return {'directory': directory, 'recipe': recipe, 'metrics': metrics,
            'source_hashes': {name: digest(path) for name, path in paths.items()}}


def camera_settings(recipe, view):
    size = recipe['size']
    center = (recipe['relief'] - recipe['backDepth']) * .5
    if view == 'main':
        target, direction, scale = (0., 0., center), (.65, -1.05, 3.4), 1.62
    elif view == 'alternate':
        target, direction, scale = (0., 0., center), (-1.6, .95, 2.6), 1.68
    elif view == 'detail':
        target, direction, scale = (0., 0., recipe['relief']*.52), (.34, -.42, 3.6), .47
    else:
        raise ValueError(f'Unknown camera view: {view}')
    return {'location': [target[i] + size*direction[i] for i in range(3)],
            'target': list(target), 'ortho_scale': size*scale,
            'clip_start': size*.001, 'clip_end': size*100.}


def settings_for(args, recipe, view, mode, blender_version):
    s = recipe['size']
    lights = [('Key', (-1.8, -1.4, 3.8), 420., 1.25),
              ('Fill', (2.4, .4, 2.2), 65., 1.9),
              ('Rim', (-.5, 2.3, 2.6), 140., 1.4)]
    return {
        'blender_version': blender_version, 'engine': 'CYCLES', 'device': 'CPU',
        'samples': args.samples, 'denoising': False, 'adaptive_sampling': False,
        'render_seed': 1, 'resolution': [args.size, args.size],
        'physical_comparison': {k: recipe[k] for k in ('units','size','relief','backDepth')},
        'camera': camera_settings(recipe, view), 'view': view, 'mode': mode,
        'lights': [{'name': n, 'location': [x*s for x in loc], 'power_watts': p*s*s,
                    'disk_diameter_meters': radius*s} for n,loc,p,radius in lights],
        'world': {'color': [.22,.22,.22], 'strength': .5},
        'material': {'base_color': [.30,.30,.30], 'roughness': .87, 'metallic': 0.},
        'wireframe': {'shader': 'ShaderNodeWireframe', 'width_pixels': .60,
                      'color': [.015,.08,.115], 'all_imported_triangle_edges': True} if mode == 'wireframe' else None,
        'color_management': {'view_transform':'AgX', 'look':'AgX - Medium High Contrast', 'exposure':0., 'gamma':1.},
        'obj_import_axes': {'forward_axis':'Y', 'up_axis':'Z'},
        'source_geometry_changes': [], 'source_normal_changes': [], 'source_transform_changes': [],
        'note': 'Direct OBJ. Imported custom normals/smooth flags preserved. No textures, displacement, geometry modifiers, edits or object rescaling. Camera and lighting match within each physical comparison; light power scales with size squared to preserve irradiance across differently sized studies. Detail view crops the central 47% of surface width.'}


def mesh_fingerprint(obj):
    """Guard imported coordinates/topology/corner normals against accidental edits."""
    mesh = obj.data
    h = hashlib.sha256()
    for collection, attr, count, code in ((mesh.vertices,'co',len(mesh.vertices)*3,'f'),
        (mesh.loops,'vertex_index',len(mesh.loops),'i'),
        (mesh.corner_normals,'vector',len(mesh.corner_normals)*3,'f')):
        values = array(code, [0]) * count
        collection.foreach_get(attr, values)
        h.update(values.tobytes())
    h.update(bytes(int(p.use_smooth) for p in mesh.polygons))
    h.update(json.dumps([list(row) for row in obj.matrix_world]).encode())
    return h.hexdigest()


def setup_scene(settings):
    import bpy
    from mathutils import Vector
    bpy.ops.object.select_all(action='SELECT')
    bpy.ops.object.delete(use_global=False)
    scene = bpy.context.scene
    scene.render.engine = 'CYCLES'
    scene.cycles.device = 'CPU'
    scene.cycles.samples = settings['samples']
    scene.cycles.use_denoising = False
    scene.cycles.use_adaptive_sampling = False
    scene.cycles.seed = 1
    scene.render.resolution_x, scene.render.resolution_y = settings['resolution']
    scene.render.resolution_percentage = 100
    scene.render.image_settings.file_format = 'PNG'
    scene.render.image_settings.color_mode = 'RGB'
    scene.render.image_settings.color_depth = '8'
    scene.render.film_transparent = False
    scene.unit_settings.system = 'METRIC'
    scene.unit_settings.scale_length = 1.
    for key, value in settings['color_management'].items():
        setattr(scene.view_settings, key, value)
    world = bpy.data.worlds.new('Documented neutral studio')
    world.use_nodes = True
    world.node_tree.nodes['Background'].inputs['Color'].default_value = (*settings['world']['color'], 1.)
    world.node_tree.nodes['Background'].inputs['Strength'].default_value = settings['world']['strength']
    scene.world = world
    c = settings['camera']
    bpy.ops.object.camera_add(location=c['location'])
    camera = bpy.context.object
    camera.rotation_euler = (Vector(c['target']) - camera.location).to_track_quat('-Z','Y').to_euler()
    camera.data.type = 'ORTHO'
    camera.data.ortho_scale = c['ortho_scale']
    camera.data.clip_start, camera.data.clip_end = c['clip_start'], c['clip_end']
    scene.camera = camera
    for light in settings['lights']:
        bpy.ops.object.light_add(type='AREA', location=light['location'])
        lamp = bpy.context.object
        lamp.name = light['name']
        lamp.data.energy = light['power_watts']
        lamp.data.shape = 'DISK'
        lamp.data.size = light['disk_diameter_meters']
        lamp.rotation_euler = (Vector(c['target']) - lamp.location).to_track_quat('-Z','Y').to_euler()
    clay = bpy.data.materials.new('Neutral clay; direct imported mesh')
    clay.use_nodes = True
    nt = clay.node_tree
    p = nt.nodes.get('Principled BSDF')
    p.inputs['Base Color'].default_value = (*settings['material']['base_color'], 1.)
    p.inputs['Roughness'].default_value = settings['material']['roughness']
    p.inputs['Metallic'].default_value = 0.
    if settings['wireframe']:
        w = nt.nodes.new('ShaderNodeWireframe')
        w.use_pixel_size = True
        w.inputs['Size'].default_value = settings['wireframe']['width_pixels']
        mix = nt.nodes.new('ShaderNodeMixRGB')
        mix.inputs[1].default_value = (*settings['material']['base_color'], 1.)
        mix.inputs[2].default_value = (*settings['wireframe']['color'], 1.)
        nt.links.new(w.outputs['Fac'], mix.inputs[0])
        nt.links.new(mix.outputs[0], p.inputs['Base Color'])
    return scene, clay


def render_one(args, entry, profile, seed, variant, view, mode):
    import bpy
    from mathutils import Vector
    path = entry['directory'] / 'heightfield.obj'
    name = f'{profile}-{seed}-{variant}-{view}-{mode}'
    image = args.output_dir / (name + '.png')
    record_path = args.output_dir / 'provenance' / (name + '.json')
    settings = settings_for(args, entry['recipe'], view, mode, bpy.app.version_string)
    settings_hash = stable_hash(settings)
    renderer_hash = digest(Path(__file__))
    if image.is_file() and record_path.is_file() and not args.force:
        old = json.loads(record_path.read_text())
        if (old.get('source_hashes') == entry['source_hashes'] and old.get('settings_sha256') == settings_hash
            and old.get('renderer_sha256') == renderer_hash and old.get('png_sha256') == digest(image)):
            print('RENDER_SKIP ' + name, flush=True)
            return
    start = time.monotonic()
    scene, clay = setup_scene(settings)
    bpy.ops.object.select_all(action='DESELECT')
    bpy.ops.wm.obj_import(filepath=str(path), forward_axis='Y', up_axis='Z')
    objects = list(bpy.context.selected_objects)
    meshes = [o for o in objects if o.type == 'MESH']
    if not meshes:
        raise ValueError('OBJ contains no mesh: ' + str(path))
    stats = []
    fingerprints = {o.name: mesh_fingerprint(o) for o in meshes}
    for obj in meshes:
        if len(obj.modifiers) or not obj.data.has_custom_normals:
            raise ValueError('Imported OBJ must preserve custom normals and have no modifiers')
        obj.data.materials.clear()
        obj.data.materials.append(clay)
        stats.append({'name':obj.name, 'vertices':len(obj.data.vertices), 'triangles':len(obj.data.polygons),
            'has_custom_normals':obj.data.has_custom_normals, 'smooth_polygons':sum(p.use_smooth for p in obj.data.polygons),
            'world_bounds':[list(obj.matrix_world @ Vector(v)) for v in obj.bound_box],
            'matrix_world':[list(row) for row in obj.matrix_world], 'imported_mesh_sha256':fingerprints[obj.name]})
    scene.render.filepath = str(image)
    bpy.ops.render.render(write_still=True)
    if fingerprints != {o.name:mesh_fingerprint(o) for o in meshes}:
        raise RuntimeError('Imported geometry/normals/transforms changed during rendering')
    for file, source_hash in entry['source_hashes'].items():
        if digest(entry['directory']/file) != source_hash:
            raise RuntimeError('Source changed during rendering: '+str(entry['directory']/file))
    save_json(record_path, {'profile':profile,'seed':seed,'variant':variant,'view':view,'mode':mode,
        'obj':str(path.relative_to(ROOT)), 'source_hashes':entry['source_hashes'], 'recipe':entry['recipe'],
        'geometry_accepted':True, 'image':image.name, 'png_sha256':digest(image),
        'settings':settings, 'settings_sha256':settings_hash, 'renderer_sha256':renderer_hash,
        'mesh_stats':stats, 'imported_geometry_normals_transforms_unchanged':True,
        'elapsed_seconds':round(time.monotonic()-start,3)})
    print('RENDER_DONE '+name, flush=True)


def arguments():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--released', action='store_true', help='Affirm generation has fully finished before reading selected outputs')
    parser.add_argument('--profiles', nargs='+', choices=PROFILES, default=list(PROFILES))
    parser.add_argument('--seeds', nargs='+', type=int, choices=SEEDS, default=[48])
    parser.add_argument('--variants', nargs='+', choices=VARIANTS, default=['legacy','new'])
    parser.add_argument('--views', nargs='+', choices=VIEWS, default=['main'])
    parser.add_argument('--modes', nargs='+', choices=MODES, default=['neutral'])
    parser.add_argument('--size', type=int, default=768)
    parser.add_argument('--samples', type=int, default=32)
    parser.add_argument('--input-dir', type=Path, default=ROOT/'out/surface-v13')
    parser.add_argument('--output-dir', type=Path, default=ROOT/'renders/surface-v13')
    parser.add_argument('--force', action='store_true')
    args = parser.parse_args(sys.argv[sys.argv.index('--')+1:] if '--' in sys.argv else sys.argv[1:])
    if not args.released: parser.error('Pass --released only after mesh generation has finished')
    if args.size < 64 or args.samples < 1: parser.error('Invalid render resolution or samples')
    return args


def main():
    args = arguments()
    entries = {}
    for profile in args.profiles:
        for seed in args.seeds:
            group = [preflight_directory(args.input_dir/input_name(profile,seed,v)) for v in args.variants]
            validate_matched_recipes([e['recipe'] for e in group])
            for variant, entry in zip(args.variants, group):
                if entry['recipe']['seed'] != seed: raise ValueError('Recipe seed does not match requested panel')
                if variant != 'legacy' and entry['recipe'].get('profile') != profile: raise ValueError('Recipe profile does not match panel')
                entries[(profile,seed,variant)] = entry
    args.output_dir.mkdir(parents=True, exist_ok=True)
    (args.output_dir/'provenance').mkdir(exist_ok=True)
    for view in args.views:
        for mode in args.modes:
            for seed in args.seeds:
                for profile in args.profiles:
                    for variant in args.variants:
                        render_one(args, entries[(profile,seed,variant)], profile,seed,variant,view,mode)
    records = [json.loads(p.read_text()) for p in sorted((args.output_dir/'provenance').glob('*.json'))]
    save_json(args.output_dir/'render_manifest.json', {'files':records, 'renderer_sha256':digest(Path(__file__))})


if __name__ == '__main__': main()
