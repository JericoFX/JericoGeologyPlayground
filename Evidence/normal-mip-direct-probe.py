import json, sys
from pathlib import Path
import numpy as np
from PIL import Image

def audit(folder):
    base = np.asarray(Image.open(folder / 'normal.png').convert('RGBA'), dtype=np.float32)
    out = {'folder': str(folder), 'levels': []}
    rgb = base[:, :, :3] * (2.0 / 255.0) - 1.0
    alpha = base[:, :, 3] > 0
    for level in (1, 2, 3):
        block = 2 ** level
        size = base.shape[0] // block
        sums = (rgb * alpha[:, :, None]).reshape(size, block, size, block, 3).sum(axis=(1, 3))
        count = alpha.reshape(size, block, size, block).sum(axis=(1, 3))
        direct = sums / np.maximum(np.linalg.norm(sums, axis=2, keepdims=True), 1e-12)
        mip = np.asarray(Image.open(folder / f'normal-mip-{level}.png').convert('RGBA'), dtype=np.float32)
        encoded = mip[:, :, :3] * (2.0 / 255.0) - 1.0
        current = encoded / np.maximum(np.linalg.norm(encoded, axis=2, keepdims=True), 1e-12)
        valid = count > 0
        full = count == block * block
        dot = np.clip((direct * current).sum(axis=2), -1.0, 1.0)
        angle = np.degrees(np.arccos(dot))
        def stats(mask):
            vals = angle[mask]
            return {'count': int(vals.size), 'median': float(np.median(vals)), 'p95': float(np.percentile(vals, 95)), 'p99': float(np.percentile(vals, 99)), 'max': float(vals.max())} if vals.size else None
        out['levels'].append({'level': level, 'full': stats(full), 'partial': stats(valid & ~full)})
    return out

for name in sys.argv[1:]:
    print(json.dumps(audit(Path(name)), indent=2))
