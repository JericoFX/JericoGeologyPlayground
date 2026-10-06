#!/usr/bin/env python3
"""Label immutable, hash-verified candidate6/candidate7 direct geometry renders."""
from pathlib import Path
import hashlib,json
from PIL import Image,ImageDraw,ImageFont
ROOT=Path(__file__).resolve().parents[1];BASE=ROOT.parent/'lab-families-expansion';DEST=ROOT/'renders/comparisons';DEST.mkdir(parents=True,exist_ok=True)
font='/usr/share/fonts/truetype/dejavu/DejaVuSans.ttf';bold=font.replace('.ttf','-Bold.ttf')
def f(size,b=False):return ImageFont.truetype(bold if b else font,size)
def sha(p):return hashlib.sha256(p.read_bytes()).hexdigest()
def verified(root,directory,seed,variant,view,mode):
 stem=f'slate_foliated-{seed}-{variant}-{view}-{mode}';p=root/'renders'/directory/'provenance'/(stem+'.json');r=json.loads(p.read_text());im=p.parent.parent/r['image']
 assert sha(im)==r['png_sha256'];assert r['imported_geometry_normals_transforms_unchanged']
 for name,h in r['source_hashes'].items():assert sha((root/r['obj']).parent/name)==h
 return r,im,p
manifests=[]
for view,mode in [('main','neutral'),('alternate','neutral'),('detail','neutral'),('main','wireframe')]:
 W=1584;H=160+3*812+64;out=Image.new('RGB',(W,H),'#edf0ef');d=ImageDraw.Draw(out);d.text((24,20),'Pizarra: base aprobada y prueba de roturas finitas',font=f(30,True),fill='#172f27');d.text((24,64),'Misma geometría de soporte, cámara, iluminación y material neutro.',font=f(23),fill='#4b5d55');records=[]
 d.text((24,110),'Candidate6 · base aprobada',font=f(25,True),fill='#1e4e3d');d.text((804,110),'Candidate7 · experimento',font=f(25,True),fill='#1e4e3d')
 for row,seed in enumerate((48,117,305)):
  a,ia,pa=verified(ROOT if view=='detail' else BASE,'candidate6-supplementary' if view=='detail' else 'candidate6',seed,'new',view,mode);b,ib,pb=verified(ROOT,'candidate7',seed,'new',view,mode)
  assert a['settings']==b['settings'],(seed,view,mode)
  assert a['recipe']==b['recipe'];assert a['renderer_sha256']==b['renderer_sha256']
  y=156+row*812
  for x,p in [(24,ia),(804,ib)]:
   im=Image.open(p);assert im.size==(768,768);out.paste(im,(x,y));d.text((x,y+770),f'Semilla {seed} · {view} · {mode}',font=f(20),fill='#354a41')
  records.append({'seed':seed,'baseline_record':str(pa),'current_record':str(pb),'baseline_png_sha256':sha(ia),'current_png_sha256':sha(ib),'equal_settings':True,'equal_recipe':True,'equal_renderer':True})
 d.text((24,H-46),'OBJ directo; sin textura, desplazamiento de render ni cambios de normales. Candidate6 no se sustituye.',font=f(20),fill='#4b5d55')
 path=DEST/f'pizarra-candidate6-candidate7-{view}-{mode}.png';out.save(path);manifests.append({'image':str(path.relative_to(ROOT)),'sha256':sha(path),'records':records});print(path,flush=True)
W=2376;H=160+3*812+64;out=Image.new('RGB',(W,H),'#edf0ef');d=ImageDraw.Draw(out)
d.text((24,20),'Pizarra · perfil anterior, control y experimento',font=f(32,True),fill='#172f27')
d.text((24,65),'Misma cámara, iluminación y dimensiones físicas. El perfil anterior es un comparador del laboratorio, no la misma litología.',font=f(24),fill='#4b5d55')
records=[]
for col,(variant,label) in enumerate([('legacy','Perfil anterior v11'),('control','Soporte · featureDepth = 0'),('new','Candidate7 · experimento')]):
 x=24+col*780;d.text((x,110),label,font=f(25,True),fill='#1e4e3d')
 for row,seed in enumerate((48,117,305)):
  rec,p,pr=verified(ROOT,'candidate7',seed,variant,'main','neutral')
  ref,_,_=verified(ROOT,'candidate7',seed,'new','main','neutral')
  assert rec['settings']==ref['settings']
  im=Image.open(p);assert im.size==(768,768);y=156+row*812;out.paste(im,(x,y));d.text((x,y+770),f'Semilla {seed}',font=f(20),fill='#354a41')
  records.append({'seed':seed,'variant':variant,'record':str(pr),'png_sha256':sha(p),'equal_settings':True})
d.text((24,H-46),'OBJ directo; sin textura, desplazamiento de render ni cambios de normales.',font=f(20),fill='#4b5d55')
path=DEST/'pizarra-anterior-control-candidate7-main-neutral.png';out.save(path);manifests.append({'image':str(path.relative_to(ROOT)),'sha256':sha(path),'records':records});print(path,flush=True)
(DEST/'comparison_manifest.json').write_text(json.dumps({'comparisons':manifests},indent=2)+'\n')
