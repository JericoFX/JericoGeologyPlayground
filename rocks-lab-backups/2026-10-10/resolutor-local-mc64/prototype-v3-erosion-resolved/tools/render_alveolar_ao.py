#!/usr/bin/env python3
"""Re-render frozen alveolar OBJ scenes with a bounded, ray-traced AO accent.

blender -b -t 8 -P tools/render_alveolar_ao.py -- --seeds 48 117 305
python3 tools/render_alveolar_ao.py --compose-only --seeds 48 117 305

Original Cycles scenes already have physical environmental shading. The left
column has no EXTRA AO multiplier. No geometry, normals or old files are edited.
"""
import argparse
import copy
import json
import math
from pathlib import Path
import sys
import time

sys.dont_write_bytecode = True
ROOT = Path(__file__).resolve().parents[1]
sys.path.insert(0, str(ROOT / 'tools'))
import render_alveolar_studies as original
import render_surface_studies as base

SOURCE = ROOT / 'renders/alveolar-v14'
OUTPUT = ROOT / 'renders/alveolar-v14-ao'


def settings_with_ao(settings, strength, radius, samples):
    if (not math.isfinite(strength) or not 0 <= strength <= 1
            or not math.isfinite(radius) or radius <= 0
            or not isinstance(samples, int) or isinstance(samples, bool) or samples < 1):
        raise ValueError('Invalid AO strength, radius or samples')
    result = copy.deepcopy(settings)
    result['ambient_occlusion'] = {
        'method': 'Cycles ShaderNodeAmbientOcclusion ray visibility',
        'strength': strength, 'distance_meters': radius, 'samples': samples,
        'inside': False, 'only_local': True,
        'formula': 'base_color * ((1 - strength) + strength * AO_visibility)',
        'baseline': 'Cycles physical lighting retained; zero strength means no extra AO',
        'textures_or_image_postprocessing': False}
    return result


def verify_baseline(record):
    if record.get('imported_geometry_normals_transforms_unchanged') is not True:
        raise ValueError('Missing original imported geometry check')
    if base.stable_hash(record['settings']) != record['settings_sha256']:
        raise ValueError('Original settings changed')
    directory = (ROOT / record['obj']).parent
    for name, sha in record['source_hashes'].items():
        if base.digest(directory / name) != sha:
            raise ValueError('Source changed: ' + name)
    for source in record['auxiliary_sources']:
        if base.digest(ROOT / source['path']) != source['sha256']:
            raise ValueError('Source for camera changed')
    for name, sha in record['renderer_dependencies'].items():
        if base.digest(ROOT / 'tools' / name) != sha:
            raise ValueError('Original renderer changed')
    if base.digest(SOURCE / record['image']) != record['png_sha256']:
        raise ValueError('Original image changed')


def apply_ao(material, settings):
    """Visibility is calculated by Cycles against the actual imported triangles."""
    ao = settings['ambient_occlusion']
    if ao['strength'] == 0:
        return
    tree = material.node_tree
    shader = tree.nodes.get('Principled BSDF')
    visibility = tree.nodes.new('ShaderNodeAmbientOcclusion')
    visibility.name = 'Documented ray-traced ambient occlusion'
    visibility.samples = ao['samples']
    visibility.inside = ao['inside']
    visibility.only_local = ao['only_local']
    visibility.inputs['Distance'].default_value = ao['distance_meters']
    visibility.inputs['Color'].default_value = (*settings['material']['base_color'], 1.)
    mix = tree.nodes.new('ShaderNodeMixRGB')
    mix.name = 'Bounded AO accent'
    mix.blend_type = 'MIX'
    mix.inputs[0].default_value = ao['strength']
    mix.inputs[1].default_value = (*settings['material']['base_color'], 1.)
    tree.links.new(visibility.outputs['Color'], mix.inputs[2])
    tree.links.new(mix.outputs['Color'], shader.inputs['Base Color'])


