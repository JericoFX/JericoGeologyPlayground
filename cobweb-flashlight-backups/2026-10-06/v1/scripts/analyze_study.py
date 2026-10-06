#!/usr/bin/env python3
"""Measure fixed coverage-masked screen ROIs in scene-linear beauty images."""
import argparse
import hashlib
import json
from pathlib import Path
import sys
import numpy as np
from PIL import Image,ImageDraw,ImageFont
ROOT=Path(__file__).resolve().parents[1]
sys.path.insert(0,str(ROOT/'scripts'))
from contract import CLASSES,FIXED,asset_identity,require_new,sha256

def analyze(run):
    manifest=json.loads((run/'manifest.json').read_text())
    assert len(manifest['frames'])==5
    assert len({f['scene_identity'] for f in manifest['frames']})==1
    assert len(set(manifest['contract']['mesh_hashes'].values()))==1
    for c in CLASSES:
        assert asset_identity(ROOT/'assets'/c['id'])==manifest['contract']['asset_hashes'][c['id']]
    coverage={name:np.load(next(run.glob(name+'_coverage_*.npy'))) for name in ['off','left','center','right','backlight']}
    initial=coverage['off'];h,w=initial.shape
    max_difference=max(float(np.max(np.abs(arr-initial))) for arr in coverage.values())
    assert max_difference==0, f'Coverage AOV changed across light states: {max_difference}'
    images={name:np.load(run/(name+'.linear.npy')) for name in coverage}
    report={'measurement':'Coverage-masked scene-linear camera luminance; includes transmitted/reflected background contributions, not isolated silk BSDF or perceived visibility',
      'coverage_AOV_max_absolute_difference':max_difference,'coverage_AOV_array_sha256':hashlib.sha256(initial.tobytes()).hexdigest(),
      'scene_identity_identical':True,'same_geometry_across_classes':True,'all_asset_hashes_unchanged':True,'classes':{}}
    for idx,c in enumerate(CLASSES):
        center=.5+c['x']/FIXED['ortho_scale']
        # Interior to the frame, and limited to the cloth vertical span.
        x0=int((center-.105)*w);x1=int((center+.105)*w);y0=int(.18*h);y1=int(.82*h)
        region=np.zeros((h,w),dtype=bool);region[y0:y1,x0:x1]=True
        mask=region & (initial>.08)
        assert mask.sum()>100,'Insufficient strand pixels in fixed ROI'
        values={}
        for name,arr in images.items():
            lum=arr[:,:,:3]@np.array([.2126,.7152,.0722])
            values[name]={'mean':float(np.mean(lum[mask])),'p90':float(np.quantile(lum[mask],.9)),'p99':float(np.quantile(lum[mask],.99))}
        on=['left','center','right'][idx]
        ratio=values[on]['mean']/values['off']['mean']
        assert ratio>5, f'Weak light response for {c["id"]}: {ratio}'
        png=np.asarray(Image.open(run/(on+'.png')).convert('RGB'))
        clipping=float(np.mean(np.max(png[mask],axis=1)==255))
        assert clipping<.01,'Beauty is clipped white in masked ROI'
        report['classes'][c['id']]={'roi':[x0,y0,x1,y1],'mask_pixels':int(mask.sum()),'mean_coverage_in_roi':float(initial[region].mean()),
          'direct_light_state':on,'direct_to_off_mean_radiance_ratio':ratio,'display_clipped_pixel_fraction':clipping,'linear_luminance':values}
    return report

if __name__=='__main__':
    parser=argparse.ArgumentParser();parser.add_argument('run');args=parser.parse_args()
    run=ROOT/'renders'/args.run
    report=analyze(run);target=run/'measurement.json';require_new([target]);target.write_text(json.dumps(report,indent=2)+'\n')
    print(json.dumps(report,indent=2))
