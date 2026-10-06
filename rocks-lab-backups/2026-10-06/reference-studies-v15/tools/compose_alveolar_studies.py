#!/usr/bin/env python3
"""Compose three hash-verified native-pixel JPG comparisons; retain source PNGs."""
import argparse
import json
from pathlib import Path
import sys
sys.dont_write_bytecode = True
from PIL import Image, ImageDraw, ImageFont
from render_alveolar_studies import ROOT, SEEDS, VARIANTS, ROWS, digest, save_json, stable_hash, stem_for

FONT = '/usr/share/fonts/truetype/dejavu/DejaVuSans.ttf'
LABELS = {'support': 'Soporte original · v11', 'limestone': 'Caliza · candidate6 · v13',
          'alveolar': 'Alveolos · experimento · v14'}
ROW_LABELS = ['Vista general · luz de estudio', 'Detalle · primer parche aceptado',
              'Detalle · aristas reales del OBJ', 'Detalle · iluminación lateral real']


def font(size, bold=False):
    return ImageFont.truetype(FONT.replace('.ttf', '-Bold.ttf') if bold else FONT, size)


def validate_row(records):
    if len(records) != 3 or any(r['settings'] != records[0]['settings'] for r in records[1:]):
        raise ValueError('Comparison requires three variants with identical render settings')
    if any(r['renderer_dependencies'] != records[0]['renderer_dependencies'] for r in records[1:]):
        raise ValueError('Comparison mixes different renderer dependencies')


def layout_for(tile_width, tile_height, cols, rows):
    margin, gap, header, row_header, caption, footer = 20, 12, 152, 38, 34, 78
    return {'tile_width': tile_width, 'tile_height': tile_height, 'margin': margin, 'gap': gap,
            'header': header, 'row_header': row_header, 'caption': caption, 'footer': footer,
            'row_height': row_header + tile_height + caption,
            'width': 2 * margin + cols * tile_width + (cols - 1) * gap,
            'height': header + rows * (row_header + tile_height + caption) + (rows - 1) * gap + footer}


def record_for(render_dir, seed, variant, row):
    record_path = render_dir / 'provenance' / (stem_for(seed, variant, *row) + '.json')
    record = json.loads(record_path.read_text())
    identity = (record.get('seed'), record.get('variant'), record.get('view'), record.get('mode'), record.get('lighting'))
    if identity != (seed, variant, *row):
        raise ValueError('Provenance identity does not match requested comparison cell')
    image_path = render_dir / record['image']
    if digest(image_path) != record['png_sha256'] or stable_hash(record['settings']) != record['settings_sha256']:
        raise ValueError('Image or settings changed since render provenance')
    if record.get('imported_geometry_normals_transforms_unchanged') is not True:
        raise ValueError('Missing imported geometry preservation check')
    source_dir = (ROOT / record['obj']).parent
    for filename, sha in record['source_hashes'].items():
        if digest(source_dir / filename) != sha:
            raise ValueError('Stale source file: ' + str(source_dir / filename))
    for source in record['auxiliary_sources']:
        if digest(ROOT / source['path']) != source['sha256']:
            raise ValueError('Stale detail anchor source')
    metrics = json.loads((source_dir / 'metrics.json').read_text())
    triangles = metrics.get('stages', {}).get('heightfield', {}).get('triangles', metrics.get('triangles'))
    if metrics.get('accepted') is not True or triangles != sum(x['triangles'] for x in record['mesh_stats']):
        raise ValueError('Mesh metrics and imported geometry disagree')
    record['verified_triangles'] = triangles
    return record, image_path, record_path


def draw_fit(draw, position, text, width, size=24, bold=False, fill='#22342c'):
    while size > 12 and draw.textbbox((0, 0), text, font=font(size, bold))[2] > width:
        size -= 1
    draw.text(position, text, font=font(size, bold), fill=fill)


