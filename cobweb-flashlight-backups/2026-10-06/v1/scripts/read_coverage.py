"""Convert Blender's scene-linear coverage AOV to NumPy for independent QA."""
import sys
from pathlib import Path
import bpy
import numpy as np
run=Path(sys.argv[sys.argv.index('--')+1])
for path in sorted(run.glob('*_coverage_*.exr')):
    target=path.with_suffix('.npy')
    if target.exists():continue
    img=bpy.data.images.load(str(path.resolve()),check_existing=False)
    values=np.asarray(img.pixels[:],dtype=np.float32).reshape((img.size[1],img.size[0],4))[::-1,:,0]
    np.save(target,values)
    bpy.data.images.remove(img)
    print(path.name,values.min(),values.max(),values.mean(),flush=True)
