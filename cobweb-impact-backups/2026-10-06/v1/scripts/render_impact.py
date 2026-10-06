#!/usr/bin/env python3
"""Native Blender renderer for the cheap whole-panel impact-response prototype.

blender -b --python scripts/render_impact.py -- --input outputs/run/before \
    --output renders/before.png --scene renders/before.blend

Geometry comes only from mesh.obj/remnants.obj. Detachment must remove actual
faces. Both meshes share the identical RGB-coverage atlas material. Remnants
are pretransformed cosmetic cards: no wrapping, cloth, or Jolt is simulated.
The fixed panel camera, lighting, exposure and silk material preserve the
approved render_physics.py style. State never changes rendering parameters.
"""
import argparse
import hashlib
import json
import math
from pathlib import Path
import sys
import time

sys.path.insert(0, str(Path(__file__).resolve().parent))
import render_physics as base

ROCK_STYLE = {'primitive': 'icosphere', 'subdivisions': 3, 'smooth_shading': False,
              'color': [0.19, 0.20, 0.205], 'roughness': 0.89,
              'noise_scale': 8.0, 'noise_detail': 2.0,
              'bump_strength': 0.12, 'bump_distance': 0.025, 'alpha': 1.0}


def parse_args(argv=None):
    if argv is None:
        argv = sys.argv[sys.argv.index('--')+1:] if '--' in sys.argv else []
    parser = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    parser.add_argument('--input', type=Path, required=True)
    parser.add_argument('--output', type=Path, required=True)
    parser.add_argument('--scene', type=Path)
    parser.add_argument('--width', type=int, default=1100)
    parser.add_argument('--height', type=int, default=900)
    parser.add_argument('--samples', type=int, default=48)
    parser.add_argument('--engine', choices=('eevee', 'cycles'), default='eevee')
    parser.add_argument('--validate-only', action='store_true')
    parser.add_argument('--overwrite', action='store_true')
    args = parser.parse_args(argv)
    if args.width < 16 or args.height < 16 or args.samples < 1:
        parser.error('Dimensions must be at least 16 pixels and samples at least 1')
    return args


def detached(frame):
    return str(frame.get('phase', '')).lower() in ('detached', 'removed')


def validate_frame(frame):
    base.validate_frame(frame)
    rock = frame.get('rock')
    if not rock:
        raise ValueError('An exported opaque rock is required')
    base.finite_point(rock['center'], 'Rock center')
    base.finite_point(rock['axis'], 'Rock axis')
    if sum(float(x)**2 for x in rock['axis']) <= 1e-20:
        raise ValueError('Rock axis must be nonzero')
    if not isinstance(rock['radius'], (int, float)) or not math.isfinite(rock['radius']) or rock['radius'] <= 0:
        raise ValueError('Rock radius must be positive and finite')
    if not isinstance(rock['angle'], (int, float)) or not math.isfinite(rock['angle']):
        raise ValueError('Rock rotation angle must be finite')
    if detached(frame) and (frame.get('collision_active', False) or frame.get('resistance_active', False)):
        raise ValueError('Detached panel cannot have collision or resistance active')


def inspect_geometry(directory, frame):
    result = {}
    for filename, key in [('mesh.obj', 'carrier'), ('remnants.obj', 'remnants')]:
        path = directory / filename
        vertices, faces, _ = base.read_obj(path) if path.exists() else ([], [], [])
        result[key] = {'vertices': len(vertices), 'faces': len(faces),
                       'present': bool(faces), 'source_exists': path.exists()}
    if detached(frame) and result['carrier']['faces']:
        raise ValueError('Detached panel must contain zero carrier faces; hiding a panel is forbidden')
    if not detached(frame) and not result['carrier']['faces']:
        raise ValueError('Intact or indenting panel requires real carrier faces')
    if any(value['faces'] for value in result.values()) and not (directory/'coverage.tga').is_file():
        raise ValueError('Visible carrier or remnant geometry requires coverage.tga')
    return result