def validate_pair(off, on):
    without_ao = lambda r: {k: v for k, v in r['settings'].items() if k != 'ambient_occlusion'}
    if without_ao(off) != without_ao(on):
        raise ValueError('Only AO may differ between comparison scenes')
    if (off['source_hashes'] != on['source_hashes'] or off['mesh_stats'] != on['mesh_stats']
            or not all(r.get('imported_geometry_normals_transforms_unchanged') for r in (off, on))):
        raise ValueError('Comparison geometry differs')


def render_one(record_path, strength, radius, samples):
    import bpy
    record = json.loads(record_path.read_text())
    verify_baseline(record)
    settings = settings_with_ao(record['settings'], strength, radius, samples)
    if bpy.app.version_string != settings['blender_version']:
        raise ValueError('Blender version differs from original render')
    tag = 'ao-off' if strength == 0 else 'ao-soft'
    stem = record_path.stem + '-' + tag
    path = OUTPUT / (stem + '.png')
    provenance = OUTPUT / 'provenance' / (stem + '.json')
    if path.exists() or provenance.exists():
        raise FileExistsError('Refusing to overwrite AO output: ' + stem)
    start = time.monotonic()
    bpy.data.orphans_purge(do_recursive=True)
    scene, material = base.setup_scene(settings)
    apply_ao(material, settings)
    bpy.ops.object.select_all(action='DESELECT')
    bpy.ops.wm.obj_import(filepath=str(ROOT / record['obj']), forward_axis='Y', up_axis='Z')
    meshes = [o for o in bpy.context.selected_objects if o.type == 'MESH']
    actual = sorted(base.mesh_fingerprint(o) for o in meshes)
    archived = sorted(o['imported_mesh_sha256'] for o in record['mesh_stats'])
    if not meshes or actual != archived:
        raise ValueError('Imported mesh differs from archived render')
    for obj in meshes:
        if obj.modifiers or not obj.data.has_custom_normals:
            raise ValueError('Mesh modifiers or missing original custom normals')
        obj.data.materials.clear()
        obj.data.materials.append(material)
    scene.render.filepath = str(path)
    bpy.ops.render.render(write_still=True)
    if sorted(base.mesh_fingerprint(o) for o in meshes) != actual:
        raise RuntimeError('Geometry, normals or transform changed during rendering')
    verify_baseline(record)
    result = copy.deepcopy(record)
    result.update({'image': path.name, 'png_sha256': base.digest(path),
                   'settings': settings, 'settings_sha256': base.stable_hash(settings),
                   'ao_renderer_sha256': base.digest(Path(__file__)),
                   'baseline_record': str(record_path.relative_to(ROOT)),
                   'baseline_record_sha256': base.digest(record_path),
                   'elapsed_seconds': round(time.monotonic() - start, 3),
                   'ao_tag': tag, 'only_render_shading_changed': True,
                   'scene_sample_count_unchanged': scene.cycles.samples == record['settings']['samples'],
                   'original_image_exact_replay': base.digest(path) == record['png_sha256'] if strength == 0 else None})
    base.save_json(provenance, result)
    print('AO_RENDER_DONE ' + str(path), flush=True)
    return result


