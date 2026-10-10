from pathlib import Path
from PIL import Image
import hashlib,json,math
HERE=Path(__file__).resolve().parent;ROOT=HERE.parents[1]
def sha(p):return hashlib.sha256(p.read_bytes()).hexdigest()
old=json.loads((HERE/'original-source-snapshot.json').read_text());new={str(p.relative_to(ROOT)):sha(p) for p in (ROOT/'original').rglob('*') if p.is_file()}
assert old==new, {'new':list(new.keys()-old.keys()),'removed':list(old.keys()-new.keys()),'modified':[p for p in old if old[p]!=new.get(p)]}
records=[]
for p in sorted((HERE/'renders').glob('*.provenance.json')):
 d=json.loads(p.read_text());img=p.with_suffix('').with_suffix('.png')
 assert sha(Path(d['obj']))==d['obj_sha256'],str(p)+' OBJ CHANGED'
 assert sha(img)==d['png_sha256'],str(p)+' PNG CHANGED'
 assert d['settings_sha256']==hashlib.sha256(json.dumps(d['settings'],sort_keys=True).encode()).hexdigest()
 assert d['wrapper_sha256']==sha(HERE/'render_evidence.py')
 assert d['renderer_source_sha256']==sha(ROOT/'original/prototype-v3-erosion-resolved/tools/render_surface_studies.py')
 assert d['imported_geometry_normals_transforms_unchanged'] is True
 with Image.open(img) as im: im.verify()
 records.append({'image':str(img.relative_to(HERE)), 'provenance':str(p.relative_to(HERE)), 'provenance_sha256':sha(p), **d})
assert len(records)==16, len(records)
byname={d['job']['name']:d for d in records}
pairs=[]
for seed,views in ((31,('main','join-neutral','join-edges')),(14,('main','micro-edges'))):
 for view in views:
  a=byname[f'rejected-{seed}-{view}'];b=byname[f'corrected-{seed}-{view}'];assert a['settings']==b['settings'];pairs.append({'seed':seed,'view':view,'settings_identical':True,'settings_sha256':a['settings_sha256']})
ref=byname['corrected-4-main']['settings']
assert all(byname[f'corrected-{s}-main']['settings']==ref for s in (4,5,8,14,25,29,31))
preserve=[]
for stage in ('base','stratified','eroded'):
 a=ROOT/f'evidence/baseline/48/output/{stage}.obj';b=ROOT/f'evidence/corrected/48/output/{stage}.obj';assert sha(a)==sha(b);preserve.append({'stage':stage,'sha256':sha(a),'byte_identical':True})
artifacts=[]
for name in ('hero-aprobada-48.png','siete-corregidas.png','antes-despues-union-31.png','antes-despues-micro-14.png'):
 p=HERE/name
 with Image.open(p) as im: size=list(im.size); im.verify()
 artifacts.append({'path':name,'sha256':sha(p),'size_bytes':p.stat().st_size,'resolution':size})
manifest={'schema_version':1,'purpose':'Reproducible direct C++ OBJ render evidence; no geometry repair is performed by the renderer','render_verification':'passed','source_original_file_count':len(old),'original_source_files_unchanged':True,'original_source_snapshot_sha256':sha(HERE/'original-source-snapshot.json'),'approved_seed48_byte_identical':preserve,'render_count':len(records),'artifacts':artifacts,'matched_comparison_pairs':pairs,'seven_main_settings_identical':True,'renders':records,'composition_record_sha256':sha(HERE/'composition-record.json'),'verification_script_sha256':sha(Path(__file__)),'geometry_review_scope':'Rendering alone does not establish code correctness or absence of self intersections. Consult separate code and geometry review. Job status labels retain their render-time context.','limitations':['Rejected before images are diagnostic exports, not the current PC model.','Microscopic seed14 frame is 14 micrometers wide, camera-only magnification of 192857.14 relative to the main frame. No vertex scaling or modifications.','Microscope rendering uses Blender floating-point import/render precision. Exact contraction distance 1.7378896249769805e-6 meters comes from original OBJ coordinates.','All edge images use Blender ShaderNodeWireframe on actual imported triangle edges; no hand drawn or invented edges.','Labels/margins and uniform image resampling were applied only for evidence sheet layout.'],'source_files':{str(p.relative_to(ROOT)):sha(p) for p in [ROOT/'evidence/baseline-cells.json',ROOT/'evidence/preservation.json']}}
(HERE/'manifest.json').write_text(json.dumps(manifest,indent=2,ensure_ascii=False)+'\n')
(HERE/'SHA256SUMS').write_text(''.join(f"{sha(p)}  {p.relative_to(HERE)}\n" for p in sorted(HERE.rglob('*')) if p.is_file() and p.name!='SHA256SUMS' and 'library' not in p.name and p.suffix!='.log'))
print(json.dumps({'passed':True,'renders':len(records),'original_source_files_unchanged':len(old),'settings_identical_pairs':len(pairs),'artifacts':artifacts},indent=2))
