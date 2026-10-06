#!/usr/bin/env python3
"""Add honest labels to verified direct-OBJ renders; do not retouch the images.

python3 tools/compose_surface_studies.py --layout comparison --seeds 48 --variants legacy new
python3 tools/compose_surface_studies.py --layout seeds --seeds 48 117 305 --variants new
"""
import argparse
import json
import sys
sys.dont_write_bytecode = True
from pathlib import Path
from PIL import Image, ImageDraw, ImageFont
from render_surface_studies import ROOT, PROFILES, SEEDS, VARIANTS, VIEWS, MODES, digest, save_json

NAMES = {'shale_laminated':'Shale / laminated', 'slate_foliated':'Slate / foliated',
         'limestone_dissolution':'Limestone / dissolution'}
NAMES_ES = {'shale_laminated':'Lutita / paquetes laminados', 'slate_foliated':'Pizarra / foliación', 'limestone_dissolution':'Caliza / juntas y disolución'}
BG = '#eef0ef'
INK = '#202927'
MUTED = '#53645d'
ACCENT = '#276254'
FONT = Path('/usr/share/fonts/truetype/dejavu/DejaVuSans.ttf')
BOLD = FONT.with_name('DejaVuSans-Bold.ttf')


def font(size, bold=False):
    return ImageFont.truetype(str(BOLD if bold else FONT), size)


def record_for(render_dir, profile, seed, variant, view, mode, source_override=None, release_override=None, snapshot_override=None):
    name = f'{profile}-{seed}-{variant}-{view}-{mode}'
    record_path = render_dir/'provenance'/(name+'.json')
    record = json.loads(record_path.read_text())
    image = render_dir/record['image']
    if digest(image) != record['png_sha256']:
        raise ValueError('Render image changed after provenance: '+str(image))
    source_dir = (ROOT/record['obj']).parent
    if release_override is not None:
        if snapshot_override is None:
            raise ValueError('Archived release validation requires a frozen image/provenance snapshot')
        snapshot = json.loads(snapshot_override.read_text())['files']
        for path in (record_path,image):
            if snapshot.get(str(path.relative_to(render_dir))) != digest(path):
                raise ValueError('Historical image/provenance does not match frozen snapshot')
        release = [entry for entry in json.loads(release_override.read_text()) if entry['name']==source_dir.name]
        if len(release)!=1: raise ValueError('Expected one matching archived generation record')
        release = release[0]
        for filename in ('heightfield.obj','recipe.json'):
            if release['hashes'].get(filename) != record['source_hashes'][filename]:
                raise ValueError('Historical render and archived generation hashes disagree')
        if release['recipe'] != record['recipe'] or release['metrics'].get('accepted') is not True:
            raise ValueError('Historical recipe or accepted metrics disagree')
        metrics = release['metrics']
        record['verified_source_directory'] = None
        record['source_verification'] = {
            'method':'archived_generation_hashes_and_frozen_render_provenance',
            'release_record':str(release_override.relative_to(ROOT)), 'release_sha256':digest(release_override),
            'snapshot':str(snapshot_override.relative_to(ROOT)), 'snapshot_sha256':digest(snapshot_override),
            'checked_release_hashes':['heightfield.obj','recipe.json'],
            'metrics_source':'accepted archived generation metrics object',
            'old_obj_bytes_rechecked':False,
            'note':'Historical OBJ is not present locally; no new byte-level old-OBJ read is claimed.'}
    else:
        if source_override is not None: source_dir = source_override/source_dir.name
        for filename, expected in record['source_hashes'].items():
            if digest(source_dir/filename) != expected:
                raise ValueError('Stale render: source changed: '+str(source_dir/filename))
        metrics = json.loads((source_dir/'metrics.json').read_text())
        record['verified_source_directory'] = str(source_dir.relative_to(ROOT))
        record['source_verification'] = {'method':'direct_current_source_file_hashes',
            'checked_files':list(record['source_hashes']), 'old_obj_bytes_rechecked':True}
    if record.get('imported_geometry_normals_transforms_unchanged') is not True:
        raise ValueError('Missing imported geometry-preservation check')
    triangles = metrics.get('stages',{}).get('heightfield',{}).get('triangles',metrics.get('triangles'))
    if triangles != sum(stat['triangles'] for stat in record['mesh_stats']):
        raise ValueError('Exported metric triangle count differs from imported mesh')
    record['verified_mesh_triangles'] = triangles
    return record, image, record_path