def make_coverage_material(path):
    import bpy
    mat = base.silk_material('Coverage silk: alpha from RGB coverage only', (0.65, 0.65, 0.60))
    base.set_hashed_transparency(mat)
    tree = mat.node_tree
    tex = tree.nodes.new('ShaderNodeTexImage')
    tex.name = 'Actual exported coverage RGB, NOT opaque TGA alpha'
    tex.image = bpy.data.images.load(str(path), check_existing=False)
    tex.image.colorspace_settings.name = 'Non-Color'
    tex.extension, tex.interpolation = 'CLIP', 'Linear'
    coords = tree.nodes.new('ShaderNodeTexCoord')
    tree.links.new(coords.outputs['UV'], tex.inputs['Vector'])
    shader = tree.nodes.get('Principled BSDF')
    tree.links.new(tex.outputs['Color'], shader.inputs['Alpha'])
    tint = tree.nodes.new('ShaderNodeMixRGB')
    tint.blend_type = 'MIX'
    tint.inputs[1].default_value = (0.52, 0.52, 0.49, 1)
    tint.inputs[2].default_value = (0.75, 0.74, 0.68, 1)
    tree.links.new(tex.outputs['Color'], tint.inputs[0])
    tree.links.new(tint.outputs[0], shader.inputs['Base Color'])
    mat['alpha_source'] = 'Unmodified grayscale Color of coverage.tga; image Alpha is intentionally unused'
    mat['shared_by_carrier_and_remnants'] = True
    return mat


def make_exported_mesh(path, name, role, mat):
    import bpy
    vertices, faces, corner_uvs = base.read_obj(path)
    if not faces:
        return None
    mesh = bpy.data.meshes.new(name)
    mesh.from_pydata(vertices, [], faces)
    mesh.update()
    uv = mesh.uv_layers.new(name='UVMap')
    for polygon, coords in zip(mesh.polygons, corner_uvs):
        for loop_index, coord in zip(polygon.loop_indices, coords):
            uv.data[loop_index].uv = coord
    obj = bpy.data.objects.new(name, mesh)
    bpy.context.collection.objects.link(obj)
    obj.data.materials.append(mat)
    obj['role'], obj['geometry_source'] = role, str(path)
    obj['coordinates'] = 'Unmodified world-space OBJ coordinates'
    return obj


def make_rock(rock):
    import bpy
    from mathutils import Vector
    mat = base.material('Opaque rough grey benchmark stone', ROCK_STYLE['color'], ROCK_STYLE['roughness'])
    tree = mat.node_tree
    noise = tree.nodes.new('ShaderNodeTexNoise')
    noise.inputs['Scale'].default_value = ROCK_STYLE['noise_scale']
    noise.inputs['Detail'].default_value = ROCK_STYLE['noise_detail']
    bump = tree.nodes.new('ShaderNodeBump')
    bump.inputs['Strength'].default_value = ROCK_STYLE['bump_strength']
    bump.inputs['Distance'].default_value = ROCK_STYLE['bump_distance']
    tree.links.new(noise.outputs['Fac'], bump.inputs['Height'])
    tree.links.new(bump.outputs['Normal'], tree.nodes.get('Principled BSDF').inputs['Normal'])
    bpy.ops.mesh.primitive_ico_sphere_add(subdivisions=ROCK_STYLE['subdivisions'], radius=rock['radius'], location=rock['center'])
    obj = bpy.context.object
    obj.name = 'Opaque benchmark stone; supplied axis-angle motion'
    obj.rotation_mode = 'AXIS_ANGLE'
    obj.rotation_axis_angle = (rock['angle'], *Vector(rock['axis']).normalized())
    obj.data.materials.append(mat)
    obj['role'] = 'scripted_benchmark_rock'
    obj['motion'] = 'Exporter-defined benchmark, not cloth or Jolt simulation'
    return {'vertices': len(obj.data.vertices), 'faces': len(obj.data.polygons),
            'center': rock['center'], 'radius': rock['radius'], 'axis': rock['axis'], 'angle': rock['angle']}


