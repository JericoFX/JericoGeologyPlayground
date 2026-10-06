#!/usr/bin/env python3
"""Optional offline scalar-height diagnostics; not artistic acceptance or physical calibration."""
import argparse,hashlib,json,pathlib
import numpy as np
from scipy.ndimage import gaussian_filter
p=argparse.ArgumentParser();p.add_argument('--reference',required=True);p.add_argument('--candidate',required=True);p.add_argument('--width',type=int,default=1024);p.add_argument('--height',type=int,default=1024);p.add_argument('--out',required=True);args=p.parse_args()
def measure(path):
 data=pathlib.Path(path).read_bytes();a=np.frombuffer(data,dtype='<u2').reshape(args.height,args.width).astype(float);a=(a-a.min())/(a.max()-a.min())
 result={'path':path,'sha256':hashlib.sha256(data).hexdigest(),'normalizedHeightStd':float(a.std()),'scales':[]}
 for sigma in (2,8,32):
  gy,gx=np.gradient(gaussian_filter(a,sigma));ex=float((gx*gx).mean());ey=float((gy*gy).mean());result['scales'].append({'gaussianSigmaPixels':sigma,'gradientEnergyX':ex,'gradientEnergyY':ey,'ratioYoverX':ey/ex})
 return result
out={'method':'Each uint16 map independently min/max normalized; scipy Gaussian filter (reflect), numpy central-gradient energy averaged over the complete image. Axis-specific Y/X ratio, NOT structure-tensor eigenvalue ratio. Diagnostic statistics, not a naturalness score or physical height calibration.','reference':measure(args.reference),'candidate':measure(args.candidate)}
pathlib.Path(args.out).write_text(json.dumps(out,indent=2)+'\n')
