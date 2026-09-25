from pathlib import Path
import numpy as np
from PIL import Image
roots=[Path('Evidence/2026-09-25-slate-far-baseline/rock-11/lod-2'),Path('Evidence/2026-09-25-slate-far-lod2-1024/rock-11/lod-2')]
for r in roots:
    for suffix in ('','-far'):
        hi=np.asarray(Image.open(r/f'high-material{suffix}.png').convert('RGB'),dtype=np.float32)
        low=np.asarray(Image.open(r/f'low-material{suffix}.png').convert('RGB'),dtype=np.float32)
        mask=np.mean(hi,axis=2)>30
        dif=np.abs(hi-low).mean(axis=2)
        print(r,suffix or 'near','meanRGBMAE',float(dif[mask].mean()),'p95PixelMAE',float(np.percentile(dif[mask],95)),'objectPixels',int(mask.sum()))
