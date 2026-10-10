"""Compose labeled evidence sheets from actual renders. No generative fill, retouching or altered geometry."""
from pathlib import Path
from PIL import Image, ImageDraw, ImageFont
import json,hashlib
HERE=Path(__file__).resolve().parent
R=HERE/'renders'
BG='#F3F1EC';FG='#232B30';MUT='#56626A';GREEN='#2C625D';RED='#944936'
FONT='/usr/share/fonts/truetype/dejavu/DejaVuSans.ttf';BOLD='/usr/share/fonts/truetype/dejavu/DejaVuSans-Bold.ttf'
log=[]
def font(size,b=False):return ImageFont.truetype(BOLD if b else FONT,size)
def txt(d,xy,s,size=26,color=FG,b=False):d.text(xy,s,fill=color,font=font(size,b))
def panel(im,filename,xy,size):
 p=R/filename;src=Image.open(p).convert('RGB');src=src.resize((size,size),Image.Resampling.LANCZOS);im.paste(src,xy);log.append({'source':str(p.relative_to(HERE)),'sha256':hashlib.sha256(p.read_bytes()).hexdigest(),'destination_xy':xy,'destination_size':[size,size],'operation':'uniform resize and paste; no pixel retouching'})
def save(im,name):
 p=HERE/name;im.save(p);return p
# Hero: approved seed 48 geometry is byte-identical.
im=Image.new('RGB',(1280,1440),BG);d=ImageDraw.Draw(im)
txt(d,(40,28),'Referencia aprobada · seed 48',38,b=True)
txt(d,(40,82),'Geometría conservada exactamente · OBJ C++ directo',25,color=MUT)
panel(im,'approved-48-hero.png',(40,132),1200)
txt(d,(40,1360),'Misma forma aprobada. Sin texturas, desplazamiento ni retoque.',24,color=MUT)
save(im,'hero-aprobada-48.png')
# Seven recovered seeds: all same camera/material/light at 800 square before uniform downsampling.
im=Image.new('RGB',(2240,1500),BG);d=ImageDraw.Draw(im)
txt(d,(40,28),'Las siete semillas corregidas',44,b=True)
txt(d,(40,86),'Etapa erosionada · n64 · misma cámara, iluminación y material neutro',28,color=MUT)
notes={4:'1 cara resuelta · 1 contracción',5:'1 cara resuelta',8:'1 cara resuelta · base corregida',14:'1 contracción microscópica',25:'2 caras resueltas · 1 contracción',29:'1 cara resuelta',31:'2 caras resueltas · unión local'}
for i,s in enumerate((4,5,8,14,25,29,31)):
 x=40+(i%4)*545;y=144+(i//4)*636
 panel(im,f'corrected-{s}-main.png',(x,y),525)
 txt(d,(x,y+537),f'Seed {s}',30,b=True);txt(d,(x,y+579),notes[s],22,color=MUT)
x=40+3*545;y=144+636
d.rounded_rectangle((x,y,x+525,y+525),radius=12,fill='#E3E7E3')
txt(d,(x+28,y+30),'Qué se compara',31,b=True)
for j,line in enumerate(['Salidas reales del generador C++.','No se agregó daño ni detalle.','','La reparación es local.','El estilo y la forma general','se mantienen.','','Las imágenes no sustituyen','la revisión del código ni las','pruebas geométricas.']):txt(d,(x+28,y+90+j*35),line,24,color=MUT)
txt(d,(40,1430),'Render: Blender 5.2.2 LTS · Cycles CPU · 2 threads · hashes y ajustes en manifest.json',25,color=MUT)
save(im,'siete-corregidas.png')
# Join: neutral surfaces plus actual imported mesh-edge shader, same view in both columns.
if (R/'corrected-31-join-edges.png').exists():
 im=Image.new('RGB',(1680,1990),BG);d=ImageDraw.Draw(im)
 txt(d,(40,26),'Seed 31 · corrección de la unión local',43,b=True)
 txt(d,(40,86),'Antes = diagnóstico rechazado. No es el modelo actual del PC.',27,color=RED)
 txt(d,(40,140),'ANTES · diagnóstico rechazado',30,color=RED,b=True);txt(d,(860,140),'DESPUÉS · corregida',30,color=GREEN,b=True)
 for j,mode in enumerate(('neutral','edges')):
  y=194+j*842
  for x,v in ((40,'rejected'),(860,'corrected')):panel(im,f'{v}-31-join-{mode}.png',(x,y),780)
  txt(d,(40,y+785),'Superficie neutra' if j==0 else 'Aristas reales del OBJ · mismo shader en ambos lados',23,color=MUT)
 txt(d,(40,1880),'Detalle real: celdas [50,31,42–44]. Encuadre de 0,23 m, ampliado 11,74× frente al general.',25,color=MUT)
 txt(d,(40,1921),'Sin editar vértices, normales o transformaciones. Misma cámara y luces dentro de cada par.',24,color=MUT)
 save(im,'antes-despues-union-31.png')
# Microscopic collapse: general pair and direct edge-shader detail at real coordinates.
if (R/'corrected-14-micro-edges.png').exists():
 im=Image.new('RGB',(1680,2060),BG);d=ImageDraw.Draw(im)
 txt(d,(40,26),'Seed 14 · contracción microscópica',43,b=True)
 txt(d,(40,86),'Cambio máximo: 1,73789 µm. No es visible en el render general.',28,color=MUT)
 txt(d,(40,140),'ANTES · diagnóstico rechazado',30,color=RED,b=True);txt(d,(860,140),'DESPUÉS · corregida',30,color=GREEN,b=True)
 for x,v in ((40,'rejected'),(860,'corrected')):panel(im,f'{v}-14-main.png',(x,194),780)
 txt(d,(40,995),'Detalle microscópico real · ancho del encuadre: 14 µm · ≈193 000× frente al general',28,b=True)
 for x,v in ((40,'rejected'),(860,'corrected')):panel(im,f'{v}-14-micro-edges.png',(x,1046),780)
 txt(d,(40,1850),'Aristas reales del OBJ: 1 arista contraída, 2 triángulos retirados.',26,color=MUT)
 txt(d,(40,1891),'Zoom de cámara, sin escalar ni deformar la malla. Distancia medida en coordenadas originales.',24,color=MUT)
 txt(d,(40,1932),'El detalle usa precisión de importación/render de Blender; la medida numérica proviene del OBJ.',24,color=MUT)
 txt(d,(40,1973),'El “antes” es una exportación diagnóstica rechazada, no el modelo actual del PC.',24,color=RED)
 save(im,'antes-despues-micro-14.png')
(HERE/'composition-record.json').write_text(json.dumps({'operations':log,'note':'Labels and margins added outside render frames; uniform resize only; no geometry or image content edits','script_sha256':hashlib.sha256(Path(__file__).read_bytes()).hexdigest()},indent=2,ensure_ascii=False)+'\n')
print('Created:',*[p.name for p in HERE.glob('*.png')])
