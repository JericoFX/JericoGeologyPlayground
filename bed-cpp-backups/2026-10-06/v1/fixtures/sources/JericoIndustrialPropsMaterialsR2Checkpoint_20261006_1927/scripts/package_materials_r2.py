"""Package six corrected assets and a code-only immutable R2 checkpoint."""
import hashlib,json,shutil,zipfile
from pathlib import Path
ROOT=Path(__file__).resolve().parents[1];OUT=ROOT/'output/delivery';STAMP='20261006_1927'
def sha(data):return hashlib.sha256(data).hexdigest()
def freeze_zip(label,entries):
 temporary=OUT/(label+'-working.zip')
 with zipfile.ZipFile(temporary,'w',compression=zipfile.ZIP_DEFLATED,compresslevel=9) as z:
  for name,data in sorted(entries.items()):
   i=zipfile.ZipInfo(name,(2026,10,6,19,27,0));i.compress_type=zipfile.ZIP_DEFLATED;i.external_attr=0o644<<16;z.writestr(i,data)
 data=temporary.read_bytes();target=OUT/(label+'_'+sha(data)[:12]+'.zip')
 with target.open('xb') as f:f.write(data)
 temporary.unlink()
 with zipfile.ZipFile(target) as z:assert z.testzip() is None
 assert target.stat().st_size<20_000_000,target
 return dict(path=str(target),bytes=len(data),sha256=sha(data))
manifest=json.loads((ROOT/'output/exports_verified/industrial-export-manifest-working.json').read_text())
verification=json.loads((ROOT/'output/verification/materials-r2-verification.json').read_text());by_id={v['label']:v for v in verification['assets']}
packages=[]
for record in manifest:
 label=record['label'];v=by_id[label]
 if not v['affected']:continue
 entries={}
 for path in [record[k] for k in ['glb','obj','metadata','mtl']]+list(record['textures'].values()):
  p=Path(path);assert p.name not in entries;entries[p.name]=p.read_bytes()
 portable=dict(record)
 for k in ['glb','obj','metadata','mtl']:portable[k]=Path(record[k]).name
 portable['textures']={k:Path(p).name for k,p in record['textures'].items()}
 entries['manifest.json']=(json.dumps({'revision':'Materials R2','asset':portable},indent=2)+'\n').encode()
 entries['recipe.json']=(ROOT/'recipes'/(label+'.json')).read_bytes()
 entries['verification.json']=(json.dumps(v,indent=2)+'\n').encode()
 entries['README.txt']=('Material R2: '+label+'\n\nThis corrects secondary-material assignments lost during the original bake. Geometry, source data, hierarchy and all actual GLB geometry/UV/tangent accessor bytes are unchanged. All nine sources and binary GLB payloads were compared. The three unaffected controls retain identical map files.\n\nImport the GLB for baked PBR and hierarchy. OBJ requires the accompanying MTL and maps; use the JSON for sockets and pivots. Do not rename individual files without fixing their references. Units: meters. Original source JSON: right-handed, Z up. GLB: right-handed, Y up.\n\nStrict GLB binary validation and import round-trip passed. Finite-resolution maps approximate procedural materials. No claim is made of pixel-level outside-region identity without a per-face raster mask. No addon is required.\n').encode()
 entries['SHA256SUMS.txt']=''.join(sha(data)+'  '+name+'\n' for name,data in sorted(entries.items())).encode()
 package=freeze_zip('Jerico_'+label+'_MaterialsR2',entries);package['asset']=label;packages.append(package)
checkpoint=ROOT.parent/('JericoIndustrialPropsMaterialsR2Checkpoint_'+STAMP)
assert not checkpoint.exists()
shutil.copytree(ROOT,checkpoint,ignore=shutil.ignore_patterns('output','__pycache__','*.pyc'))
files={str(p.relative_to(checkpoint)):p.read_bytes() for p in checkpoint.rglob('*') if p.is_file() and p.name!='source-sha256.json'}
assert not any(any(part in {'output','addons','refs','references','__pycache__'} for part in Path(name).parts) for name in files)
(checkpoint/'source-sha256.json').write_text(json.dumps({name:sha(data) for name,data in sorted(files.items())},indent=2)+'\n')
files['source-sha256.json']=(checkpoint/'source-sha256.json').read_bytes()
code=freeze_zip('JericoIndustrialProps_MaterialsR2_Code',files);code['checkpoint_path']=str(checkpoint)
evidence={}
for p in (ROOT/'output/verification').iterdir():
 if p.is_file():evidence['verification/'+p.name]=p.read_bytes()
for p in (ROOT/'output/evidence').iterdir():
 if p.is_file() and '-working' not in p.name:evidence['evidence/'+p.name]=p.read_bytes()
for name in ['MATERIALS_R2_VERIFICATION.md','preserve-bake-material-slots-r2.patch']:
 evidence[name]=(OUT/name).read_bytes()
for p in OUT.glob('materials-r2-comparison_*.png'):evidence[p.name]=p.read_bytes()
proof=freeze_zip('JericoIndustrialProps_MaterialsR2_Evidence',evidence)
result=dict(portable_packages=packages,code=code,evidence=proof,comparison=str(next(OUT.glob('materials-r2-comparison_*.png'))),patch=str(OUT/'preserve-bake-material-slots-r2.patch'),report=str(OUT/'MATERIALS_R2_VERIFICATION.md'),verification=str(ROOT/'output/verification/materials-r2-verification.json'))
(OUT/'materials-r2-delivery-manifest.json').write_text(json.dumps(result,indent=2)+'\n');print(json.dumps(result,indent=2))