def variant_label(record, language='en'):
    recipe = record['recipe']
    generator = recipe['generatorVersion']
    short_version = generator.rsplit('-',1)[-1]
    profile = recipe.get('profile', 'stratified')
    if language == 'es':
        if record['variant'] == 'legacy': return f'Perfil anterior: {profile} ({short_version})'
        if record['variant'] == 'control': return f'Control: rasgo nuevo = 0 ({short_version})'
        return f'Estudio nuevo: {profile} ({short_version})'
    if record['variant'] == 'legacy': return f'Legacy: {profile} ({short_version})'
    if record['variant'] == 'control': return f'Zero feature-depth control ({short_version})'
    return f'New: {profile} ({short_version})'


def draw_fit(draw, position, text, max_width, size, fill=INK, bold=False):
    while size > 14 and draw.textbbox((0,0),text,font=font(size,bold))[2] > max_width:
        size -= 1
    draw.text(position,text,font=font(size,bold),fill=fill)


def compose(args, view, mode, seed=None, variant=None):
    es = args.language == 'es'
    rows = []
    for profile in args.profiles:
        if args.layout == 'angles':
            row = [record_for(args.render_dir,profile,seed,'new',v,mode) for v in args.views]
        elif args.layout == 'evolution':
            row = [record_for(args.baseline_render_dir,profile,seed,'new',view,mode,source_override=args.baseline_source_dir,release_override=args.baseline_release_file,snapshot_override=args.baseline_snapshot),
                   record_for(args.render_dir,profile,seed,'new',view,mode)]
        else:
            cells = [(seed,v) for v in args.variants] if args.layout == 'comparison' else [(s,variant) for s in args.seeds]
            row = [record_for(args.render_dir,profile,s,v,view,mode) for s,v in cells]
        physical = [r[0]['settings']['physical_comparison'] for r in row]
        cameras = [r[0]['settings']['camera'] for r in row]
        lights = [r[0]['settings']['lights'] for r in row]
        full_settings = [r[0]['settings'] for r in row]
        if args.layout == 'evolution' and any(item != full_settings[0] for item in full_settings):
            raise ValueError('Candidate evolution must match every render setting')
        if any(item != physical[0] for item in physical) or (args.layout != 'angles' and any(item != cameras[0] for item in cameras)) or any(item != lights[0] for item in lights):
            raise ValueError('Panel comparisons have unmatched physical size, cameras or lights')
        rows.append((profile,row))
    sizes = {Image.open(p).size for _,row in rows for _,p,_ in row}
    if len(sizes) != 1: raise ValueError('All source images on a sheet must use the same dimensions')
    source_w,source_h = next(iter(sizes))
    iw = args.tile_size or source_w
    ih = round(source_h * iw/source_w)
    margin,gap,header,rowhead,caption,footer = 28,16,130,48,68,102
    ncols = len(rows[0][1])
    width = margin*2 + iw*ncols + gap*(ncols-1)
    row_height = rowhead + ih + caption
    height = header + row_height*len(rows) + gap*(len(rows)-1) + footer
    canvas = Image.new('RGB',(width,height),BG)
    draw = ImageDraw.Draw(canvas)
    detail = ' / central detail' if view == 'detail' else (' / alternate angle' if view == 'alternate' else '')
    title = f'Surface families / seed {seed}' if seed is not None else f'Surface families / three seeds / {variant}'
    if args.layout == 'angles': title = f'New families / seed {seed} / two angles'
    if args.layout == 'evolution': title = f'Candidate {args.baseline_id} vs candidate {args.current_id} / seed {seed}'
    if es:
        title = f'Semilla {seed}' if seed is not None else 'Tres semillas'
        if args.layout == 'angles': title += ' / dos ángulos'
        if args.layout == 'evolution': title = f'Estudios {args.baseline_id} y {args.current_id} / semilla {seed}'
        detail = '' if view == 'main' else (' / detalle central' if view == 'detail' else ' / ángulo alternativo')
    if args.label: title = args.label + ' / ' + title
    draw_fit(draw,(margin,24),title+detail,width-2*margin,34,bold=True)
    sub = 'Actual imported OBJ triangles' if mode == 'wireframe' else 'Neutral clay / actual imported OBJ geometry'
    if es: sub = 'Aristas reales del OBJ importado' if mode == 'wireframe' else 'Arcilla neutra / geometría OBJ real'
    draw.text((margin,72),sub,font=font(22),fill=ACCENT)
    subtitle = 'Same source mesh, clay and fixed lights; main and alternate camera views.' if args.layout == 'angles' else 'Matched camera, light and physical dimensions within each row.'
    if es: subtitle = 'Misma malla, material y luces; cámaras principal y alternativa.' if args.layout == 'angles' else 'Cámara, luz y dimensiones físicas iguales dentro de cada fila.'
    draw_fit(draw,(margin,103),subtitle,width-2*margin,17,fill=MUTED)
    sources = []
    for ri,(profile,row) in enumerate(rows):
        top = header+ri*(row_height+gap)
        recipe = row[0][0]['recipe']
        physical = f"{recipe['size']:g} m square; relief limit {recipe['relief']:g} m; backing {recipe['backDepth']:g} m"
        if es:
            number = lambda x: f'{x:g}'.replace('.',',')
            physical = f"{number(recipe['size'])} × {number(recipe['size'])} m; relieve máx. {number(recipe['relief'])} m; respaldo {number(recipe['backDepth'])} m"
        draw_fit(draw,(margin,top+8),(NAMES_ES if es else NAMES)[profile]+'  |  '+physical,width-2*margin,23,bold=True)
        for ci,(record,path,record_path) in enumerate(row):
            left = margin+ci*(iw+gap)
            image_top = top+rowhead
            with Image.open(path) as image:
                panel = image.convert('RGB')
                if panel.size != (iw,ih): panel = panel.resize((iw,ih),Image.Resampling.LANCZOS)
                canvas.paste(panel,(left,image_top))
            draw.rectangle((left,image_top,left+iw-1,image_top+ih-1),outline='#c4ccc8',width=1)
            label = variant_label(record,args.language)
            if args.layout == 'angles':
                angle = {'main':'Principal','alternate':'Alternativo','detail':'Detalle'}[record['view']] if es else record['view'].capitalize()
                label = angle + ' / ' + record['profile']
            if args.layout == 'evolution': label = ('Estudio' if es else 'Candidate') + f" {args.baseline_id if ci==0 else args.current_id} / {record['profile']}"
            if args.layout == 'seeds': label = ('Semilla' if es else 'Seed') + f" {record['seed']} / {label}"
            draw_fit(draw,(left,image_top+ih+10),label,iw,21,bold=True)
            count = record['verified_mesh_triangles']
            text = f"{count:,} triangles / OBJ {record['source_hashes']['heightfield.obj'][:12]}"
            if es: text = f"{count:,}".replace(',','.') + f" triángulos / OBJ {record['source_hashes']['heightfield.obj'][:12]}"
            draw_fit(draw,(left,image_top+ih+39),text,iw,16,fill=MUTED)
            sources.append({'image':str(path.relative_to(ROOT)), 'png_sha256':record['png_sha256'],
                            'provenance':str(record_path.relative_to(ROOT)), 'provenance_sha256':digest(record_path),
                            'obj_sha256':record['source_hashes']['heightfield.obj'],
                            'verified_source_directory':record['verified_source_directory'], 'triangles_from_verified_metrics':record['verified_mesh_triangles'], 'source_verification':record['source_verification']})
    foot_y = height-footer+14
    preservation = 'Sin texturas ni modificadores de malla; normales OBJ conservadas.' if es else 'No textures, displacement, geometry modifiers or per-object rescaling. Imported normals preserved.'
    draw_fit(draw,(margin,foot_y),preservation,width-2*margin,17,fill=MUTED)
    note = f'Candidate {args.baseline_id}: archived geometry. Candidate {args.current_id}: revised geometry; identical physical and render settings.' if args.layout == 'evolution' else 'Legacy and new are distinct generators. Zero-depth controls isolate the new family feature.'
    if es: note = 'Geometría matemática; sin imágenes como entrada. Morfología todavía simplificada.'
    if args.note: note = args.note
    draw_fit(draw,(margin,foot_y+28),note,width-2*margin,17,fill=MUTED)
    limits = 'Relieve sin voladizos; laterales y respaldo cerrados. No simula erosión.' if es else 'Single-valued fronts with closed sides/back; no overhangs or erosion simulation.'
    draw_fit(draw,(margin,foot_y+56),limits,width-2*margin,16,fill=MUTED)
    stem = f'comparison-seed-{seed}' if seed is not None else f'{variant}-three-seeds'
    if args.layout == 'angles': stem = f'new-families-two-angles-seed-{seed}'
    elif args.layout == 'evolution': stem = f'candidate{args.baseline_id}-vs-candidate{args.current_id}-seed-{seed}'
    elif seed is not None: stem += '-'+ '-'.join(args.variants)
    stem += '-'+ ('-'.join(args.views) if args.layout == 'angles' else view)+'-'+mode
    if es: stem += '-es'
    out = args.output_dir/(stem+'.png')
    canvas.save(out)
    save_json(args.output_dir/(stem+'.json'), {'image':str(out.relative_to(ROOT)),'png_sha256':digest(out),
        'composer_sha256':digest(Path(__file__)),'layout':args.layout,'view':view,'mode':mode,'language':args.language,
        'pixel_operations':(['LANCZOS display downsampling; original full-resolution PNGs retained'] if args.tile_size else ['unaltered source PNG pixels pasted into labeled grid'])+['one-pixel border'],
        'source_resolution':[source_w,source_h],'display_resolution':[iw,ih],
        'sources':sources,'labels_disclose_distinct_generators':True})
    print(out,flush=True)


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--layout', choices=['comparison','seeds','evolution','angles'], default='comparison')
    parser.add_argument('--label', default='', help='Visible preliminary or candidate label')
    parser.add_argument('--note', default='', help='Visible candid status/limitation note')
    parser.add_argument('--language', choices=['en','es'], default='en')
    parser.add_argument('--tile-size', type=int, default=0, help='Optional display thumbnail width; does not alter source renders')
    parser.add_argument('--profiles',nargs='+',choices=PROFILES,default=list(PROFILES))
    parser.add_argument('--seeds',nargs='+',type=int,choices=SEEDS,default=[48])
    parser.add_argument('--variants',nargs='+',choices=VARIANTS,default=['legacy','new'])
    parser.add_argument('--views',nargs='+',choices=VIEWS,default=['main'])
    parser.add_argument('--modes',nargs='+',choices=MODES,default=['neutral'])
    parser.add_argument('--render-dir',type=Path,default=ROOT/'renders/surface-v13')
    parser.add_argument('--output-dir',type=Path,default=ROOT/'renders/surface-v13/sheets')
    parser.add_argument('--baseline-id',type=int,default=1)
    parser.add_argument('--current-id',type=int,default=2)
    parser.add_argument('--baseline-render-dir',type=Path,default=ROOT/'renders/candidate1')
    parser.add_argument('--baseline-source-dir',type=Path,default=ROOT/'controls/candidate1/out')
    parser.add_argument('--baseline-release-file',type=Path)
    parser.add_argument('--baseline-snapshot',type=Path)
    args = parser.parse_args()
    if args.tile_size and args.tile_size < 256: parser.error('Thumbnail width must be at least 256 pixels')
    args.output_dir.mkdir(parents=True,exist_ok=True)
    for view in (args.views[:1] if args.layout == 'angles' else args.views):
        for mode in args.modes:
            for selector in (args.seeds if args.layout != 'seeds' else args.variants):
                compose(args,view,mode,seed=selector if args.layout!='seeds' else None,
                        variant=selector if args.layout=='seeds' else None)


if __name__ == '__main__': main()
