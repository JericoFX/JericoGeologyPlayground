#!/usr/bin/env python3
"""Layout unmodified Cycles render pixels into an explicitly labeled evidence sheet."""
from pathlib import Path
from PIL import Image,ImageDraw,ImageFont
from contract import CLASSES,FIXED,require_new
ROOT=Path(__file__).resolve().parents[1]
fontfile='/usr/share/fonts/truetype/dejavu/DejaVuSans.ttf'
def font(n):return ImageFont.truetype(fontfile,n)
run=ROOT/'renders/final01';out=ROOT/'delivery';out.mkdir(exist_ok=True)
path=out/'comparacion_linterna_tres_clases.png';require_new([path])
size=(1530,1950);sheet=Image.new('RGB',size,(16,22,28));d=ImageDraw.Draw(sheet)
d.text((35,22),'TELARAÑAS · RESPUESTA A LA LINTERNA',font=font(36),fill=(237,242,247))
d.text((36,73),'Misma geometría y anclajes · cobertura fija por clase · cámara y exposición fijas',font=font(20),fill=(156,178,192))
for i,c in enumerate(CLASSES):d.text((i*500+40,117),c['label'],font=font(27),fill=(210,225,237))
rows=[('SIN LINTERNA · solo ambiente bajo',['off']*3),('FOCO SOBRE CADA CLASE · tres tomas separadas',['left','center','right']),('CONTRALUZ · mismo foco detrás de los tres paños',['backlight']*3)]
for r,(label,names) in enumerate(rows):
    y=164+r*565;d.text((35,y),label,font=font(22),fill=(221,189,111))
    for i,(c,name) in enumerate(zip(CLASSES,names)):
        raw=Image.open(run/(name+'.png')).convert('RGB');w,h=raw.size
        center=.5+c['x']/FIXED['ortho_scale'];half=.145
        crop=raw.crop((int((center-half)*w),int(.05*h),int((center+half)*w),int(.95*h)))
        crop.thumbnail((470,510),Image.Resampling.LANCZOS)
        x=i*500+30+(470-crop.width)//2
        sheet.paste(crop,(x,y+38))
d.text((35,1874),'La luz revela la tela; no activa su opacidad. Seda artística aproximada, no seda física medida.',font=font(20),fill=(181,196,205))
d.text((35,1904),'La tenue puede delatarse con fondos claros o más luz ambiente. Esto no demuestra resistencia.',font=font(20),fill=(181,196,205))
sheet.save(path)
# Five state-wide screenshots preserve the whole scene and exact relative positions.
for name,label in [('off','SIN LINTERNA'),('left','FOCO A LA IZQUIERDA'),('center','FOCO AL CENTRO'),('right','FOCO A LA DERECHA'),('backlight','CONTRALUZ')]:
    raw=Image.open(run/(name+'.png')).convert('RGB');w,h=raw.size
    annotated=Image.new('RGB',(w,h+105),(16,22,28));annotated.paste(raw,(0,70));draw=ImageDraw.Draw(annotated)
    draw.text((25,15),label,font=font(27),fill=(235,239,243))
    for c in CLASSES:
        x=int((.5+c['x']/FIXED['ortho_scale'])*w)
        draw.text((x-95,h+75),c['label'],font=font(20),fill=(173,190,201))
    target=out/(name+'_rotulado.png');require_new([target]);annotated.save(target)
print(path)
