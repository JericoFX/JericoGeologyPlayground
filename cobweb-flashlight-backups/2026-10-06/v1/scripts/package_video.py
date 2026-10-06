#!/usr/bin/env python3
"""Assemble 24 actual Cycles sweep renders; no brightness interpolation or opacity animation."""
from pathlib import Path
import json
import subprocess
from PIL import Image,ImageDraw,ImageFont
from contract import CLASSES,FIXED,require_new
ROOT=Path(__file__).resolve().parents[1]
run=ROOT/'renders/sweep01';manifest=json.loads((run/'manifest.json').read_text())
assert len(manifest['frames'])==24,'Sweep is incomplete'
assert len({x['scene_identity'] for x in manifest['frames']})==1
out=ROOT/'delivery/film_frames';require_new([out]);out.mkdir()
fontfile='/usr/share/fonts/truetype/dejavu/DejaVuSans.ttf'
f=ImageFont.truetype(fontfile,21);small=ImageFont.truetype(fontfile,16)
frames=[(ROOT/'renders/final01/off.png','SIN LINTERNA · AMBIENTE BAJO')]*6
frames += [(run/x['png'],'BARRIDO REAL DE UN FOCO · TELAS Y CÁMARA FIJAS') for x in manifest['frames']]
frames += [(ROOT/'renders/final01/off.png','SIN LINTERNA · LA COBERTURA SIGUE SIENDO LA MISMA')]*6
for index,(path,label) in enumerate(frames):
    im=Image.open(path).convert('RGB').resize((1000,414),Image.Resampling.LANCZOS)
    film=Image.new('RGB',(1000,520),(16,22,28));film.paste(im,(0,62));d=ImageDraw.Draw(film)
    d.text((22,17),label,font=f,fill=(231,236,239))
    for c in CLASSES:
        x=int((.5+c['x']/FIXED['ortho_scale'])*1000)
        name=c['label'];width=d.textbbox((0,0),name,font=small)[2]
        d.text((x-width//2,491),name,font=small,fill=(169,189,203))
    film.save(out/f'frame_{index:04d}.png')
target=ROOT/'delivery/barrido_linterna_tres_clases.mp4';require_new([target])
subprocess.run(['ffmpeg','-nostdin','-v','warning','-framerate','6','-i',str(out/'frame_%04d.png'),'-c:v','libx264','-preset','slow','-crf','18',
  '-pix_fmt','yuv420p','-movflags','+faststart',str(target)],check=True)
print(target)