def build_scene(args, frame, geometry):
    import bpy
    bpy.ops.object.select_all(action='SELECT')
    bpy.ops.object.delete(use_global=False)
    scene = bpy.context.scene
    scene.render.engine = 'BLENDER_EEVEE_NEXT' if args.engine == 'eevee' else 'CYCLES'
    if args.engine == 'eevee' and hasattr(scene, 'eevee'):
        scene.eevee.taa_render_samples = args.samples
        if hasattr(scene.eevee, 'use_raytracing'):
            scene.eevee.use_raytracing = False
    if args.engine == 'cycles':
        scene.cycles.device, scene.cycles.samples, scene.cycles.seed = 'CPU', args.samples, 17
        scene.cycles.use_denoising = False
        scene.cycles.transparent_max_bounces, scene.cycles.max_bounces = 64, 5
    scene.render.resolution_x, scene.render.resolution_y = args.width, args.height
    scene.render.resolution_percentage = 100
    scene.render.image_settings.file_format, scene.render.image_settings.color_mode = 'PNG', 'RGBA'
    scene.render.image_settings.color_depth = '8'
    scene.render.film_transparent = False
    scene.render.threads_mode, scene.render.threads = 'FIXED', 8
    scene.render.filepath = str(args.output.resolve())
    scene.view_settings.view_transform, scene.view_settings.look = 'AgX', 'AgX - Medium High Contrast'
    scene.view_settings.exposure, scene.view_settings.gamma = 0, 1
    scene.render.fps = 24
    world = bpy.data.worlds.new('Constant neutral laboratory world')
    world.use_nodes = True
    world.node_tree.nodes['Background'].inputs['Color'].default_value = (0.16, 0.16, 0.16, 1)
    world.node_tree.nodes['Background'].inputs['Strength'].default_value = 0.18
    scene.world = world
    camera = base.CAMERAS['panel']
    camdata = bpy.data.cameras.new('Fixed collection camera')
    camdata.type, camdata.ortho_scale = 'ORTHO', camera['ortho_scale']
    camdata.clip_start, camdata.clip_end = 0.05, 100
    cam = bpy.data.objects.new('Fixed collection camera', camdata)
    bpy.context.collection.objects.link(cam)
    cam.location = camera['location']
    base.point_at(cam, camera['target'])
    scene.camera = cam
    dark = base.material('Neutral charcoal lab backing', (0.016, 0.018, 0.020), 0.95)
    floor = base.material('Neutral lab bench', (0.034, 0.037, 0.040), 0.80)
    steel = base.material('Brushed steel stationary anchors', (0.32, 0.35, 0.37), 0.28, 0.84)
    base.box('Lab bench', (0, 1, -0.125), (30, 30, 0.18), floor)
    base.box('Matte back wall', (-2.85, 1.4, 2.0), (0.1, 15, 12), dark)
    lights = [base.add_light('Constant broad key', (3.3, -1.1, 5.3), (-1.6, 1.5, 1.8), 1000, 4.3),
              base.add_light('Constant silk rim', (-0.9, 4.2, 4.6), (-1.65, 1.5, 1.8), 480, 3.0),
              base.add_light('Constant low fill', (3.1, 4.0, 1.2), (-1.65, 1.5, 1.8), 160, 4.0)]
    markers = set()
    for particle in frame.get('particles', []):
        if base.is_anchor(particle):
            pos = tuple(particle.get('rest', particle['p']))
            if pos not in markers:
                marker = base.sphere('Fixed steel anchor ' + str(particle['id']), pos, .047, steel)
                marker['role'] = 'fixed_world_anchor'
                markers.add(pos)
    coverage = args.input/'coverage.tga'
    silk = make_coverage_material(coverage) if coverage.exists() else None
    for key, filename, role, label in [('carrier', 'mesh.obj', 'actual_exported_carrier', 'Actual whole panel carrier'),
                                       ('remnants', 'remnants.obj', 'cosmetic_stone_local_remnants', 'Actual exported cosmetic silk cards')]:
        if geometry[key]['faces']:
            make_exported_mesh(args.input/filename, label, role, silk)
    rock = make_rock(frame['rock'])
    # Base style descriptor is intentionally byte-for-byte equivalent to the
    # approved renderer's style fields. The added stone is hashed separately.
    style = {'camera': camera, 'lights': lights, 'world_color': [0.16]*3, 'world_strength': .18,
             'exposure': 0, 'gamma': 1, 'view_transform': 'AgX', 'look': 'AgX - Medium High Contrast',
             'silk_colors': base.CLASS_COLORS, 'silk_diameter_multiplier': 1.0, 'carrier_alpha': 'coverage RGB directly',
             'engine': args.engine, 'resolution': [args.width, args.height], 'samples': args.samples}
    style_hash = hashlib.sha256(json.dumps(style, sort_keys=True).encode()).hexdigest()
    impact_style = {'base': style, 'rock': ROCK_STYLE, 'representation': 'carrier+atlas only', 'shared_remnant_material': True}
    return {'camera_and_style': style, 'style_sha256': style_hash,
            'impact_style_sha256': hashlib.sha256(json.dumps(impact_style, sort_keys=True).encode()).hexdigest(),
            'rock_style': ROCK_STYLE, 'geometry': geometry, 'carrier': geometry['carrier'],
            'remnants': geometry['remnants'], 'rock': rock, 'stationary_anchor_count': len(markers),
            'blender_version': bpy.app.version_string, 'representation': 'carrier+atlas only',
            'silk_curve_count': 0, 'shared_remnant_atlas_material': True,
            'simulation_claim': 'Offline impact-response prototype; scripted rock motion; no Jolt or simulated wrapping'}