def compose(render_dir, output_dir, seed):
    rows = [[record_for(render_dir, seed, variant, row) for variant in VARIANTS] for row in ROWS]
    for row in rows:
        validate_row([r[0] for r in row])
    sizes = set()
    for row in rows:
        for _, path, _ in row:
            with Image.open(path) as im:
                sizes.add(im.size)
    if len(sizes) != 1:
        raise ValueError('Native source PNG sizes differ')
    iw, ih = next(iter(sizes))
    layout = layout_for(iw, ih, 3, len(rows))
    width, height, margin, gap = (layout[k] for k in ('width', 'height', 'margin', 'gap'))
    sheet = Image.new('RGB', (width, height), '#eef0ef')
    draw = ImageDraw.Draw(sheet)
    draw_fit(draw, (margin, 20), f'Bloques y erosión localizada · semilla {seed}', width - 2 * margin, 34, True)
    draw_fit(draw, (margin, 67), 'Cámara, escala, arcilla e iluminación iguales dentro de cada fila.', width - 2 * margin, 23)
    draw_fit(draw, (margin, 100), 'Parche de 2 × 2 m · relieve 0,55 m · respaldo 0,30 m · malla 256² · fuente 1025²', width - 2 * margin, 21)
    sources = []
    for row_index, row in enumerate(rows):
        top = layout['header'] + row_index * (layout['row_height'] + gap)
        draw_fit(draw, (margin, top), ROW_LABELS[row_index], width - 2 * margin, 24, True)
        for col, (record, path, record_path) in enumerate(row):
            x, y = margin + col * (iw + gap), top + layout['row_header']
            with Image.open(path) as im:
                sheet.paste(im.convert('RGB'), (x, y))
            draw_fit(draw, (x, y + ih + 7), LABELS[record['variant']], iw, 21, True)
            sources.append({'variant': record['variant'], 'row': list(ROWS[row_index]),
                            'image': str(path.relative_to(ROOT)), 'png_sha256': record['png_sha256'],
                            'provenance': str(record_path.relative_to(ROOT)), 'provenance_sha256': digest(record_path),
                            'obj_sha256': record['source_hashes']['heightfield.obj'],
                            'verified_triangles': record['verified_triangles'],
                            'settings_sha256': record['settings_sha256']})
    y = height - layout['footer'] + 10
    draw_fit(draw, (margin, y), 'OBJ directo; normales conservadas. Sin texturas ni displacement de render. PNG originales retenidos.', width - 2 * margin, 19)
    draw_fit(draw, (margin, y + 29), 'Experimento sin promover. Depresiones ciegas en heightfield; sin túneles, socavados ni simulación geológica.', width - 2 * margin, 19)
    output = output_dir / f'alveolos-comparativa-semilla-{seed}.jpg'
    sheet.save(output, quality=91, subsampling=0, optimize=True)
    provenance = {'image': str(output.relative_to(ROOT)), 'jpeg_sha256': digest(output),
                  'composer_sha256': digest(Path(__file__)), 'seed': seed, 'layout': layout,
                  'pixel_operations': ['Source PNGs pasted at native resolution without resizing or retouching',
                                       'Labels added outside source pixels', 'JPEG compression quality 91, 4:4:4'],
                  'sources': sources, 'all_rows_equal_settings': True, 'experimental_not_promoted': True}
    save_json(output.with_suffix('.json'), provenance)
    print(output, flush=True)
    return provenance


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--seeds', nargs='+', type=int, choices=SEEDS, default=list(SEEDS))
    parser.add_argument('--render-dir', type=Path, default=ROOT / 'renders/alveolar-v14')
    parser.add_argument('--output-dir', type=Path, default=ROOT / 'renders/alveolar-v14/comparisons')
    args = parser.parse_args()
    args.output_dir.mkdir(parents=True, exist_ok=True)
    records = [compose(args.render_dir, args.output_dir, seed) for seed in args.seeds]
    save_json(args.output_dir / 'comparison_manifest.json', {'comparisons': records})


if __name__ == '__main__':
    main()