def compose(seed, variant):
    """Paste verified render pixels unchanged; labels occupy separate margins."""
    from PIL import Image, ImageDraw, ImageFont
    rows, sources = [], []
    for view in ('main', 'detail'):
        pair = []
        for tag in ('ao-off', 'ao-soft'):
            name = original.stem_for(seed, variant, view, 'neutral', 'studio') + '-' + tag
            provenance = OUTPUT / 'provenance' / (name + '.json')
            record = json.loads(provenance.read_text())
            path = OUTPUT / record['image']
            if base.digest(path) != record['png_sha256']:
                raise ValueError('AO render pixels changed')
            if base.stable_hash(record['settings']) != record['settings_sha256']:
                raise ValueError('AO settings changed')
            verify_baseline(json.loads((ROOT / record['baseline_record']).read_text()))
            pair.append(record)
            sources.append({'image': str(path.relative_to(ROOT)), 'png_sha256': base.digest(path),
                            'provenance': str(provenance.relative_to(ROOT)),
                            'provenance_sha256': base.digest(provenance)})
        validate_pair(*pair)
        rows.append(pair)
    width, height = rows[0][0]['settings']['resolution']
    margin, gap, header, row_extra, footer = 24, 16, 120, 68, 112
    canvas = Image.new('RGB', (2 * width + 2 * margin + gap,
        2 * (height + row_extra) + header + footer), '#eef0ef')
    draw = ImageDraw.Draw(canvas)
    def text(x, y, value, size=21, bold=False):
        suffix = '-Bold' if bold else ''
        font = ImageFont.truetype('/usr/share/fonts/truetype/dejavu/DejaVuSans' + suffix + '.ttf', size)
        draw.text((x, y), value, font=font, fill='#26332d')
    labels = {'alveolar': 'Alveolos · candidate8', 'support': 'Soporte original', 'limestone': 'Caliza · candidate6'}
    text(margin, 20, f'{labels[variant]} · AO suave · semilla {seed}', 31, True)
    text(margin, 67, 'Misma malla, cámara, luz y arcilla gris. Solo cambia el refuerzo AO.', 22)
    for index, pair in enumerate(rows):
        y = header + index * (height + row_extra)
        text(margin, y, 'Vista general' if index == 0 else 'Detalle · primer parche', 23, True)
        for column, record in enumerate(pair):
            x = margin + column * (width + gap)
            with Image.open(OUTPUT / record['image']) as im:
                if im.size != (width, height):
                    raise ValueError('Unexpected render image dimensions')
                canvas.paste(im.convert('RGB'), (x, y + 33))
            label = 'Sin refuerzo AO' if column == 0 else 'AO suave · intensidad 30 %'
            text(x, y + height + 39, label, 21, True)
    bottom = canvas.height - footer + 18
    text(margin, bottom, 'AO por rayos · radio 0,12 m · 32 muestras AO · Blender Cycles 4.3.2', 20)
    text(margin, bottom + 29, 'Cycles conserva su sombreado físico en ambas columnas. OBJ y normales intactos.', 19)
    text(margin, bottom + 57, 'Imágenes pegadas a resolución original, sin retoque, curvas ni sombras pintadas.', 19)
    path = OUTPUT / 'comparisons' / f'{variant}-ao-semilla-{seed}.png'
    if path.exists():
        raise FileExistsError('Refusing to overwrite comparison')
    canvas.save(path)
    base.save_json(path.with_suffix('.json'), {
        'image': str(path.relative_to(ROOT)), 'png_sha256': base.digest(path), 'seed': seed,
        'variant': variant, 'sources': sources, 'only_ao_settings_differ': True,
        'layout_size': list(canvas.size), 'pixel_operations': ['Native resolution paste', 'Labels outside render pixels'],
        'composer_sha256': base.digest(Path(__file__))})
    print('AO_COMPARISON_DONE ' + str(path), flush=True)


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--seeds', nargs='+', type=int, choices=original.SEEDS, default=[48, 117, 305])
    parser.add_argument('--variants', nargs='+', choices=original.VARIANTS, default=['alveolar'])
    parser.add_argument('--compose-only', action='store_true')
    args = parser.parse_args(sys.argv[sys.argv.index('--') + 1:] if '--' in sys.argv else sys.argv[1:])
    for sub in ('provenance', 'comparisons', 'evidence'):
        (OUTPUT / sub).mkdir(parents=True, exist_ok=True)
    for seed in args.seeds:
        for variant in args.variants:
            if args.compose_only:
                compose(seed, variant)
                continue
            for view in ('main', 'detail'):
                record = SOURCE / 'provenance' / (original.stem_for(seed, variant, view, 'neutral', 'studio') + '.json')
                off, on = [render_one(record, strength, .12, 32) for strength in (0., .30)]
                validate_pair(off, on)


if __name__ == '__main__':
    main()
