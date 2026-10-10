#!/usr/bin/env python3
"""Private review contact sheet; pass external reference paths, never bundle assets."""
import argparse
from pathlib import Path
from PIL import Image,ImageDraw,ImageFont
p=argparse.ArgumentParser();p.add_argument('--reference',required=True);p.add_argument('--render',required=True);p.add_argument('--title',required=True);p.add_argument('--out',required=True);a=p.parse_args()
font='/usr/share/fonts/truetype/dejavu/DejaVuSans.ttf'
large=ImageFont.truetype(font,31);small=ImageFont.truetype(font,22)
canvas=Image.new('RGB',(900,1464),(245,244,240));d=ImageDraw.Draw(canvas)
d.text((24,18),a.title,fill=(35,36,38),font=large)
d.text((24,70),'Referencia: albedo',fill=(60,60,60),font=small);d.text((464,70),'Referencia: altura',fill=(60,60,60),font=small)
reference=Image.open(a.reference).convert('RGB')
for k in range(2):canvas.paste(reference.crop((512*k,38,512*(k+1),550)).resize((426,426),Image.Resampling.LANCZOS),(24+440*k,108))
d.text((24,554),'Nuevo estudio matemático · malla real',fill=(35,36,38),font=large)
render=Image.open(a.render).convert('RGB').resize((852,852),Image.Resampling.LANCZOS);canvas.paste(render,(24,606))
# Physical render details are in the verified provenance accompanying the original PNG.
Path(a.out).parent.mkdir(parents=True,exist_ok=True);canvas.save(a.out)
