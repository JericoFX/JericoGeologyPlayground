import sys
from pathlib import Path
import numpy as np
from PIL import Image
for text in sys.argv[1:]:
    folder=Path(text)
    base=np.asarray(Image.open(folder/'normal.png').convert('RGBA'),dtype=np.float32)
    rgb=base[:,:,:3]*(2/255)-1
    alpha=base[:,:,3]>0
    print(folder)
    for level in (1,2,3):
        b=2**level
        n=base.shape[0]//b
        sums=(rgb*alpha[:,:,None]).reshape(n,b,n,b,3).sum(axis=(1,3))
        counts=alpha.reshape(n,b,n,b).sum(axis=(1,3))
        length=np.linalg.norm(sums,axis=2)/np.maximum(counts,1)
        for label,mask in (('full',counts==b*b),('partial',(counts>0)&(counts<b*b))):
            v=length[mask]
            print(level,label,'count',v.size,'min',float(v.min()),'p5',float(np.percentile(v,5)),'median',float(np.median(v)),'fraction<.95',float((v<.95).mean()),'fraction<.9',float((v<.9).mean()))