def main():
    args = parse_args()
    args.input = args.input.resolve()
    paths = [args.output, args.output.with_suffix('.render.json')] + ([args.scene] if args.scene else [])
    for path in paths:
        if path.exists() and not args.overwrite:
            raise FileExistsError(f'Refusing to replace {path}; pass --overwrite')
        path.parent.mkdir(parents=True, exist_ok=True)
    frame_path = args.input/'frame.json'
    frame = json.loads(frame_path.read_text())
    validate_frame(frame)
    geometry = inspect_geometry(args.input, frame)
    start = time.perf_counter()
    report = build_scene(args, frame, geometry)
    report.update({'input': str(args.input), 'state': frame,
                   'renderer_sha256': hashlib.sha256(Path(__file__).read_bytes()).hexdigest(),
                   'base_renderer_sha256': hashlib.sha256(Path(base.__file__).read_bytes()).hexdigest(),
                   'input_sha256': {p.name: hashlib.sha256(p.read_bytes()).hexdigest()
                     for p in (frame_path, args.input/'mesh.obj', args.input/'remnants.obj', args.input/'coverage.tga') if p.exists()}})
    report['scene_build_seconds'] = time.perf_counter()-start
    import bpy
    if args.scene:
        bpy.ops.file.pack_all()
        bpy.ops.wm.save_as_mainfile(filepath=str(args.scene.resolve()))
    if not args.validate_only:
        start = time.perf_counter()
        bpy.ops.render.render(write_still=True)
        report['render_seconds'] = time.perf_counter()-start
        report['output_sha256'] = hashlib.sha256(args.output.read_bytes()).hexdigest()
    report['rendered'] = not args.validate_only
    args.output.with_suffix('.render.json').write_text(json.dumps(report, indent=2)+'\n')
    print('IMPACT_RENDER ' + json.dumps(report, sort_keys=True))

if __name__ == '__main__':
    main()
